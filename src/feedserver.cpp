#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <haywire.h>
#include "opt.h"

#include "json.h"
#include "json-traverse.h"
#include "vc_vector.h"
#include "vc_string.h"
#include "userfeed.h"
#include "feedarticle.h"
#include "rocksdb/c.h"
#include "rocksdb/options.h"
#include "rocksdb/slice.h"
#include "rocksdb/utilities/transaction.h"

#include "feedserver.h"

// URI Routes 

// 1.a subscribeuser: Subscribe a User to a Feed 
#define USERSUBSCRIBE_URI "/subscribeuser"

// 1.b unsubscribeuser: Unsubscribe a User from a feed
#define UNSUBSCRIBE_URI "/unsubscribeuser"

// 2. feed: Add Articles to a Feed
#define FEED_URI "/feed"
#define FEED_URI_SPLAT "/feed/*"

// 3. Get all Feeds a Subscriber is following 
#define USER_URI "/user"
#define USER_URI_SPLAT "/user/*"

// 4. Get Articles from the set of Feeds a Subscriber is following
#define USERARTICLES_URI "/userarticles"
#define USERARTICLES_URI_SPLAT "/userarticles/*"

// Database Engine: RocksDB 
//
// uf: User Feed database
// fa: Feed Article database

using namespace rocksdb;

#include <unistd.h>  // sysconf() - get CPU count

const char rocksdb_uf_path[] = "/tmp/rocksdb_ua";
const char rocksdb_fa_path[] = "/tmp/rocksdb_fa";
DB *rocksdb_uf;
DB *rocksdb_fa;

// Route Object
//
// grab the User or Feed from the URI
//
// argument: uri="/foo/bar HTTP/1.0..."
//
// returns: object=("feed"|"user")
//
char *route_object(char *uri, size_t uri_length){
  char *haystack = uri;
  char *needle, *needle_end;

  if (uri_length <= 1) return NULL;

  if (haystack[0] != '/') return NULL;

  haystack = &haystack[1];
  needle_end = strchr(haystack,' ');

  if (needle_end==NULL) return NULL;

  if ((needle_end - haystack) > 0)
    haystack = strndup(haystack,(size_t)(needle_end-haystack));
  if (haystack==NULL) return NULL;

  needle = strchr(haystack,'/');
  if (needle!=NULL)
    needle = strdup(needle+1);

  free(haystack);
  return needle;
}

// Route Object Validator
//
// validate if object consists of alpha or numeric characters
//
// argument: object=("feed"|"user")
//
// returns: 1=true|0=false
//
int isobject_valid(char *object){
  if (!object) return 0;
  do{
    if (!isalnum(*object++)) return 0;
  } while(*object != NULL);
  return 1;
}

// Article Validator
//
// validate if article consists of printable characters
//
// argument: article=("lorem ipsum")
//
// returns: 1=true|0=false
//
int isarticle_valid(char *article){
  if (!article) return 0;
  do{
    if (!isalnum(*article++)) return 0;
  } while(*article != NULL);
  return 1;
}

// Decline Request
//
// quickly decline the request using HTTP Response Code and Message
//
// arguments:
//            code="400 Bad Request"
//            msg ="Client issued bad request"
//            req = haywire request object
//            res = haywire response object
static void decline(char *code, char *msg, http_request *req, 
                      hw_http_response *res)
{
  FEEDER_ROUTE_DECLS;

  body.value = msg;
  body.length = strlen(msg);
  hw_set_body(res, &body);

  FEEDER_ROUTE_STATUS(code);
  FEEDER_ROUTE_HTML_CONTENT;
  FEEDER_ROUTE_SEND_BOILERPLATE;
}

