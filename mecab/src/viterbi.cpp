// MeCab -- Yet Another Part-of-Speech and Morphological Analyzer
//
//
//  Copyright(C) 2001-2011 Taku Kudo <taku@chasen.org>
//  Copyright(C) 2004-2006 Nippon Telegraph and Telephone Corporation
#include <algorithm>
#include <iterator>
#include <cmath>
#include <cstring>
#include "common.h"
#include "connector.h"
#include "mecab.h"
#include "nbest_generator.h"
#include "param.h"
#include "viterbi.h"
#include "scoped_ptr.h"
#include "string_buffer.h"
#include "tokenizer.h"

namespace MeCab {

namespace {
void calc_alpha(Node *n, double beta) {
  n->alpha = 0.0;
  for (Path *path = n->lpath; path; path = path->lnext) {
    n->alpha = logsumexp(n->alpha,
                         -beta * path->cost + path->lnode->alpha,
                         path == n->lpath);
  }
}

void calc_beta(Node *n, double beta) {
  n->beta = 0.0;
  for (Path *path = n->rpath; path; path = path->rnext) {
    n->beta = logsumexp(n->beta,
                        -beta * path->cost + path->rnode->beta,
                        path == n->rpath);
  }
}

bool partial_match(const char *f1, const char *f2) {
  scoped_fixed_array<char, BUF_SIZE> buf1;
  scoped_fixed_array<char, BUF_SIZE> buf2;
  scoped_fixed_array<char *, 64> c1;
  scoped_fixed_array<char *, 64> c2;

  std::strncpy(buf1.get(), f1, buf1.size());
  std::strncpy(buf2.get(), f2, buf2.size());

  const size_t n1 = MeCab::tokenizeCSV(buf1.get(), c1.get(), c1.size());
  const size_t n2 = MeCab::tokenizeCSV(buf2.get(), c2.get(), c2.size());
  const size_t n  = std::min(n1, n2);

  for (size_t i = 0; i < n; ++i) {
    if (std::strcmp(c1[i], "*") != 0 &&
        std::strcmp(c1[i], c2[i]) != 0) {
      return false;
    }
  }
  return true;
}
}  // namespace

Viterbi::Viterbi()
    :  tokenizer_(0), connector_(0),
       cost_factor_(0) {}

Viterbi::~Viterbi() {}

bool Viterbi::open(const Param &param) {
  tokenizer_.reset(new Tokenizer<Node, Path>);
  CHECK_FALSE(tokenizer_->open(param)) << tokenizer_->what();
  CHECK_FALSE(tokenizer_->dictionary_info()) << "Dictionary is empty";

  connector_.reset(new Connector);
  CHECK_FALSE(connector_->open(param)) << connector_->what();

  CHECK_FALSE(tokenizer_->dictionary_info()->lsize ==
              connector_->left_size() &&
              tokenizer_->dictionary_info()->rsize ==
              connector_->right_size())
      << "Transition table and dictionary are not compatible";

  cost_factor_ = param.get<int>("cost-factor");
  if (cost_factor_ == 0) {
    cost_factor_ = 800;
  }

  return true;
}

bool Viterbi::analyze(Lattice *lattice) const {
  if (!lattice || !lattice->sentence()) {
    return false;
  }

  if (!initPartial(lattice)) {
    return false;
  }

  if (lattice->has_request_type(MECAB_NBEST) ||
      lattice->has_request_type(MECAB_MARGINAL_PROB)) {
    if (!viterbiWithAllPath(lattice)) {
      return false;
    }
  } else {
    if (!viterbi(lattice)) {
      return false;
    }
  }

  if (!forwardbackward(lattice)) {
    return false;
  }

  if (!buildBestLattice(lattice)) {
    return false;
  }

  if (!buildAllLattice(lattice)) {
    return false;
  }

  if (!initNBest(lattice)) {
    return false;
  }

  return true;
}

const Tokenizer<Node, Path> *Viterbi::tokenizer() const {
  return tokenizer_.get();
}

const Connector *Viterbi::connector() const {
  return connector_.get();
}

// static
bool Viterbi::forwardbackward(Lattice *lattice) {
  if (!lattice->has_request_type(MECAB_MARGINAL_PROB)) {
    return true;
  }

  Node **end_node_list   = lattice->end_nodes();
  Node **begin_node_list = lattice->begin_nodes();

  const size_t len = lattice->size();
  const double theta = lattice->theta();

  end_node_list[0]->alpha = 0.0;
  for (int pos = 0; pos <= static_cast<long>(len); ++pos) {
    for (Node *node = begin_node_list[pos]; node; node = node->bnext) {
      calc_alpha(node, theta);
    }
  }

  begin_node_list[len]->beta = 0.0;
  for (int pos = static_cast<long>(len); pos >= 0; --pos) {
    for (Node *node = end_node_list[pos]; node; node = node->enext) {
      calc_beta(node, theta);
    }
  }

  const double Z = begin_node_list[len]->alpha;
  lattice->set_Z(Z);  // alpha of EOS

  for (int pos = 0; pos <= static_cast<long>(len); ++pos) {
    for (Node *node = begin_node_list[pos]; node; node = node->bnext) {
      node->prob = std::exp(node->alpha + node->beta - Z);
      for (Path *path = node->lpath; path; path = path->lnext) {
        path->prob = std::exp(path->lnode->alpha
                              - theta * path->cost
                              + path->rnode->beta - Z);
      }
    }
  }

  return true;
}

// static
bool Viterbi::buildResultForNBest(Lattice *lattice) {
  return buildAllLattice(lattice);
}

// static
bool Viterbi::buildAllLattice(Lattice *lattice) {
  if (!lattice->has_request_type(MECAB_ALL_MORPHS)) {
    return true;
  }

  Node *prev = lattice->bos_node();
  const size_t len = lattice->size();
  Node **begin_node_list = lattice->begin_nodes();

  for (long pos = 0; pos <= static_cast<long>(len); ++pos) {
    for (Node *node = begin_node_list[pos]; node; node = node->bnext) {
      prev->next = node;
      node->prev = prev;
      prev = node;
    }
  }

  return true;
}

// static
bool Viterbi::buildBestLattice(Lattice *lattice) {
  Node *node = lattice->eos_node();
  for (Node *prev_node; node->prev;) {
    node->isbest = 1;
    prev_node = node->prev;
    prev_node->next = node;
    node = prev_node;
  }

  return true;
}

// static
bool Viterbi::initNBest(Lattice *lattice) {
  if (!lattice->has_request_type(MECAB_NBEST)) {
    return true;
  }
  lattice->allocator()->nbest_generator()->set(lattice);
  return true;
}

// static
bool Viterbi::initPartial(Lattice *lattice) {
  if (!lattice->has_request_type(MECAB_PARTIAL)) {
    if (lattice->has_constraint()) {
      lattice->set_boundary_constraint(0, MECAB_TOKEN_BOUNDARY);
      lattice->set_boundary_constraint(lattice->size(),
                                       MECAB_TOKEN_BOUNDARY);
    }
    return true;
  }

  Allocator<Node, Path> *allocator = lattice->allocator();
  char *str = allocator->partial_buffer(lattice->size() + 1);
  strncpy(str, lattice->sentence(), lattice->size() + 1);

  std::vector<char *> lines;
  const size_t lsize = tokenize(str, "\n",
                                std::back_inserter(lines),
                                lattice->size() + 1);
  char* column[2];
  scoped_array<char> buf(new char[lattice->size() + 1]);
  StringBuffer os(buf.get(), lattice->size() + 1);

  std::vector<std::pair<char *, char *> > tokens;
  tokens.reserve(lsize);

  size_t pos = 0;
  for (size_t i = 0; i < lsize; ++i) {
    const size_t size = tokenize(lines[i], "\t", column, 2);
    if (size == 1 && std::strcmp(column[0], "EOS") == 0) {
      break;
    }
    const size_t len = std::strlen(column[0]);
    if (size == 2) {
      tokens.push_back(std::make_pair(column[0], column[1]));
    } else {
      tokens.push_back(std::make_pair(column[0], reinterpret_cast<char *>(0)));
    }
    os << column[0];
    pos += len;
  }

  os << '\0';

  lattice->set_sentence(os.str());

  pos = 0;
  for (size_t i = 0; i < tokens.size(); ++i) {
    const char *surface = tokens[i].first;
    const char *feature = tokens[i].second;
    const size_t len = std::strlen(surface);
    lattice->set_boundary_constraint(pos, MECAB_TOKEN_BOUNDARY);
    lattice->set_boundary_constraint(pos + len, MECAB_TOKEN_BOUNDARY);
    if (feature) {
      lattice->set_feature_constraint(pos, pos + len, feature);
      for (size_t n = 1; n < len; ++n) {
        lattice->set_boundary_constraint(pos + n,
                                         MECAB_INSIDE_TOKEN);
      }
    }
    pos += len;
  }

  return true;
}

namespace {
inline Node *filterNodeInternal(Lattice *lattice,
                                Node *node,
                                const char *feature,
                                size_t pos,
                                size_t next_strong_boundary_pos) {
  Node *prev = 0;
  Node *result = 0;

  for (Node *n = node; n; n = n->bnext) {
    if (n->stat == MECAB_UNK_NODE &&
        pos + n->rlength > next_strong_boundary_pos) {
      const size_t diff = pos + n->rlength - next_strong_boundary_pos;
      n->rlength -= diff;
      n->length -= diff;
    }
    if (pos + n->rlength <= next_strong_boundary_pos &&
        MECAB_INSIDE_TOKEN !=
        lattice->boundary_constraint(pos + n->rlength) &&
        (!feature || std::strcmp(feature, "*") == 0 ||
         partial_match(feature, n->feature))) {
      if (prev) {
        prev->bnext = n;
        prev = n;
      } else {
        result = n;
        prev = result;
      }
    }
  }

  if (prev) {
    prev->bnext = 0;
  }

  return result;
}
}  // namespace

// static
Node *Viterbi::filterNode(Lattice *lattice, Node *node, size_t pos) const {
  // |next_strong_boundary_pos| is the position of strong boundary where
  // any words cannot be longer than |next_strong_boundary_pos - pos|.
  // |next_strong_boundary_pos| is a possible and shortest
  // word which can satisfy the boundary constraints.
  // TODO(taku): This part is potentially O(n^2).
  size_t next_strong_boundary_pos = lattice->size();
  size_t next_weak_boundary_pos = lattice->size();
  for (size_t i = pos + 1; i <= lattice->size(); ++i) {
    const int b = lattice->boundary_constraint(i);
    if (MECAB_INSIDE_TOKEN != b) {
      next_weak_boundary_pos = i;
    }
    if (MECAB_TOKEN_BOUNDARY == b) {
      next_strong_boundary_pos = i;
      break;
    }
  }

  const char *feature = lattice->feature_constraint(pos);

  Node *result = 0;
  result = filterNodeInternal(lattice, node, feature,
                              pos, next_strong_boundary_pos);

  if (result) {
    return result;
  }

  node = tokenizer_->getUnknownNode(
      lattice->sentence() + pos,
      lattice->sentence() + next_weak_boundary_pos,
      lattice->allocator());
  result = filterNodeInternal(lattice, node, feature,
                              pos, next_strong_boundary_pos);

  if (result) {
    return result;
  }

  node = lattice->newNode();
  node->surface = lattice->sentence() + pos;
  node->feature = feature;
  node->length = next_weak_boundary_pos - pos;
  node->rlength = next_weak_boundary_pos - pos;
  node->bnext = 0;
  node->wcost = 0;

  result = node;

  return result;
}
}  // MeCab

#undef VITERBI_WITH_ALL_PATH_
#include "viterbisub.h"
#define VITERBI_WITH_ALL_PATH_
#include "viterbisub.h"
