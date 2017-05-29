#include <string.h>
#include "userfeed.h"
#include "json.h"
#include "json-traverse.h"
#include "vc_string.h"

#define STRING_PTR(x) *((char **)x)
static void free_string(void *data){
  free(STRING_PTR(data));
}

uf_pair_t *uf_pair_new(char const *user, char const *feed){
  uf_pair_t *uf = malloc(sizeof(uf_pair_t));
  char *feed_name;

  uf->user = strdup(user);
  uf->feeds = vc_vector_create(1,sizeof(char *), free_string);

  if (feed!=NULL){
    feed_name = strdup(feed);
    vc_vector_push_back(uf->feeds, &feed_name);
  }
  
  return uf;
}

int uf_pair_add_feed(uf_pair_t *uf, char const *feed){
  char *feed_name;
  vc_vector *vf = uf->feeds;
  void *vval;
  size_t i;
  for (
      vval = vc_vector_begin(vf), i = 0; 
      vval != vc_vector_end(vf); 
      vval = vc_vector_next(vf,vval), i++
      ){
    if (strcmp(STRING_PTR(vval),feed)==0) return 0;
  }
  feed_name = strdup(feed);
  vc_vector_push_back(uf->feeds, &feed_name);
  return 1;
}

int uf_pair_delete_feed(uf_pair_t *uf, char const *feed){
  vc_vector *vf = uf->feeds;
  void *vval;
  size_t i;
  for (
    vval = vc_vector_begin(vf), i = 0; 
    vval != vc_vector_end(vf); 
    vval = vc_vector_next(vf,vval), i++
  ){
    if (strcmp(STRING_PTR(vval),feed)==0){
      vc_vector_erase(vf,i);
      return 1;
    }
  }
  return 0;
}

void uf_pair_release(uf_pair_t *uf){
  free(uf->user);
  vc_vector_release(uf->feeds);
  free(uf);
}

uf_pair_t *uf_pair_from_request(char const *json_payload, size_t payload_length ){
  size_t i;
  json_value *jv, *jo, *ji;
  uf_pair_t *ufp=NULL;
  char const *user;

  jv = json_parse((json_char *)json_payload,payload_length);
  if (jv==NULL) return NULL;

  if (jv->type != json_object || jv->u.object.length != 2){
    json_value_free(jv);
    return NULL;
  }

  ji = json_find_object(jv,"user");
  if (ji == NULL){
    json_value_free(jv);
    return NULL;
  }

  if (ji->type == json_array){
    if (ji->u.array.length != 1){
      json_value_free(jv);
      return NULL;
    }
    jo = (json_value *)ji->u.array.values[0];
    user = jo->u.string.ptr;
  } else if (ji->type == json_string){
    user = ji->u.string.ptr;
  } else {
    json_value_free(jv);
    return NULL;
  }

  ji = json_find_object(jv,"feeds");

  if (ji == NULL){
    json_value_free(jv);
    return NULL;
  }
  if (ji->type == json_array){
    if (ji->u.array.length <= 0){
      json_value_free(jv);
      return NULL;
    }
    ufp = uf_pair_new(user, NULL);
    for (i = 0; i < ji->u.array.length; i++){
      jo = (json_value *)ji->u.array.values[i];
      if (jo->type == json_string)
        uf_pair_add_feed(ufp,jo->u.string.ptr);
    }
  } else if (ji->type == json_string){
    /* create */
    ufp = uf_pair_new(user, ji->u.string.ptr);
  } else {
    json_value_free(jv);
    return NULL;
  }

  json_value_free(jv);

  return ufp;
}

vc_vector *uf_pair_to_dbvalue(uf_pair_t *ufp, char const *useruri, char const *feeduri){
  vc_vector *vs = vc_string_create();
  vc_vector *vf = ufp->feeds;
  void *vval;

  vc_string_append(vs,"{\n\"user\": {\n\"name\": \"");
  vc_string_append(vs,ufp->user);
  vc_string_append(vs,"\",\n\"uri\": \"");
  vc_string_append(vs,useruri);
  vc_string_append(vs,"/");
  vc_string_append(vs,ufp->user);
  vc_string_append(vs,"\"\n},\n\"feeds\": [\n");
  for (vval = vc_vector_begin(vf); vval != vc_vector_end(vf); vval = vc_vector_next(vf,vval)){
    vc_string_append(vs,"{\n\"name\": \"");
    vc_string_append(vs,STRING_PTR(vval));
    vc_string_append(vs,"\",\n\"uri\": \"");
    vc_string_append(vs,feeduri);
    vc_string_append(vs,"/");
    vc_string_append(vs,STRING_PTR(vval));
    vc_string_append(vs,"\"\n}\n");
    if (vc_vector_next(vf,vval) != vc_vector_end(vf))
      vc_string_append(vs,",\n");
  }
  vc_string_append(vs,"]\n}\n");

  return vs;
}

uf_pair_t *uf_pair_from_dbvalue(char const *json_payload, size_t payload_length ){
  size_t i;
  json_value *jv, *jo, *ji;
  uf_pair_t *ufp=NULL;

  jv = json_parse((json_char *)json_payload,payload_length);
  if (jv==NULL) return NULL;

  if (jv->type != json_object){
    json_value_free(jv);
    return NULL;
  }

  jo = json_find_object(jv,"user");
  jo = json_find_object(jo,"name");

  if (jo==NULL){
    json_value_free(jv);
    return NULL;
  }

  ufp = uf_pair_new(jo->u.string.ptr,NULL);

  ji = json_find_object(jv,"feeds");
  if (ji->type == json_array){
    for (i = 0; i < ji->u.array.length; i++){
      jo = json_find_object(ji->u.array.values[i],"name");
      uf_pair_add_feed(ufp,jo->u.string.ptr);
    }
  }
  json_value_free(jv);

  return ufp;
}