// 1.a User Subscribe Route Handler
//
// ```
// PUT /subscribeuser
// ```
// *Payload*
// 
// ```
// {
//   "user": "foo",
//   "feed": "bar"
// }
// ```
// 
// **Response**
// 
// ```
// {
//   "user": { 
//     "name": "foo" , 
//     "uri": "/user/foo" , 
//   },
//   "feeds": [
//     { "name": "bar" , "uri": "/feed/bar" }
//   ]
// }
//
// arguments: req=haywire request object, res=haywire response object, udata=NULL
//
// Better to use rocksdb merge class here
// https://github.com/facebook/rocksdb/wiki/merge-operator
void subscribe_user(http_request *req, hw_http_response *res, void *udata){
  FEEDER_ROUTE_DECLS;

  FEEDER_SHUTDOWN_CHECK;

  if (req->method != HW_HTTP_PUT)
    return decline(HTTP_STATUS_501,"Not Implemented!",req,res);
  if (req->body->length <= 0)
    return decline(HTTP_STATUS_400,"Empty JSON Payload!",req,res);

  // User Feed Request
  uf_pair_t *uf_req = uf_pair_from_request(req->body->value,req->body->length);

  if (!uf_req) return decline(HTTP_STATUS_500,"Parse error in payload!",req,res);

  std::string db_val;
  vc_vector *vec_uf_res;

  // Lookup User
  Status s = rocksdb_uf->Get(ReadOptions(),uf_req->user,&db_val);

  // User Not Found
  if (s.IsNotFound()){
    vec_uf_res = uf_pair_to_dbvalue(uf_req,USER_URI,FEED_URI);

    if (!vec_uf_res) 
      return decline(HTTP_STATUS_500,"uf_pair_to_dbvalue(uf_req) fail!",req,res);

    // Insert New User
    s = rocksdb_uf->Put(WriteOptions(),uf_req->user,vc_string_begin(vec_uf_res));
    if (!s.ok()){
      uf_pair_release(uf_req);
      vc_vector_release(vec_uf_res);
      return decline(HTTP_STATUS_500,(char*)s.ToString().c_str(),req,res);
    }
  } 
  // Database Error in Lookup
  else if (!s.ok()){
    uf_pair_release(uf_req);
    return decline(HTTP_STATUS_500,(char*)s.ToString().c_str(),req,res);
  } 
  // Database Error in Lookup Value
  else if (db_val.size() <= 0){
    uf_pair_release(uf_req);
    return decline(HTTP_STATUS_500,(char*)s.ToString().c_str(),req,res);
  }
  // User Found: Merge Feed add Request into Database
  else {
    uf_pair_t *uf_db = uf_pair_from_dbvalue(db_val.c_str(),db_val.size());

    if (!uf_db){
      uf_pair_release(uf_req);
      return decline(HTTP_STATUS_500,"uf_pair_from_dbvalue(uf_db) fail!",req,res);
    }

    // Search for Feeds to add
    vc_vector *vec_f = uf_req->feeds;
    void *vec_v; 
    int new_feeds=0;
    for (
      vec_v=vc_vector_begin(vec_f);
      vec_v!=vc_vector_end(vec_f);
      vec_v=vc_vector_next(vec_f,vec_v))
    {
      if(uf_pair_add_feed(uf_db,STRING_PTR(vec_v)))
        new_feeds = 1;
    }

    // New Feeds found in Request, so push to Database
    if (new_feeds){
      vec_uf_res = uf_pair_to_dbvalue(uf_db,USER_URI,FEED_URI);
      if (!vec_uf_res) {
        uf_pair_release(uf_req);
        uf_pair_release(uf_db);
        return decline(HTTP_STATUS_500,"uf_pair_to_dbvalue(uf_db)!",req,res);
      }
      s = rocksdb_uf->Put(WriteOptions(),uf_db->user,vc_string_begin(vec_uf_res));
      if (!s.ok()){
        uf_pair_release(uf_req);
        uf_pair_release(uf_db);
        vc_vector_release(vec_uf_res);
        return decline(HTTP_STATUS_500,(char*)s.ToString().c_str(),req,res);
      }
    } 
    // No Feeds, so send 304 No Modified or already Subscribed
    else {
        uf_pair_release(uf_req);
        uf_pair_release(uf_db);
        return decline(HTTP_STATUS_304,"User already subscribed!",req,res);
    }

    uf_pair_release(uf_db);
  }

  if (!vec_uf_res) return decline(HTTP_STATUS_500,"subscribe_user() logic error!",req,res);

  body.value = vc_string_begin(vec_uf_res);
  body.length = vc_vector_size(vec_uf_res);
  hw_set_body(res, &body);

  FEEDER_ROUTE_STATUS(HTTP_STATUS_200);
  FEEDER_ROUTE_JSON_CONTENT;
  FEEDER_ROUTE_SEND_BOILERPLATE;

  if (vec_uf_res) vc_string_release(vec_uf_res);
  if (uf_req) uf_pair_release(uf_req);
}

