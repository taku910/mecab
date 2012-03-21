#include <climits>
#include <string>
#include "common.h"
#include "tagger.h"
#include "decomposer.h"

namespace dual {
namespace {

class PointwiseWordScorer {
 public:
  PointwiseWordScorer(const std::vector<float> &score,
                      int weight)
      : score_(&score), weight_(-weight) {
    wscore_.resize(score_->size());
    wscore_[0] = -(*score_)[0];
    float prev = wscore_[0];

    for (size_t i = 1; i < score.size(); ++i) {
      if (score[i] != INVALID_COST) {
        wscore_[i] =  prev - (*score_)[i];
        prev = wscore_[i];
      } else {
        wscore_[i] = INVALID_COST;
      }
    }
  }

  int getWordScore(size_t begin, size_t mblen) const {
    if ((*score_)[begin] == INVALID_COST ||
        (*score_)[begin + mblen] == INVALID_COST) {
      return 0;
    }
    return static_cast<int>(
        weight_ * ((*score_)[begin] +
                   wscore_[begin + mblen] -
                   wscore_[begin] +
                   (*score_)[begin + mblen + 1]));
  }

  void findNextWordBoundary(size_t begin,
                            std::vector<size_t> *mblen) const {
    mblen->clear();
    for (size_t i = begin + 1; i < score_->size(); ++i) {
      if ((*score_)[i] != INVALID_COST) {
        mblen->push_back(i - begin);
        if ((*score_)[i] > 0.0) {
          return;
        }
      }
    }
  }

 private:
  const std::vector<float> *score_;
  std::vector<float> wscore_;
  int weight_;
};

void insertNewNode(size_t begin_pos, MeCab::Node *node,
                   MeCab::Lattice *lattice) {
  MeCab::Node **begin_nodes = lattice->begin_nodes();
  MeCab::Node **end_nodes = lattice->end_nodes();
  for (MeCab::Node *rnode = node; rnode != NULL;
       rnode = rnode->bnext) {
    const size_t end_pos = begin_pos + rnode->length;
    rnode->enext = end_nodes[end_pos];
    end_nodes[end_pos] = rnode;
  }

  if (begin_nodes[begin_pos] == NULL) {
    begin_nodes[begin_pos] = node;
  } else {
    for (MeCab::Node *rnode = node; rnode != NULL;
         rnode = rnode->bnext) {
      if (rnode->bnext == NULL) {
        rnode->bnext = begin_nodes[begin_pos];
        begin_nodes[begin_pos] = node;
        break;
      }
    }
  }
}

void propagatePointwiseWordScoreToLattice(
    const PointwiseWordScorer &scorer,
    MeCab::Lattice *lattice) {
  MeCab::Node **begin_nodes = lattice->begin_nodes();
  MeCab::Node **end_nodes = lattice->end_nodes();

  for (size_t pos = 0; pos < lattice->size(); ++pos) {
    for (MeCab::Node *rnode = begin_nodes[pos];
         rnode != NULL; rnode = rnode->bnext) {
      if (rnode->stat == MECAB_BOS_NODE ||
          rnode->stat == MECAB_EOS_NODE) {
        continue;
      }
      // add PointWise score to word lattice.
      rnode->wcost += scorer.getWordScore(pos, rnode->length);
    }
  }

  std::vector<size_t> mblens;
  for (size_t pos = 0; pos < lattice->size(); ++pos) {
    if (end_nodes[pos] == NULL) {
      continue;
    }
    scorer.findNextWordBoundary(pos, &mblens);
    for (size_t i = 0; i < mblens.size(); ++i) {
      bool found = false;
      const size_t mblen = mblens[i];
      for (const MeCab::Node *rnode = begin_nodes[pos];
           rnode; rnode = rnode->bnext) {
        if (rnode->length == mblen) {
          found = true;
          break;
        }
      }
      if (found) {
        continue;
      }
      MeCab::Node *new_node = lattice->newNode();
      new_node->surface = lattice->sentence() + pos;
      new_node->length = mblen;
      new_node->rlength = mblen;
      new_node->feature = "名詞,一般,*,*,*,*,*";
      new_node->wcost = 11651;
      new_node->lcAttr = 1285;
      new_node->rcAttr = 1285;
      // add PointWise score to word lattice.
      new_node->wcost += scorer.getWordScore(pos, new_node->length);
      insertNewNode(pos, new_node, lattice);
    }
  }
}
}  // namespace

bool Decomposer::viterbi(MeCab::Lattice *lattice) const {
  const size_t size = lattice->size();
  MeCab::Node **begin_nodes = lattice->begin_nodes();
  MeCab::Node **end_nodes = lattice->end_nodes();

  for (size_t pos = 0; pos <= size; ++pos) {
    for (MeCab::Node *rnode = begin_nodes[pos];
         rnode != NULL; rnode = rnode->bnext) {
      long best_cost = INT_MAX;
      MeCab::Node *best_node = NULL;
      for (MeCab::Node *lnode = end_nodes[pos];
           lnode != NULL; lnode = lnode->enext) {
        if (rnode == lnode) {
          continue;
        }
        const long cost = lnode->cost + getCost(lnode, rnode);
        if (cost < best_cost) {
          best_node = lnode;
          best_cost = cost;
        }
      }
      rnode->prev = best_node;
      rnode->next = 0;
      rnode->cost = best_cost;
    }
  }

  MeCab::Node *node = lattice->eos_node();
  MeCab::Node *prev = NULL;
  while (node->prev != NULL) {
    prev = node->prev;
    prev->next = node;
    node = prev;
  }

  return true;
}

Decomposer::Decomposer() : model_(0) {}

Decomposer::~Decomposer() {}

bool Decomposer::open(const MeCab::Model *model) {
  model_ = model;
  return true;
}

int Decomposer::getCost(const MeCab::Node *lnode,
                        const MeCab::Node *rnode) const {
  return model_->transition_cost(lnode->rcAttr,
                                 rnode->lcAttr) + rnode->wcost;
}

bool Decomposer::decompose(
    int weight,
    const std::vector<float> &score,
    MeCab::Lattice *lattice) const {
  PointwiseWordScorer scorer(score, weight);
  propagatePointwiseWordScoreToLattice(scorer, lattice);
  return viterbi(lattice);
}
}  // dual
