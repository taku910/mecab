/*
  MeCab -- Yet Another Part-of-Speech and Morphological Analyzer


  Copyright(C) 2001-2006 Taku Kudo <taku@chasen.org>
  Copyright(C) 2004-2006 Nippon Telegraph and Telephone Corporation
*/
#ifndef MECAB_MECAB_H_
#define MECAB_MECAB_H_

/* C/C++ common data structures  */
struct mecab_dictionary_info_t {
  const char                     *filename;
  const char                     *charset;
  unsigned int                    size;
  int                             type;
  unsigned int                    lsize;
  unsigned int                    rsize;
  unsigned short                  version;
  struct mecab_dictionary_info_t *next;
};

struct mecab_path_t {
  struct mecab_node_t* rnode;
  struct mecab_path_t* rnext;
  struct mecab_node_t* lnode;
  struct mecab_path_t* lnext;
  int                  cost;
  float                prob;
};

struct mecab_learner_path_t {
  struct mecab_learner_node_t*  rnode;
  struct mecab_learner_path_t*  rnext;
  struct mecab_learner_node_t*  lnode;
  struct mecab_learner_path_t*  lnext;
  double                        cost;
  const int                     *fvector;
};

struct mecab_token_t {
  unsigned short lcAttr;
  unsigned short rcAttr;
  unsigned short posid;
  short wcost;
  unsigned int   feature;
  unsigned int   compound;  /* reserved for noun compound */
};

struct mecab_node_t {
  struct mecab_node_t  *prev;
  struct mecab_node_t  *next;
  struct mecab_node_t  *enext;
  struct mecab_node_t  *bnext;
  struct mecab_path_t  *rpath;
  struct mecab_path_t  *lpath;
  const char           *surface;
  const char           *feature;
  unsigned int          id;
  unsigned short        length; /* length of morph */
  unsigned short        rlength; /* real length of morph(include white space before the morph) */
  unsigned short        rcAttr;
  unsigned short        lcAttr;
  unsigned short        posid;
  unsigned char         char_type;
  unsigned char         stat;
  unsigned char         isbest;
  float                 alpha;
  float                 beta;
  float                 prob;
  short                 wcost;
  long                  cost;
  struct mecab_token_t  *token;
};

/* almost the same as mecab_node_t.
   used only for cost estimation */
struct mecab_learner_node_t {
  struct mecab_learner_node_t *prev;
  struct mecab_learner_node_t *next;
  struct mecab_learner_node_t *enext;
  struct mecab_learner_node_t *bnext;
  struct mecab_learner_path_t *rpath;
  struct mecab_learner_path_t *lpath;
  struct mecab_learner_node_t *anext;
  const char                  *surface;
  const char                  *feature;
  unsigned int                 id;
  unsigned short               length;
  unsigned short               rlength;
  unsigned short               rcAttr;
  unsigned short               lcAttr;
  unsigned short               posid;
  unsigned char                char_type;
  unsigned char                stat;
  unsigned char                isbest;
  double                       alpha;
  double                       beta;
  short                        wcost2;
  double                       wcost;
  double                       cost;
  const int                    *fvector;
  struct mecab_token_t         *token;
};

#define MECAB_NOR_NODE  (0)
#define MECAB_UNK_NODE  (1)
#define MECAB_BOS_NODE  (2)
#define MECAB_EOS_NODE  (3)
#define MECAB_EON_NODE  (4)

#define MECAB_USR_DIC   (1)
#define MECAB_SYS_DIC   (0)
#define MECAB_UNK_DIC   (2)

#define MECAB_ONE_BEST          (1)
#define MECAB_NBEST             (2)
#define MECAB_PARTIAL           (4)
#define MECAB_MARGINAL_PROB     (8)
#define MECAB_ALTERNATIVE       (16)
#define MECAB_ALL_MORPHS        (32)
#define MECAB_ALLOCATE_SENTENCE (64)

/* C interface  */
#ifdef __cplusplus
#include <cstdio>
#else
#include <stdio.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32
#include <windows.h>
#  ifdef DLL_EXPORT
#    define MECAB_DLL_EXTERN  __declspec(dllexport)
#    define MECAB_DLL_CLASS_EXTERN  __declspec(dllexport)
#  else
#    define MECAB_DLL_EXTERN  __declspec(dllimport)
#  endif
#endif

