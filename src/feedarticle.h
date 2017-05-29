#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "vc_vector.h"
#include "userfeed.h"

typedef struct
{
  char *feed;
  vc_vector *articles;
  vc_vector *disp;
} fa_submission_t;

fa_submission_t *fa_submission_from_request(char const *feed, char const *json_payload, 
                                size_t payload_length);
fa_submission_t *fa_submission_new(char const *feed);
void fa_submission_add_article(fa_submission_t *fa, char const *article);
void fa_submission_release(fa_submission_t *fa);
size_t fa_submission_article_count(fa_submission_t *fa);
void fa_submission_article_disp(fa_submission_t *fa, size_t idx, char const *disp);
vc_vector *fa_submission_response(fa_submission_t *fa);

#ifndef STRING_PTR
#define STRING_PTR(x) *((char **)x)
#endif

#ifdef __cplusplus
}  /* end extern "C" */
#endif
