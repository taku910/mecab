#ifndef DUAL_DECOMPOSER_H_
#define DUAL_DECOMPOSER_H_

#include <vector>
#include <mecab.h>
#include "tagger.h"

namespace dual {
class Decomposer {
 public:
  Decomposer();
  ~Decomposer();

  bool open(const MeCab::Model *model);
  bool decompose(
      int weight,
      const std::vector<float> &score,
      MeCab::Lattice *lattice) const;

 private:
  bool viterbi(MeCab::Lattice *lattice) const;
  int getCost(const MeCab::Node *lnode,
              const MeCab::Node *rnode) const;

  const MeCab::Model *model_;
};
}  // dual
#endif