#ifndef MECAB_DLL_EXTERN
#  define MECAB_DLL_EXTERN extern
#endif

#ifndef MECAB_DLL_CLASS_EXTERN
#  define MECAB_DLL_CLASS_EXTERN
#endif

  typedef struct mecab_t                 mecab_t;
  typedef struct mecab_model_t           mecab_model_t;
  typedef struct mecab_lattice_t         mecab_lattice_t;
  typedef struct mecab_dictionary_info_t mecab_dictionary_info_t;
  typedef struct mecab_node_t            mecab_node_t;
  typedef struct mecab_learner_node_t    mecab_learner_node_t;
  typedef struct mecab_path_t            mecab_path_t;
  typedef struct mecab_learner_path_t    mecab_learner_path_t;
  typedef struct mecab_token_t           mecab_token_t;

#ifndef SWIG
  /* C interface */

  /* old mecab interface */
  MECAB_DLL_EXTERN mecab_t*      mecab_new(int argc, char **argv);
  MECAB_DLL_EXTERN mecab_t*      mecab_new2(const char *arg);
  MECAB_DLL_EXTERN const char*   mecab_version();
  MECAB_DLL_EXTERN const char*   mecab_strerror(mecab_t *mecab);
  MECAB_DLL_EXTERN void          mecab_destroy(mecab_t *mecab);

  MECAB_DLL_EXTERN int           mecab_get_partial(mecab_t *mecab);
  MECAB_DLL_EXTERN void          mecab_set_partial(mecab_t *mecab, int partial);
  MECAB_DLL_EXTERN float         mecab_get_theta(mecab_t *mecab);
  MECAB_DLL_EXTERN void          mecab_set_theta(mecab_t *mecab, float theta);
  MECAB_DLL_EXTERN int           mecab_get_lattice_level(mecab_t *mecab);
  MECAB_DLL_EXTERN void          mecab_set_lattice_level(mecab_t *mecab, int level);
  MECAB_DLL_EXTERN int           mecab_get_all_morphs(mecab_t *mecab);
  MECAB_DLL_EXTERN void          mecab_set_all_morphs(mecab_t *mecab, int all_morphs);

  MECAB_DLL_EXTERN int           mecab_parse(mecab_t *mecab, mecab_lattice_t *lattice);

  MECAB_DLL_EXTERN const char*   mecab_sparse_tostr(mecab_t *mecab, const char *str);
  MECAB_DLL_EXTERN const char*   mecab_sparse_tostr2(mecab_t *mecab, const char *str, size_t len);
  MECAB_DLL_EXTERN char*         mecab_sparse_tostr3(mecab_t *mecab, const char *str, size_t len,
                                                     char *ostr, size_t olen);
  MECAB_DLL_EXTERN const mecab_node_t* mecab_sparse_tonode(mecab_t *mecab, const char*);
  MECAB_DLL_EXTERN const mecab_node_t* mecab_sparse_tonode2(mecab_t *mecab, const char*, size_t);
  MECAB_DLL_EXTERN const char*   mecab_nbest_sparse_tostr(mecab_t *mecab, size_t N, const char *str);
  MECAB_DLL_EXTERN const char*   mecab_nbest_sparse_tostr2(mecab_t *mecab, size_t N,
                                                           const char *str, size_t len);
  MECAB_DLL_EXTERN char*         mecab_nbest_sparse_tostr3(mecab_t *mecab, size_t N,
                                                           const char *str, size_t len,
                                                           char *ostr, size_t olen);
  MECAB_DLL_EXTERN int           mecab_nbest_init(mecab_t *mecab, const char *str);
  MECAB_DLL_EXTERN int           mecab_nbest_init2(mecab_t *mecab, const char *str, size_t len);
  MECAB_DLL_EXTERN const char*   mecab_nbest_next_tostr(mecab_t *mecab);
  MECAB_DLL_EXTERN char*         mecab_nbest_next_tostr2(mecab_t *mecab, char *ostr, size_t olen);
  MECAB_DLL_EXTERN const mecab_node_t* mecab_nbest_next_tonode(mecab_t *mecab);
  MECAB_DLL_EXTERN const char*   mecab_format_node(mecab_t *mecab, const mecab_node_t *node);
  MECAB_DLL_EXTERN const mecab_dictionary_info_t* mecab_dictionary_info(mecab_t *mecab);

  /* lattice interface */
  MECAB_DLL_EXTERN mecab_lattice_t *mecab_lattice_new();
  MECAB_DLL_EXTERN void             mecab_lattice_destroy(mecab_lattice_t *lattice);
  MECAB_DLL_EXTERN void             mecab_lattice_clear(mecab_lattice_t *lattice);
  MECAB_DLL_EXTERN int              mecab_lattice_is_available(mecab_lattice_t *lattice);
  MECAB_DLL_EXTERN mecab_node_t    *mecab_lattice_get_bos_node(mecab_lattice_t *lattice);
  MECAB_DLL_EXTERN mecab_node_t    *mecab_lattice_get_eos_node(mecab_lattice_t *lattice);
  MECAB_DLL_EXTERN mecab_node_t   **mecab_lattice_get_begin_nodes(mecab_lattice_t *lattice);
  MECAB_DLL_EXTERN mecab_node_t   **mecab_lattice_get_end_nodes(mecab_lattice_t *lattice);
  MECAB_DLL_EXTERN mecab_node_t    *mecab_lattice_get_begin_node(mecab_lattice_t *lattice, size_t pos);
  MECAB_DLL_EXTERN mecab_node_t    *mecab_lattice_get_end_node(mecab_lattice_t *lattice, size_t pos);
  MECAB_DLL_EXTERN char            *mecab_lattice_strdup(mecab_lattice_t *lattice, const char *str);
  MECAB_DLL_EXTERN char            *mecab_lattice_alloc(mecab_lattice_t *lattice, size_t len);
  MECAB_DLL_EXTERN const char      *mecab_lattice_get_sentence(mecab_lattice_t *lattice);
  MECAB_DLL_EXTERN void             mecab_lattice_set_sentence(mecab_lattice_t *lattice, const char *sentence);
  MECAB_DLL_EXTERN void             mecab_lattice_get_sentence2(mecab_lattice_t *lattice, const char *sentence, size_t len);
  MECAB_DLL_EXTERN size_t           mecab_lattice_get_size(mecab_lattice_t *lattice);
  MECAB_DLL_EXTERN size_t           mecab_lattice_get_len(mecab_lattice_t *lattice);
  MECAB_DLL_EXTERN double           mecab_lattice_get_z(mecab_lattice_t *lattice);
  MECAB_DLL_EXTERN void             mecab_lattice_set_z(mecab_lattice_t *lattice, double Z);
  MECAB_DLL_EXTERN double           mecab_lattice_get_theta(mecab_lattice_t *lattice);
  MECAB_DLL_EXTERN void             mecab_lattice_set_theta(mecab_lattice_t *lattice, double theta);
  MECAB_DLL_EXTERN int              mecab_lattice_next(mecab_lattice_t *lattice);
  MECAB_DLL_EXTERN int              mecab_lattice_get_request_type(mecab_lattice_t *lattice);
  MECAB_DLL_EXTERN int              mecab_lattice_has_request_type(mecab_lattice_t *lattice, int request_type);
  MECAB_DLL_EXTERN void             mecab_lattice_set_request_type(mecab_lattice_t *lattice, int request_type);
  MECAB_DLL_EXTERN void             mecab_lattice_add_request_type(mecab_lattice_t *lattice, int request_type);
  MECAB_DLL_EXTERN void             mecab_lattice_remove_request_type(mecab_lattice_t *lattice, int request_type);
  MECAB_DLL_EXTERN const char      *mecab_lattice_tostr(mecab_lattice_t *lattice);
  MECAB_DLL_EXTERN const char      *mecab_lattice_tostr2(mecab_lattice_t *lattice, char *buf, size_t size);
  MECAB_DLL_EXTERN const char      *mecab_lattice_nbest_tostr(mecab_lattice_t *lattice, size_t N);
  MECAB_DLL_EXTERN const char      *mecab_lattice_nbest_tostr2(mecab_lattice_t *lattice, size_t N, char *buf, size_t size);
  MECAB_DLL_EXTERN const char      *mecab_lattice_strerror(mecab_lattice_t *lattice);

  /* model interface */
  MECAB_DLL_EXTERN mecab_model_t   *mecab_model_new(int argc, char **argv);
  MECAB_DLL_EXTERN mecab_model_t   *mecab_model_new2(const char *arg);
  MECAB_DLL_EXTERN void             mecab_model_destroy(mecab_model_t *model);
  MECAB_DLL_EXTERN int              mecab_model_is_available(mecab_model_t *model);
  MECAB_DLL_EXTERN mecab_t         *mecab_model_new_tagger(mecab_model_t *model);
  MECAB_DLL_EXTERN mecab_lattice_t *mecab_model_new_lattice(mecab_model_t *model);
  MECAB_DLL_EXTERN const mecab_dictionary_info_t* mecab_model_dictionary_info(mecab_model_t *model);
  MECAB_DLL_EXTERN const char      *mecab_model_strerror(mecab_model_t *model);

  /* static functions */
  MECAB_DLL_EXTERN int           mecab_do(int argc, char **argv);
  MECAB_DLL_EXTERN int           mecab_dict_index(int argc, char **argv);
  MECAB_DLL_EXTERN int           mecab_dict_gen(int argc, char **argv);
  MECAB_DLL_EXTERN int           mecab_cost_train(int argc, char **argv);
  MECAB_DLL_EXTERN int           mecab_system_eval(int argc, char **argv);
  MECAB_DLL_EXTERN int           mecab_test_gen(int argc, char **argv);