// 1.b Unsubscribe User from Feed Route Handler
//
// ```
// PUT /unsubscribeuser
// ```
// 
// *Payload*
// 
// ```
// {
//   "user": "foo",
//   "feed": "bar"
// }
// 
// **Response**
// 
// ```
// {
//   "user": { 
//     "name": "foo" , 
//     "uri": "/user/foo"
//   },
//   "feeds": [ ]
// }
// ```
//
// arguments: req=haywire request object, res=haywire response object, udata=NULL
//
// Better to use rocksdb merge class here
// https://github.com/facebook/rocksdb/wiki/merge-operator
void unsubscribe_user(http_request *req, hw_http_response *res, void *udata){
  FEEDER_ROUTE_DECLS;

  FEEDER_SHUTDOWN_CHECK;

  if (req->method != HW_HTTP_PUT)
    return decline(HTTP_STATUS_501,"Not Implemented!",req,res);
  if (req->body->length <= 0)
    return decline(HTTP_STATUS_400,"Empty JSON Payload!",req,res);

  // User Feed Request
  uf_pair_t *uf_req = uf_pair_from_request(req->body->value,req->body->length);

  if (!uf_req) return decline(HTTP_STATUS_500,"Parse error in payload!",req,res);

  std::string db_val;
  vc_vector *vec_uf_res;

  // Lookup User
  Status s = rocksdb_uf->Get(ReadOptions(),uf_req->user,&db_val);

  // User Not Found
  if (s.IsNotFound()){
    uf_pair_release(uf_req);
    return decline(HTTP_STATUS_404,"User not found!",req,res);
  }
  // Database Error in Lookup
  else if (!s.ok()){
    uf_pair_release(uf_req);
    return decline(HTTP_STATUS_500,(char*)s.ToString().c_str(),req,res);
  } 
  // Database Error in Lookup Value
  else if (db_val.size() <= 0){
    uf_pair_release(uf_req);
    return decline(HTTP_STATUS_500,(char*)s.ToString().c_str(),req,res);
  }
  // User Found: Merge Feed delete Request into Database
  else {
    uf_pair_t *uf_db = uf_pair_from_dbvalue(db_val.c_str(),db_val.size());

    if (!uf_db){
      uf_pair_release(uf_req);
      return decline(HTTP_STATUS_500,"uf_pair_from_dbvalue(uf_db) fail!",req,res);
    }

    // Search for Feeds to add
    vc_vector *vec_f = uf_req->feeds;
    void *vec_v; 
    int feeds_deleted=0;
    for (
      vec_v=vc_vector_begin(vec_f);
      vec_v!=vc_vector_end(vec_f);
      vec_v=vc_vector_next(vec_f,vec_v))
    {
      if(uf_pair_delete_feed(uf_db,STRING_PTR(vec_v)))
        feeds_deleted = 1;
    }

    // New Feeds to delete found in Request, so push to Database
    if (feeds_deleted){
      vec_uf_res = uf_pair_to_dbvalue(uf_db,USER_URI,FEED_URI);
      if (!vec_uf_res) {
        uf_pair_release(uf_req);
        uf_pair_release(uf_db);
        return decline(HTTP_STATUS_500,"uf_pair_to_dbvalue(uf_db)!",req,res);
      }
      s = rocksdb_uf->Put(WriteOptions(),uf_db->user,vc_string_begin(vec_uf_res));
      if (!s.ok()){
        uf_pair_release(uf_req);
        uf_pair_release(uf_db);
        vc_vector_release(vec_uf_res);
        return decline(HTTP_STATUS_500,(char*)s.ToString().c_str(),req,res);
      }
    } 
    // No Feeds, send 304 - User Not subscribed feeds
    else {
      uf_pair_release(uf_req);
      uf_pair_release(uf_db);
      return decline(HTTP_STATUS_304,"User Not Subscribed!",req,res);
    }

    uf_pair_release(uf_db);
  }

  if (!vec_uf_res) return decline(HTTP_STATUS_500,"user_subscribe() logic error!",req,res);

  body.value = vc_string_begin(vec_uf_res);
  body.length = vc_vector_size(vec_uf_res);
  hw_set_body(res, &body);

  FEEDER_ROUTE_STATUS(HTTP_STATUS_200);
  FEEDER_ROUTE_JSON_CONTENT;
  FEEDER_ROUTE_SEND_BOILERPLATE;

  if (vec_uf_res) vc_string_release(vec_uf_res);
  if (uf_req) uf_pair_release(uf_req);
}

