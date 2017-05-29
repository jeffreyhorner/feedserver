#include "feedarticle.h"
#include "json.h"
#include "json-traverse.h"
#include "vc_string.h"

static void free_string(void *data){
  free(STRING_PTR(data));
}

fa_submission_t *fa_submission_from_request(char const *feed, char const *json_payload, 
                                size_t payload_length)
{
  size_t i;
  json_value *jv, *jo, *ji;
  fa_submission_t *fa;

  jv = json_parse((json_char *)json_payload,payload_length);
  if (jv==NULL) return NULL;

  if (jv->type != json_object){
    json_value_free(jv);
    return NULL;
  }

  fa = fa_submission_new(feed);

  ji = json_find_object(jv,"articles");

  if (ji == NULL){
    json_value_free(jv);
    fa_submission_release(fa);
    return NULL;
  }

  if (ji->type == json_array){
    for (i = 0; i < ji->u.array.length; i++){
      jo = json_find_object(ji->u.array.values[i],"content");
      if (jo->type == json_string)
        fa_submission_add_article(fa,jo->u.string.ptr);
      else if (jo->type == json_array){
        jo = jo->u.array.values[0];
        if (jo->type == json_string)
          fa_submission_add_article(fa,jo->u.string.ptr);
      }
    }
  }

  json_value_free(jv);

  return fa;
}

fa_submission_t *fa_submission_new(char const *feed){
  fa_submission_t *fa = malloc(sizeof(fa_submission_t));

  fa->feed = strdup(feed);
  fa->articles = vc_vector_create(1,sizeof(char *), free_string);
  fa->disp = vc_vector_create(1,sizeof(char *), free_string);

  return fa;
}

void fa_submission_add_article(fa_submission_t *fa, char const *article){
  vc_vector *va = vc_string_create();
  char *article_content, *disp = strdup("");
  
  vc_string_append(va,fa->feed);
  vc_string_append(va,"/");
  vc_string_append(va,article);
  article_content = strdup(vc_string_begin(va));

  vc_vector_push_back(fa->articles,&article_content);
  vc_vector_push_back(fa->disp,&disp);

  vc_string_release(va);
}

void fa_submission_release(fa_submission_t *fa){
  free(fa->feed);
  vc_vector_release(fa->articles);
  vc_vector_release(fa->disp);
  free(fa);
}

size_t fa_submission_article_count(fa_submission_t *fa){
  return vc_vector_count(fa->articles);
}

void fa_submission_article_disp(fa_submission_t *fa, size_t idx, char const *disp){
  char *disp_content = strdup(disp);
  vc_vector_replace(fa->disp,idx,&disp_content);
}

vc_vector *fa_submission_response(fa_submission_t *fa){
  vc_vector *vs = vc_string_create();
  vc_vector *vd = fa->disp;
  void *vval;

  vc_string_append(vs,"{\n\"articles\": [\n");
  for (vval = vc_vector_begin(vd); vval != vc_vector_end(vd); vval = vc_vector_next(vd,vval)){
    vc_string_append(vs,"{\"disposition\": \"");
    vc_string_append(vs,STRING_PTR(vval));
    vc_string_append(vs,"\"}\n");
    if (vc_vector_next(vd,vval) != vc_vector_end(vd))
      vc_string_append(vs,",\n");
  }
  vc_string_append(vs,"]\n}\n");

  return vs;
}