#endif

#ifdef __cplusplus
}
#endif

/* C++ interface */
#ifdef __cplusplus

namespace MeCab {
typedef struct mecab_dictionary_info_t DictionaryInfo;
typedef struct mecab_path_t            Path;
typedef struct mecab_node_t            Node;
typedef struct mecab_learner_path_t    LearnerPath;
typedef struct mecab_learner_node_t    LearnerNode;
typedef struct mecab_token_t           Token;

template <typename N, typename P> class Allocator;
class Tagger;
class NBestGenerator;

class MECAB_DLL_CLASS_EXTERN Lattice {
 public:
  virtual void clear()              = 0;
  virtual bool is_available() const = 0;

  // Node operations
  // return bos/eos node
  virtual Node *bos_node() const              = 0;
  virtual Node *eos_node() const              = 0;

#ifndef SWIG
  virtual Node **begin_nodes() const          = 0;
  virtual Node **end_nodes() const            = 0;
  virtual char *strdup(const char *str)       = 0;
  virtual char *alloc(size_t len)             = 0;
#endif

  virtual Node *end_nodes(size_t pos) const   = 0;
  virtual Node *begin_nodes(size_t pos) const = 0;

  // sentence operations
  virtual const char *sentence() const = 0;
  virtual void set_sentence(const char *sentence)             = 0;
  virtual void set_sentence(const char *sentence, size_t len) = 0;
  virtual size_t size() const                                 = 0;
  virtual size_t len()  const                                 = 0;