// 2. Add Articles to a Feed
// 
// ```
// POST /feed/bar
// ```
// 
// *Payload*
// 
// ```
// {
//     "articles": [
//       { "content": "lorem ipsum" },
//       { "content":  "dolor sit amet"  }
//     ]
// }
// ```
// 
// **Response**
// 
// ```
// {
//     "articles": [
//       { "disposition": "new" }
//       { "disposition": "would_duplicate" }
//     ]
// }
// ```
// arguments: req=haywire request object, res=haywire response object, udata=NULL
void submit_articles(http_request *req, hw_http_response *res, void *udata){
  FEEDER_ROUTE_DECLS;

  FEEDER_SHUTDOWN_CHECK;

  if (req->method != HW_HTTP_POST)
    return decline(HTTP_STATUS_501,"Not Implemented!",req,res);
  if (req->body->length <= 0)
    return decline(HTTP_STATUS_400,"Empty JSON Payload!",req,res);

  // Get Feed from URI
  char *feed = route_object(req->url->value,req->url->length);
  if (feed==NULL)
    return decline(HTTP_STATUS_400,"Bad feed in uri!",req,res);
 
  if (!isobject_valid(feed)){
    free(feed);
    return decline(HTTP_STATUS_400,"Bad feed in uri!",req,res);
  }

  // Feed Article submission from Request
  fa_submission_t *fa_req= fa_submission_from_request(feed,req->body->value,req->body->length);

  free(feed);

  if (!fa_req) {
    return decline(HTTP_STATUS_500,"Parse error in payload!",req,res);
  }

  // Add Articles
  Status s;
  vc_vector *vec_a = fa_req->articles;
  void *vec_va;
  size_t i;
  for (
      vec_va=vc_vector_begin(vec_a), i = 0;
      vec_va!=vc_vector_end(vec_a);
      vec_va=vc_vector_next(vec_a,vec_va), i++
  ){
    s = rocksdb_fa->Put(WriteOptions(),STRING_PTR(vec_va),"");
    if (s.ok())
      fa_submission_article_disp(fa_req,i,"written");
    else
      fa_submission_article_disp(fa_req,i,s.ToString().c_str());
  }

  vc_vector *vec_fa_res = fa_submission_response(fa_req);
 
  if (!vec_fa_res){
    fa_submission_release(fa_req);
    return decline(HTTP_STATUS_500,"fa_submission_response(fa_req) failed!",req,res);
  }
  
  body.value = vc_string_begin(vec_fa_res);
  body.length = vc_vector_size(vec_fa_res);
  hw_set_body(res, &body);

  FEEDER_ROUTE_STATUS(HTTP_STATUS_200);
  FEEDER_ROUTE_JSON_CONTENT;
  FEEDER_ROUTE_SEND_BOILERPLATE;

  if (vec_fa_res) vc_string_release(vec_fa_res);
  if (fa_req) fa_submission_release(fa_req);
}

