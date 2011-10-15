#include <iostream>
#include <mecab.h>

#define CHECK(eval) if (! eval) { \
   const char *e = tagger ? tagger->what() : MeCab::getTaggerError(); \
   std::cerr << "Exception:" << e << std::endl; \
   delete tagger; \
   return -1; }

int main (int argc, char **argv) 
{
  char input[1024] = "太郎は次郎が持っている本を花子に渡した。";

  MeCab::Tagger *tagger = MeCab::createTagger("");
  CHECK(tagger);

  const char *result = tagger->parse(input);
  CHECK(result);
  std::cout << "INPUT: " << input << std::endl;
  std::cout << "RESULT: " << result << std::endl;
   
  tagger->set_lattice_level(1);
  result = tagger->parseNBest(3, input);
  CHECK(result); 
  std::cout << "NBEST: " << std::endl << result;

  CHECK(tagger->parseNBestInit(input));
  for (int i = 0; i < 3; ++i) {
    std::cout << i << ":" << std::endl << tagger->next();
  } 

  const MeCab::Node* node = tagger->parseToNode(input);
  CHECK(node);
  for (; node; node = node->next) {
    std::cout.write(node->surface, node->length);
  }

  tagger->set_lattice_level(2);    
  node = tagger->parseToNode(input);
  CHECK(node);

  for (; node; node = node->next) {
    std::cout << node->id << ' ';
    if (node->stat == MECAB_BOS_NODE)
      std::cout << "BOS";
    else if (node->stat == MECAB_EOS_NODE)
      std::cout << "EOS";
    else
      std::cout.write (node->surface, node->length);

    std::cout << ' ' << node->feature
	      << ' ' << (int)(node->surface - input)
	      << ' ' << (int)(node->surface - input + node->length)
	      << ' ' << node->rcAttr
	      << ' ' << node->lcAttr
	      << ' ' << node->posid
	      << ' ' << (int)node->char_type
	      << ' ' << (int)node->stat
	      << ' ' << (int)node->isbest
	      << ' ' << node->alpha
	      << ' ' << node->beta
	      << ' ' << node->prob
	      << ' ' << node->cost << std::endl;
  }

  tagger->set_lattice_level(0);
  node = tagger->parseToNode(input);
  size_t len = node->sentence_length;
  for (int i = 0; i <= len; ++i) {
    MeCab::Node *b = node->begin_node_list[i];
    MeCab::Node *e = node->end_node_list[i];
    for (; b; b = b->bnext) {
      printf("B[%d] %s\t%s\n", i, b->surface, b->feature);
    }
    for (; e; e = e->enext) {
      printf("E[%d] %s\t%s\n", i, e->surface, e->feature);
    }
  }

  tagger->set_all_morphs(true);
  node = tagger->parseToNode(input);   
  for (; node; node = node->next) {
    if (node->stat == MECAB_BOS_NODE)
      std::cout << "BOS";
    else if (node->stat == MECAB_EOS_NODE)
      std::cout << "EOS";
    else
      std::cout.write (node->surface, node->length);
    std::cout << "\t" << node->feature << std::endl; 
  }

  const MeCab::DictionaryInfo *d = tagger->dictionary_info();
  for (; d; d = d->next) {
    std::cout << "filename: " <<  d->filename << std::endl;
    std::cout << "charset: " <<  d->charset << std::endl;
    std::cout << "size: " <<  d->size << std::endl;
    std::cout << "type: " <<  d->type << std::endl;
    std::cout << "lsize: " <<  d->lsize << std::endl;
    std::cout << "rsize: " <<  d->rsize << std::endl;
    std::cout << "version: " <<  d->version << std::endl;
  }


  delete tagger;
   
  return 0;
}
