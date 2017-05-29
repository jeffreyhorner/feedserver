#include <stdio.h>
#include <assert.h>
#include "json.h"
#include "json-traverse.h"
#include "vc_vector.h"
#include "vc_string.h"
#include "feedarticle.h"

int main(int argc, char **argv){
  FILE *fd;
  char ibuf[512];
  size_t ilen, i;
  fa_submission_t *fa;
  vc_vector *ivec, *favec;

  assert(argc == 2);
  assert((fd = fopen(argv[1],"r")) != NULL);

  ivec = vc_string_create();

  while((ilen = fread(ibuf,1,512,fd)) > 0){
    vc_string_n_append(ivec,ibuf,ilen);
  }
  fclose(fd);

  /* Test: JSON parse error */
  assert(fa_submission_from_request("bar"," ",1) == NULL);

  /* Test: JSON parse success */
  assert(fa = fa_submission_from_request("bar",vc_string_begin(ivec),vc_string_length(ivec)));

  /* Update article disposition */
  for (i = 0; i < fa_submission_article_count(fa); i++){
    fa_submission_article_disp(fa,i,"new");
  }
 
  /* Test: Create article disposition response */
  assert(favec = fa_submission_response(fa));

  printf("%s",vc_string_begin(favec));

  fa_submission_release(fa);
  vc_string_release(favec);
  vc_string_release(ivec);
  return 0;
}