// 3. Get all Feeds a Subscriber is following
// 
// ```
// GET /user/foo
// ```
// 
// **Response**
// 
// ```
//   "user": { 
//     "name": "foo" , 
//     "uri": "/user/foo" , 
//   },
//   "feeds": [
//     { "name": "bar" , "uri": "/feed/bar" }
//   ]
// }
//
// arguments: req=haywire request object, res=haywire response object, udata=NULL
//
void get_user(http_request *req, hw_http_response *res, void *udata){
  FEEDER_ROUTE_DECLS;

  FEEDER_SHUTDOWN_CHECK;

  if (req->method != HW_HTTP_GET)
    return decline(HTTP_STATUS_501,"Not Implemented!",req,res);

  // Get User from URI
  char *user = route_object(req->url->value,req->url->length);
  if (user==NULL)
    return decline(HTTP_STATUS_400,"Bad user in uri!",req,res);
 
  if (!isobject_valid(user)){
    free(user);
    return decline(HTTP_STATUS_400,"Bad user in uri!",req,res);
  }

  std::string db_val;
  vc_vector *vec_uf_res;

  // Lookup User
  Status s = rocksdb_uf->Get(ReadOptions(),user,&db_val);

  free(user);

  // User Not Found
  if (s.IsNotFound())
    return decline(HTTP_STATUS_404,"User not found!",req,res);

  // Database Error in Lookup
  else if (!s.ok())
    return decline(HTTP_STATUS_500,(char*)s.ToString().c_str(),req,res);

  // Database Error in Lookup Value
  else if (db_val.size() <= 0)
    return decline(HTTP_STATUS_500,(char*)s.ToString().c_str(),req,res);

  // User Found: Return Request
  body.value = (char *)db_val.c_str();
  body.length = db_val.size();
  hw_set_body(res, &body);

  FEEDER_ROUTE_STATUS(HTTP_STATUS_200);
  FEEDER_ROUTE_JSON_CONTENT;
  FEEDER_ROUTE_SEND_BOILERPLATE;
}

// 4. Get Articles from the set of Feeds a Subscriber is following
// 
// ```
// GET /userarticles/foo
// ```
// 
// **Response**
// 
// ```
// {
//   [
//     {   
//       "feed": "bar", 
//       "article": "lorem ipsum"
//     },
//     { "feed": "bar",
//       "article": "dolor sit amet"
//     }
//   ]
// }
// ```
void get_articles(http_request *req, hw_http_response *res, void *udata){
  FEEDER_ROUTE_DECLS;

  FEEDER_SHUTDOWN_CHECK;

  if (req->method != HW_HTTP_GET)
    return decline(HTTP_STATUS_501,"Not Implemented!",req,res);

  // Get User from URI
  char *user = route_object(req->url->value,req->url->length);
  if (user==NULL)
    return decline(HTTP_STATUS_400,"Bad user in uri!",req,res);
 
  if (!isobject_valid(user)){
    free(user);
    return decline(HTTP_STATUS_400,"Bad user in uri!",req,res);
  }

  std::string db_val;
  vc_vector *vec_uf_res;

  // Lookup User
  Status s = rocksdb_uf->Get(ReadOptions(),user,&db_val);

  free(user);

  // User Not Found
  if (s.IsNotFound())
    return decline(HTTP_STATUS_404,"User not found!",req,res);

  // Database Error in Lookup
  else if (!s.ok())
    return decline(HTTP_STATUS_500,(char*)s.ToString().c_str(),req,res);

  // Database Error in Lookup Value
  else if (db_val.size() <= 0)
    return decline(HTTP_STATUS_500,(char*)s.ToString().c_str(),req,res);

  // User Found: 
  uf_pair_t *uf_db = uf_pair_from_dbvalue(db_val.c_str(),db_val.size());
  if (!uf_db)
    return decline(HTTP_STATUS_500,"uf_pair_from_dbvalue(uf_db) fail!",req,res);

  vc_vector *vec_ua_res = vc_string_create();

  vc_string_append(vec_ua_res,"{\"articles\":[\n");
  size_t prefix_length = vc_string_length(vec_ua_res);

  // Loop over Feeds
  Iterator *it = rocksdb_fa->NewIterator(ReadOptions());
  vc_vector *vec_f = uf_db->feeds;
  void *vec_v; 
  char *feed, *article;
  size_t feed_length;
  for (
      vec_v=vc_vector_begin(vec_f);
      vec_v!=vc_vector_end(vec_f);
      vec_v=vc_vector_next(vec_f,vec_v))
  {
    feed = STRING_PTR(vec_v);
    feed_length = strlen(feed);
    for (
      it->Seek(feed); 
      it->Valid() && strncmp(feed,it->key().ToString().c_str(),feed_length) == 0;
      it->Next()
    ){
      vc_string_append(vec_ua_res,"{\"feed\":\"");
      vc_string_append(vec_ua_res,feed);
      vc_string_append(vec_ua_res,"\",\"content\":\"");
      article = (char *)it->key().ToString().c_str();
      article += feed_length + 1;
      vc_string_append(vec_ua_res,article);
      vc_string_append(vec_ua_res,"\"},\n");
    }
  }
  delete it;

  if (vc_string_length(vec_ua_res) > prefix_length){
    // Remove last comman and newline
    vc_string_n_remove(vec_ua_res,2);
    // End
    vc_string_append(vec_ua_res,"\n]}\n");
  } else {
    vc_string_release(vec_ua_res);
    uf_pair_release(uf_db);
    return decline(HTTP_STATUS_204,"No articles for user!",req,res);
  }

  body.value = vc_string_begin(vec_ua_res);
  body.length = vc_string_length(vec_ua_res);
  hw_set_body(res, &body);

  FEEDER_ROUTE_STATUS(HTTP_STATUS_200);
  FEEDER_ROUTE_JSON_CONTENT;
  FEEDER_ROUTE_SEND_BOILERPLATE;

  vc_string_release(vec_ua_res);
  uf_pair_release(uf_db);
}

