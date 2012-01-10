//  MeCab -- Yet Another Part-of-Speech and Morphological Analyzer
//
//
//  Copyright(C) 2001-2011 Taku Kudo <taku@chasen.org>
//  Copyright(C) 2004-2006 Nippon Telegraph and Telephone Corporation
namespace MeCab {
namespace {
#ifdef VITERBI_WITH_ALL_PATH_
inline bool connectWithAllPath(size_t pos, Node *rnode,
                               Node **begin_node_list,
                               Node **end_node_list,
                               const Connector *connector,
                               Allocator<Node, Path> *allocator)
#else
inline bool connect(size_t pos, Node *rnode,
                    Node **begin_node_list,
                    Node **end_node_list,
                    const Connector *connector,
                    Allocator<Node, Path> *allocator)
#endif
{
  for (;rnode; rnode = rnode->bnext) {
    register long best_cost = 2147483647;

    Node* best_node = 0;

    for (Node *lnode = end_node_list[pos]; lnode; lnode = lnode->enext) {
#ifdef VITERBI_WITH_ALL_PATH_
      register int  lcost = connector->cost(lnode, rnode);  // local cost
      register long cost  = lnode->cost + lcost;
#else
      register long cost  = lnode->cost + connector->cost(lnode, rnode);
#endif

      if (cost < best_cost) {
        best_node  = lnode;
        best_cost  = cost;
      }

#ifdef VITERBI_WITH_ALL_PATH_
      Path *path   = allocator->newPath();
      path->cost   = lcost;
      path->rnode  = rnode;
      path->lnode  = lnode;
      path->lnext  = rnode->lpath;
      rnode->lpath = path;
      path->rnext  = lnode->rpath;
      lnode->rpath = path;
#endif
    }

    // overflow check 2003/03/09
    if (!best_node) {
      return false;
    }

    rnode->prev = best_node;
    rnode->next = 0;
    rnode->cost = best_cost;
    const size_t x = rnode->rlength + pos;
    rnode->enext = end_node_list[x];
    end_node_list[x] = rnode;
  }

  return true;
}
}  // namespace

#ifdef VITERBI_WITH_ALL_PATH_
bool Viterbi::viterbiWithAllPath
#else
bool Viterbi::viterbi
#endif
(Lattice *lattice) const {
  Node **end_node_list   = lattice->end_nodes();
  Node **begin_node_list = lattice->begin_nodes();
  Allocator<Node, Path> *allocator = lattice->allocator();
  const bool partial  = lattice->has_request_type(MECAB_PARTIAL);
  const size_t len = lattice->size();
  const char *begin = lattice->sentence();
  const char *end = begin + len;

  Node *bos_node = tokenizer_->getBOSNode(lattice->allocator());
  bos_node->surface = lattice->sentence();
  end_node_list[0] = bos_node;

  for (size_t pos = 0; pos < len; ++pos) {
    if (end_node_list[pos]) {
      Node *right_node = tokenizer_->lookup(begin + pos, end, allocator);
      if (partial) {
        right_node = filterNode(begin_node_list[pos], right_node);
      }
      begin_node_list[pos] = right_node;
#ifdef VITERBI_WITH_ALL_PATH_
      if (!connectWithAllPath(pos, right_node,
                              begin_node_list,
                              end_node_list,
                              connector_.get(),
                              allocator)) {
        lattice->set_what("too long sentence.");
        return false;
      }
#else
      if (!connect(pos, right_node,
                   begin_node_list,
                   end_node_list,
                   connector_.get(),
                   allocator)) {
        lattice->set_what("too long sentence.");
        return false;
      }
#endif
    }
  }


  Node *eos_node = tokenizer_->getEOSNode(lattice->allocator());
  eos_node->surface = lattice->sentence() + lattice->size();
  begin_node_list[lattice->size()] = eos_node;

  for (long pos = len; static_cast<long>(pos) >= 0; --pos) {
    if (end_node_list[pos]) {
#ifdef VITERBI_WITH_ALL_PATH_
      if (!connectWithAllPath(pos, eos_node,
                              begin_node_list,
                              end_node_list,
                              connector_.get(),
                              allocator)) {
        lattice->set_what("too long sentence.");
        return false;
      }
#else
      if (!connect(pos, eos_node,
                   begin_node_list,
                   end_node_list,
                   connector_.get(),
                   allocator)) {
        lattice->set_what("too long sentence.");
        return false;
      }
#endif
      break;
    }
  }

  end_node_list[0] = bos_node;
  begin_node_list[lattice->size()] = eos_node;

  return true;
}
}  // Mecab
