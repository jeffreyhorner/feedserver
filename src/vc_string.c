#pragma once

#include "vc_string.h"

#define VC_STRING_MIN_SIZE 512

static inline bool vc_string_add_null_terminator(vc_vector *v){
  char null_terminator = '\0';
  return vc_vector_append(v,&null_terminator,1);
}
vc_vector *vc_string_create(void){
  vc_vector *v = vc_vector_create(VC_STRING_MIN_SIZE,1,NULL);
  vc_string_add_null_terminator(v);
  return v;
}

bool vc_string_append(vc_vector *v, char const *str){
  vc_string_n_append(v, str, strlen(str));
}

bool vc_string_n_append(vc_vector *v, char const *str, size_t n){
  vc_vector_pop_back(v);
  vc_vector_append(v,(const void *)str, n);
  return vc_string_add_null_terminator(v);
}

bool vc_string_n_remove(vc_vector *v, size_t n){
  if (n <= 0) return (bool)0;
  do {
    vc_vector_pop_back(v);
  } while(--n);
  vc_vector_pop_back(v);
  return vc_string_add_null_terminator(v);
}

char *vc_string_begin(vc_vector *v){
  return (char *)vc_vector_begin(v);
}

size_t vc_string_length(vc_vector *v){
  return vc_vector_size(v) - 1;
}

void vc_string_release(vc_vector *v){
  vc_vector_release(v);
}

vc_vector *vc_string_json_deparse_char(char *s){

  if (s==NULL || *s == '\0') return NULL;

  vc_vector *v_d = vc_vector_create(VC_STRING_MIN_SIZE,1,NULL);

  while(*s != '\0')
  {
    switch(*s)
    {
      case '\b':  vc_vector_append(v_d,"\\b", 2);  break;
      case '\f':  vc_vector_append(v_d,"\\f", 2);  break;
      case '\n':  vc_vector_append(v_d,"\\n", 2);  break;
      case '\r':  vc_vector_append(v_d,"\\r", 2);  break;
      case '\t':  vc_vector_append(v_d,"\\t", 2);  break;
      case '"':   vc_vector_append(v_d,"\\\"",2);  break;
      case '\\':  vc_vector_append(v_d,"\\\\",2);  break;
      case '/':   vc_vector_append(v_d,"\\/", 2);  break;
      default:    vc_vector_append(v_d,    s, 1);  break;
    }
    s++;
  }

  vc_string_add_null_terminator(v_d);
  return v_d;
}