void shutdown_feeder(void *udata){

  /* TODO: wait for db write activity to finish and then close database */

  delete rocksdb_uf;
  delete rocksdb_fa;

  exit(0);
}
void shutdown_feeder_response(http_request *req, hw_http_response *res, void *udata){
  FEEDER_ROUTE_DECLS;
  char *msg = "Shutting down!";
  feeder_shutdown = 1;

  body.value = msg;
  body.length = strlen(msg);
  hw_set_body(res, &body);

  FEEDER_ROUTE_STATUS(HTTP_STATUS_503);
  FEEDER_ROUTE_HTML_CONTENT;
  hw_set_http_version(res, 1, 0);
  hw_http_response_send(res, NULL , shutdown_feeder); 
}

int main(int args, char** argsv)
{
  configuration config;

  /* Haywire Init */

  config.http_listen_address = "0.0.0.0";
  struct opt_config *conf;
  conf = opt_config_init();
  opt_flag_int(conf, (int32_t *)&config.http_listen_port, "port", 8000, "Port to listen on.");
  opt_flag_int(conf, (int32_t *)&config.thread_count, "threads", 0, 
        "Number of threads to use.");
  opt_flag_string(conf, &config.balancer, "balancer", "ipc", 
                  "Method to load balance threads.");
  opt_flag_string(conf, &config.parser, "parser", "http_parser", "HTTP parser to use");
  opt_flag_int(conf, (int32_t *)&config.max_request_size, "max_request_size", 1048576, 
                  "Maximum request size. Defaults to 1MB.");
  opt_flag_bool(conf, &config.tcp_nodelay, "tcp_nodelay", 
                    "If present, enables tcp_nodelay (i.e. disables Nagle's algorithm).");
  opt_flag_int(conf, (int32_t *)&config.listen_backlog, "listen_backlog", 0, 
            "Maximum size of the backlog when accepting connection. Defaults to SOMAXCONN.");
  opt_config_parse(conf, args, argsv);

  hw_init_with_config(&config);

  // Routes init
  hw_http_add_route(USERSUBSCRIBE_URI,subscribe_user, NULL);
  hw_http_add_route(UNSUBSCRIBE_URI, unsubscribe_user, NULL);
  hw_http_add_route(FEED_URI_SPLAT, submit_articles, NULL);
  hw_http_add_route(USER_URI_SPLAT, get_user, NULL);
  hw_http_add_route(USERARTICLES_URI_SPLAT, get_articles, NULL);
  hw_http_add_route("/shutdown",shutdown_feeder_response,NULL);

  // RocksDB init
  Options options;
  options.IncreaseParallelism();
  options.create_if_missing = true;
  options.enable_thread_tracking=true;

  // open DBs
  Status s;
  s = DB::Open(options,rocksdb_uf_path,&rocksdb_uf);
  assert(s.ok());

  s = DB::Open(options,rocksdb_fa_path,&rocksdb_fa);
  assert(s.ok());

  //  Event Loop
  hw_http_open();

  opt_config_free(conf);
  return 0;
}
