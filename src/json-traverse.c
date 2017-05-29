#include "json-traverse.h"

json_value *json_find_object(json_value *jv, char const *name){
  int i;
  json_value *jo;

  if (jv==NULL) return NULL;

  switch (jv->type) {
    case json_none:
      return NULL;
      break;
    case json_object:
      for(i = 0; i < jv->u.object.length; i++){
        if ( strcmp(name,jv->u.object.values[i].name)==0){
          return jv->u.object.values[i].value;
        } else {
          jo = json_find_object(jv->u.object.values[i].value,name);
          if (jo) return jo;
        }
      }
      break;
    case json_array:
      for(i = 0; i < jv->u.array.length; i++){
        jo = json_find_object(jv->u.array.values[i],name);
        if (jo) return jo;
      }
      break;
    default:
      break;
  }
  return NULL;
}