  virtual void   set_Z(double Z) = 0;
  virtual double Z() const = 0;

  virtual float theta() const          = 0;
  virtual void  set_theta(float theta) = 0;

  // nbest;
  virtual bool next() = 0;

  // request type
  virtual int request_type() const                = 0;
  virtual bool has_request_type(int request_type) const = 0;
  virtual void set_request_type(int request_type) = 0;
  virtual void add_request_type(int request_type) = 0;
  virtual void remove_request_type(int request_type) = 0;

#ifndef SWIG
  virtual Allocator<Node, Path> *allocator() const = 0;
  virtual NBestGenerator *nbest_generator() = 0;
#endif

  virtual const char *toString()                = 0;
  virtual const char *toString(const Node *node) = 0;
  virtual const char *enumNBestAsString(size_t N) = 0;

#ifndef SWIG
  virtual const char *toString(char *buf, size_t size) = 0;
  virtual const char *toString(const Node *node,
                               char *buf, size_t size) = 0;
  virtual const char *enumNBestAsString(size_t N, char *buf, size_t size) = 0;
#endif

  virtual const char *what() const              = 0;
  virtual void set_what(const char *str)        = 0;

  static Lattice *create();

  virtual ~Lattice() {}
};

class MECAB_DLL_CLASS_EXTERN Model {
 public:
  virtual bool open(int argc, char **argv) = 0;
  virtual bool open(const char *arg) = 0;

