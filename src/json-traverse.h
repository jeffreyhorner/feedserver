#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "json.h"

json_value *json_find_object(json_value *jv, char const *name);

#ifdef __cplusplus
}  /* end extern "C" */
#endif
