//  MeCab -- Yet Another Part-of-Speech and Morphological Analyzer
//
//  $Id: feature_index.h 173 2009-04-18 08:10:57Z taku-ku $;
//
//  Copyright(C) 2001-2006 Taku Kudo <taku@chasen.org>
//  Copyright(C) 2004-2006 Nippon Telegraph and Telephone Corporation
#ifndef MECAB_FEATUREINDEX_H
#define MECAB_FEATUREINDEX_H

#include <map>
#include <vector>
#include "mecab.h"
#include "mmap.h"
#include "darts.h"
#include "freelist.h"
#include "common.h"
#include "string_buffer.h"
#include "dictionary_rewriter.h"

namespace MeCab {

class Param;

class FeatureIndex {
 protected:
  std::vector<int>     feature_;
  ChunkFreeList<int>   feature_freelist_;
  ChunkFreeList<char>  char_freelist_;
  std::vector<const char*>   unigram_templs_;
  std::vector<const char*>   bigram_templs_;
  DictionaryRewriter   rewrite_;
  StringBuffer         os_;
  size_t               maxid_;
  const double         *alpha_;
  whatlog              what_;

  virtual int id(const char *) = 0;
  const char* getIndex(char **, char **, size_t);
  bool openTemplate(const Param &);

 public:
  virtual bool open(const Param &) = 0;
  virtual void clear() = 0;
  virtual void close() = 0;
  virtual bool buildFeature(LearnerPath *path) = 0;

  void set_alpha(const double *);

  size_t size() const { return maxid_; }

  bool buildUnigramFeature(LearnerPath *, const char*);
  bool buildBigramFeature(LearnerPath *, const char *, const char*);

  void calcCost(LearnerPath *path);
  void calcCost(LearnerNode *node);

  const char *what() { return what_.str(); }

  const char *strdup(const char *str);

  static bool convert(const char *text_filename,
                      const char *binary_filename);

  explicit FeatureIndex(): feature_freelist_(8192 * 32),
                           char_freelist_(8192 * 32),
                           maxid_(0), alpha_(0) {}
  virtual ~FeatureIndex() {}
};

class EncoderFeatureIndex: public FeatureIndex {
 private:
  std::map<std::string, int> dic_;
  std::map<std::string, std::pair<const int*, size_t> > feature_cache_;
  int id(const char *key);

 public:
  bool open(const Param &param);
  void close();
  void clear();

  bool save(const char *filename);
  void shrink(size_t, std::vector<double> *) ;
  bool buildFeature(LearnerPath *path);
  void clearcache();
};

class DecoderFeatureIndex: public FeatureIndex {
 private:
  Mmap<char>         mmap_;
  Darts::DoubleArray da_;
  int id(const char *key);
 public:
  bool open(const Param &param);
  void clear();
  void close();
  bool buildFeature(LearnerPath *path);
};
}

#endif