  virtual bool is_available() const = 0;
  virtual const DictionaryInfo *dictionary_info() const = 0;
  virtual Tagger  *createTagger() const = 0;
  virtual Lattice *createLattice() const = 0;
  virtual const char *what() const = 0;

  static const char *version();

  virtual ~Model() {}

#ifndef SIWG
  static Model* create(int argc, char **argv);
  static Model* create(const char *arg);
#endif
};

class MECAB_DLL_CLASS_EXTERN Tagger {
 public:
  // New interface
  static bool  parse(const Model &model, Lattice *lattice);
  virtual bool parse(Lattice *lattice) const                = 0;
  virtual bool parse(const char *str, Lattice *lattice) const = 0;
  virtual bool parse(const char *str, size_t len, Lattice *lattice) const = 0;

  // DEPERECATED: use Lattice class
  virtual const char* parse(const char *str)                = 0;
  virtual const Node* parseToNode(const char *str)          = 0;
  virtual const char* parseNBest(size_t N, const char *str) = 0;
  virtual bool  parseNBestInit(const char *str)             = 0;
  virtual const Node* nextNode()                            = 0;
  virtual const char* next()                                = 0;
  virtual const char* formatNode(const Node *node)          = 0;

#ifndef SWIG
  // DEPERECATED: use Lattice class
  virtual const char* parse(const char *str, size_t len, char *ostr, size_t olen) = 0;
  virtual const char* parse(const char *str, size_t len)                          = 0;
  virtual const Node* parseToNode(const char *str, size_t len)                    = 0;
  virtual const char* parseNBest(size_t N, const char *str, size_t len)           = 0;
  virtual bool  parseNBestInit(const char *str, size_t len)                       = 0;

  virtual const char* next(char *ostr , size_t olen)                        = 0;
  virtual const char* parseNBest(size_t N, const char *str,
                                 size_t len, char *ostr, size_t olen)       = 0;
  virtual const char* formatNode(const Node *node, char *ostr, size_t olen) = 0;
#endif

  // DEPERECATED: use Lattice::set_request_type()
  virtual void set_request_type(int request_type) = 0;
  virtual int  request_type() const = 0;

  // DEPERECATED: use Lattice::set_request_type()
  virtual bool  partial() const                             = 0;
  virtual void  set_partial(bool partial)                   = 0;
  virtual int   lattice_level() const                       = 0;
  virtual void  set_lattice_level(int level)                = 0;
  virtual bool  all_morphs() const                          = 0;
  virtual void  set_all_morphs(bool all_morphs)             = 0;

  // DEPERECATED: use Lattice::set_theta()
  virtual float theta() const                               = 0;
  virtual void  set_theta(float theta)                      = 0;

  // DEPERECATED: use Model::dictionary_info()
  virtual const DictionaryInfo* dictionary_info() const = 0;

  // DEPERECATED: use Model::what() and Lattice::what()
  virtual const char* what() const = 0;

  virtual ~Tagger() {}

#ifndef SIWG
  static Tagger *create(int argc, char **argv);
  static Tagger *create(const char *arg);
#endif

  static const char *version();
};

/* factory method */
MECAB_DLL_EXTERN Lattice     *createLattice();

MECAB_DLL_EXTERN Model       *createModel(int argc, char **argv);
MECAB_DLL_EXTERN Model       *createModel(const char *arg);
MECAB_DLL_EXTERN Tagger      *createTagger(int argc, char **argv);
MECAB_DLL_EXTERN Tagger      *createTagger(const char *arg);

MECAB_DLL_EXTERN void        deleteLattice(Lattice *lattice);
MECAB_DLL_EXTERN void        deleteModel(Model *model);
MECAB_DLL_EXTERN void        deleteTagger(Tagger *tagger);

MECAB_DLL_EXTERN const char*  getLastError();

// Keep it for backward compatibility.
// getTaggerError() is the same as getLastError()
MECAB_DLL_EXTERN const char*  getTaggerError();
}
#endif
#endif  /* MECAB_MECAB_H_ */
