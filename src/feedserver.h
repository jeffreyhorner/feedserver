#ifdef __cplusplus
extern "C" {
#endif

#define CRLF "\r\n"

#define FEEDER_ROUTE_DECLS \
  hw_string status_code; \
  hw_string content_type_name; \
  hw_string content_type_value;\
  hw_string body; \
  hw_string keep_alive_name; \
  hw_string keep_alive_value;

#define FEEDER_ROUTE_SEND_BOILERPLATE \
  if (req->keep_alive) \
  {  \
    SETSTRING(keep_alive_name, "Connection"); \
 \
    SETSTRING(keep_alive_value, "Keep-Alive"); \
    hw_set_response_header(res, &keep_alive_name, &keep_alive_value); \
  } \
  else \
  { \
    hw_set_http_version(res, 1, 0); \
  } \
 \
  hw_http_response_send(res, NULL , NULL); 

#define FEEDER_ROUTE_HTML_CONTENT \
  SETSTRING(content_type_name, "Content-Type"); \
  SETSTRING(content_type_value, "text/html"); \
  hw_set_response_header(res, &content_type_name, &content_type_value); 

#define FEEDER_ROUTE_JSON_CONTENT \
  SETSTRING(content_type_name, "Content-Type"); \
  SETSTRING(content_type_value, "application/json"); \
  hw_set_response_header(res, &content_type_name, &content_type_value); 

#define FEEDER_ROUTE_STATUS(X) \
  SETSTRING(status_code, X); \
  hw_set_response_status_code(res, &status_code);

/* Graceful Shutdown; signaled by a 'GET /shutdown' */
int feeder_shutdown = 0;

#define FEEDER_SHUTDOWN_CHECK \
  if (feeder_shutdown!=0){ \
    decline(HTTP_STATUS_503,"Shutting down!",req,res); \
    return; \
  }
  
#ifdef __cplusplus
}  /* end extern "C" */
#endif
