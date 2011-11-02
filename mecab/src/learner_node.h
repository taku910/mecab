//  MeCab -- Yet Another Part-of-Speech and Morphological Analyzer
//
//
//  Copyright(C) 2001-2006 Taku Kudo <taku@chasen.org>
//  Copyright(C) 2004-2006 Nippon Telegraph and Telephone Corporation
#ifndef MECAB_LEARNER_NODE_H
#define MECAB_LEARNER_NODE_H

#include <cstring>
#include "mecab.h"
#include "common.h"
#include "utils.h"

namespace MeCab {

template <class T1, class T2> T1 repeat_find_if(T1 b, T1 e,
                                                const T2& v, size_t n) {
  T1 r = b;
  for (size_t i = 0; i < n; ++i) {
    r = std::find(b, e, v);
    if (r == e) return e;
    b = r + 1;
  }
  return r;
}

// NOTE: first argment:   answer,
//       second argment:  system output
inline bool node_cmp_eq(const LearnerNode &node1,
                        const LearnerNode &node2,
                        size_t size, size_t unk_size) {
  if (node1.length == node2.length &&
      strncmp(node1.surface, node2.surface, node1.length) == 0) {
    const char *p1 = node1.feature;
    const char *p2 = node2.feature;
    // There is NO case when node1 becomes MECAB_UNK_NODE
    if (node2.stat == MECAB_UNK_NODE)
      size = unk_size;  // system cannot output other extra information
    const char *r1 = repeat_find_if(p1, p1 + std::strlen(p1), ',', size);
    const char *r2 = repeat_find_if(p2, p2 + std::strlen(p2), ',', size);
    if (static_cast<size_t>(r1 - p1) == static_cast<size_t>(r2 - p2) &&
        std::strncmp(p1, p2, static_cast<size_t>(r1 - p1)) == 0) {
      return true;
    }
  }

  return false;
}

inline bool is_empty(LearnerPath *path) {
  return((!path->rnode->rpath && path->rnode->stat != MECAB_EOS_NODE) ||
         (!path->lnode->lpath && path->lnode->stat != MECAB_BOS_NODE) );
}

inline void calc_expectation(LearnerPath *path, double *expected, double Z) {
  if ( is_empty(path) ) return;

  double c = std::exp(path->lnode->alpha +
                      path->cost +
                      path->rnode->beta - Z);

  for (const int *f = path->fvector; *f != -1; ++f) {
    expected[*f] += c;
  }

  if (path->rnode->stat != MECAB_EOS_NODE) {
    for (const int *f = path->rnode->fvector; *f != -1; ++f) {
      expected[*f] += c;
    }
  }
}

inline void calc_online_update(LearnerPath *path, double *expected) {
  if ( is_empty(path) ) return;

  for (const int *f = path->fvector; *f != -1; ++f) {
    expected[*f] += 1.0;
  }

  if (path->rnode->stat != MECAB_EOS_NODE) {
    for (const int *f = path->rnode->fvector; *f != -1; ++f) {
      expected[*f] += 1.0;
    }
  }
}


inline void calc_alpha(LearnerNode *n) {
  n->alpha = 0.0;
  for (LearnerPath *path = n->lpath; path; path = path->lnext)
    n->alpha = logsumexp(n->alpha,
                         path->cost + path->lnode->alpha,
                         path == n->lpath);
}

inline void calc_beta(LearnerNode *n) {
  n->beta = 0.0;
  for (LearnerPath *path = n->rpath; path; path = path->rnext)
    n->beta = logsumexp(n->beta,
                        path->cost + path->rnode->beta,
                        path == n->rpath);
}
}

#endif
