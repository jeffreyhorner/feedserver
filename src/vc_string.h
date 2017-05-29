#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "vc_vector.h"

vc_vector *vc_string_create(void);
bool vc_string_append(vc_vector *v, char const *str);
bool vc_string_n_append(vc_vector *v, char const *str, size_t n);
bool vc_string_n_remove(vc_vector *v, size_t n);
char *vc_string_begin(vc_vector *v);
size_t vc_string_length(vc_vector *v);
void vc_string_release(vc_vector *v);
vc_vector *vc_string_json_deparse_char(char *str);

#ifdef __cplusplus
}  /* end extern "C" */
#endif
