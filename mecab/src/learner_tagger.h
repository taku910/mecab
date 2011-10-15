//  MeCab -- Yet Another Part-of-Speech and Morphological Analyzer
//
//  $Id: learner_tagger.h 173 2009-04-18 08:10:57Z taku-ku $;
//
//  Copyright(C) 2001-2006 Taku Kudo <taku@chasen.org>
//  Copyright(C) 2004-2006 Nippon Telegraph and Telephone Corporation
#ifndef MECAB_TAGGER_H
#define MECAB_TAGGER_H

#include <vector>
#include "mecab.h"
#include "freelist.h"
#include "feature_index.h"
#include "tokenizer.h"
#include "scoped_ptr.h"

namespace MeCab {

class FeatureIndex;

class LearnerTagger {
 protected:
  LearnerTokenizer           *tokenizer_;
  FreeList<LearnerPath>      *path_freelist_;
  FeatureIndex               *feature_index_;
  scoped_string               begin__;
  const char                 *begin_;
  const char                 *end_;
  size_t                      len_;
  std::vector<LearnerNode *>  beginNodeList_;
  std::vector<LearnerNode *>  endNodeList_;
  whatlog                     what_;

  LearnerNode *lookup(size_t);
  bool connect(size_t, LearnerNode *);
  bool viterbi();
  bool buildLattice();
  bool initList();
 public:
  bool empty() const { return (len_ == 0); }
  void close() {}
  void clear() {}
  const char *what() { return what_.str(); }

  explicit LearnerTagger(): tokenizer_(0), path_freelist_(0),
                            feature_index_(0), begin_(0), end_(0), len_(0) {}
  virtual ~LearnerTagger() {}
};

class EncoderLearnerTagger: public LearnerTagger {
 private:
  size_t eval_size_;
  size_t unk_eval_size_;
  std::vector<LearnerPath *> ans_path_list_;

 public:
  bool open(LearnerTokenizer *tokenzier,
            FreeList<LearnerPath> *path_freelist,
            FeatureIndex *feature_index,
            size_t eval_size, size_t unk_eval_size);
  bool read(std::istream *, std::vector<double> *);
  int eval(size_t *, size_t *, size_t *) const;
  double gradient(double *expected);
  double online_update(double *expected);
  explicit EncoderLearnerTagger(): eval_size_(1024), unk_eval_size_(1024) {}
  virtual ~EncoderLearnerTagger() { close(); }
};

class DecoderLearnerTagger: public LearnerTagger {
 private:
  scoped_ptr<LearnerTokenizer> tokenizer__;
  scoped_ptr<FreeList<LearnerPath> > path_freelist__;
  scoped_ptr<FeatureIndex> feature_index__;
 public:
  bool open(const Param &);
  bool parse(std::istream *, std::ostream *);
  virtual ~DecoderLearnerTagger() { close(); }
};
}

#endif
