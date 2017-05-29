#include <stdio.h>
#include <assert.h>
#include "json.h"
#include "json-traverse.h"
#include "vc_vector.h"
#include "vc_string.h"
#include "userfeed.h"

int main(int argc, char **argv){
  FILE *fd;
  char ibuf[512];
  size_t ilen;
  uf_pair_t *ufp;
  vc_vector *ivec, *ufvec, *ufvec_cmp;

  assert(argc == 2);
  assert((fd = fopen(argv[1],"r")) != NULL);

  ivec = vc_string_create();

  while((ilen = fread(ibuf,1,512,fd)) > 0){
    vc_string_n_append(ivec,ibuf,ilen);
  }
  fclose(fd);

  /* Test: JSON parse error */
  assert(uf_pair_from_request(" ",1) == NULL);

  /* Test: JSON parse success */
  assert(ufp = uf_pair_from_request(vc_string_begin(ivec),vc_string_length(ivec)));

  /* Test: dbvalue build success */
  assert(ufvec = uf_pair_to_dbvalue(ufp,"/user","/feed"));

  uf_pair_release(ufp);

  printf("%s",vc_string_begin(ufvec));

  /* Test: userfeed build success */
  assert(ufp = uf_pair_from_dbvalue(vc_string_begin(ufvec),vc_string_length(ufvec)));

  assert(ufvec_cmp = uf_pair_to_dbvalue(ufp,"/user","/feed"));

  printf("%s",vc_string_begin(ufvec_cmp));

  /* Test: dbvalue == dbvalue from userfeed */
  assert(strcmp(vc_string_begin(ufvec), vc_string_begin(ufvec_cmp)) == 0);

  vc_string_release(ufvec);
  vc_string_release(ufvec_cmp);

 
  /* Test: Add feed to user */

  uf_pair_add_feed(ufp,"baz");

  assert(ufvec = uf_pair_to_dbvalue(ufp,"/user","/feed"));

  uf_pair_release(ufp);

  assert(ufp = uf_pair_from_dbvalue(vc_string_begin(ufvec),vc_string_length(ufvec)));

  assert(ufvec_cmp = uf_pair_to_dbvalue(ufp,"/user","/feed"));

  assert(strcmp(vc_string_begin(ufvec), vc_string_begin(ufvec_cmp)) == 0);

  printf("%s",vc_string_begin(ufvec_cmp));

  vc_string_release(ufvec);
  vc_string_release(ufvec_cmp);
  
  /* Test: Remove user from feed */

  assert(uf_pair_delete_feed(ufp,"baz") == 1);

  assert(ufvec = uf_pair_to_dbvalue(ufp,"/user","/feed"));

  uf_pair_release(ufp);

  assert(ufp = uf_pair_from_dbvalue(vc_string_begin(ufvec),vc_string_length(ufvec)));

  assert(ufvec_cmp = uf_pair_to_dbvalue(ufp,"/user","/feed"));

  assert(strcmp(vc_string_begin(ufvec), vc_string_begin(ufvec_cmp)) == 0);

  printf("%s",vc_string_begin(ufvec_cmp));

  vc_string_release(ufvec);
  vc_string_release(ufvec_cmp);

  /* Test: Remove user from non-existent feed */

  assert(uf_pair_delete_feed(ufp,"bee") == 0);

  /* Test: Remove user from feed (empty out) */

  assert(uf_pair_delete_feed(ufp,"bar") == 1);

  assert(ufvec = uf_pair_to_dbvalue(ufp,"/user","/feed"));

  printf("%s",vc_string_begin(ufvec));

  uf_pair_release(ufp);
  vc_string_release(ufvec);
  vc_string_release(ivec);
  return 0;
}
