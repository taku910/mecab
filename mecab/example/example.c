#include <mecab.h>
#include <stdio.h>

#define CHECK(eval) if (! eval) { \
    fprintf (stderr, "Exception:%s\n", mecab_strerror (mecab)); \
    mecab_destroy(mecab); \
    return -1; }

int main (int argc, char **argv) 
{
  char input[] = "太郎は次郎が持っている本を花子に渡した。";
  mecab_t *mecab;
  const mecab_node_t *node;
  const char *result;
  int i;
  size_t len;

  mecab = mecab_new2("");
  CHECK(mecab);

  result = mecab_sparse_tostr(mecab, input);
  CHECK(result)
  printf ("INPUT: %s\n", input);
  printf ("RESULT:\n%s", result);

  mecab_set_lattice_level(mecab, 1);
  result = mecab_nbest_sparse_tostr (mecab, 3, input);
  CHECK(result);
  fprintf (stdout, "NBEST:\n%s", result);

  CHECK(mecab_nbest_init(mecab, input));
  for (i = 0; i < 3; ++i) {
    printf ("%d:\n%s", i, mecab_nbest_next_tostr (mecab));
  }

  node = mecab_sparse_tonode(mecab, input);
  CHECK(node);
  for (; node; node = node->next) {
    fwrite (node->surface, sizeof(char), node->length, stdout);
    printf("\t%s\n", node->feature);
  }

  mecab_set_lattice_level(mecab, 2);   
  node = mecab_sparse_tonode(mecab, input);
  CHECK(node);
  for (;  node; node = node->next) {
    printf("%d ", node->id);
    
    if (node->stat == MECAB_BOS_NODE)
      printf("BOS");
    else if (node->stat == MECAB_EOS_NODE)
      printf("EOS");
    else
      fwrite (node->surface, sizeof(char), node->length, stdout);

    printf(" %s %d %d %d %d %d %d %d %d %f %f %f %ld\n",
	   node->feature,
	   (int)(node->surface - input),
	   (int)(node->surface - input + node->length),
	   node->rcAttr,
	   node->lcAttr,
	   node->posid,
	   (int)node->char_type,
	   (int)node->stat,
	   (int)node->isbest,
	   node->alpha,
	   node->beta,
	   node->prob,
	   node->cost);
  }

  node = mecab_sparse_tonode(mecab, input);
  len = node->sentence_length;
  for (i = 0; i <= len; ++i) {
    mecab_node_t *b = node->begin_node_list[i];
    mecab_node_t *e = node->end_node_list[i];
    for (; b; b = b->bnext) {
        printf("B[%d] %s\t%s\n", i, b->surface, b->feature);
    }
    for (; e; e = e->enext) {
        printf("E[%d] %s\t%s\n", i, e->surface, e->feature);
    }
  }

  mecab_set_lattice_level(mecab, 0);
  mecab_set_all_morphs(mecab, 1); 
  node = mecab_sparse_tonode(mecab, input);
  CHECK(node);
  for (; node; node = node->next) {
    fwrite (node->surface, sizeof(char), node->length, stdout);
    printf("\t%s\n", node->feature);
  }
   

  const mecab_dictionary_info_t *d = mecab_dictionary_info(mecab);
  for (; d; d = d->next) {
    printf("filename: %s\n", d->filename);
    printf("charset: %s\n", d->charset);
    printf("size: %d\n", d->size);
    printf("type: %d\n", d->type);
    printf("lsize: %d\n", d->lsize);
    printf("rsize: %d\n", d->rsize);
    printf("version: %d\n", d->version);
  }

  mecab_destroy(mecab);
   
  return 0;
}
