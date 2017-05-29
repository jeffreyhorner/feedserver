#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "vc_string.h"

typedef struct
{
  char *user;
  vc_vector *feeds;
} uf_pair_t;

uf_pair_t *uf_pair_new(char const *user, char const *feed);
int uf_pair_add_feed(uf_pair_t *uf, char const *feed); 
int uf_pair_delete_feed(uf_pair_t *uf, char const *feed); 
void uf_pair_release(uf_pair_t *uf);
uf_pair_t *uf_pair_from_request(char const *json_payload, size_t payload_length);
vc_vector *uf_pair_to_dbvalue(uf_pair_t *ufp, char const *useruri, char const *feeduri);
uf_pair_t *uf_pair_from_dbvalue(char const *json_payload, size_t payload_length );

#ifndef STRING_PTR
#define STRING_PTR(x) *((char **)x)
#endif

#ifdef __cplusplus
}  /* end extern "C" */
#endif
