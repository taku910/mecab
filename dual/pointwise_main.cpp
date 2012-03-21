#include <iostream>
#include <vector>
#include <string>
#include <unistd.h>
#include <stdlib.h>
#include "tagger.h"
#include "decomposer.h"
#include "mecab.h"

namespace {
void OutputPointwise(const char *str, size_t len,
                     const std::vector<float> &score,
                     std::ostream *os) {
  size_t prev = 0;
  for (size_t i = 1; i < score.size(); ++i) {
    if (score[i] > 0.0) {
      os->write(str + prev, i - prev);
      *os << "\t*\n";
      prev = i;
    }
  }
  os->write(str + prev, len - prev);
  *os << "EOS\n";
}

void OutputLattice(const MeCab::Lattice *lattice,
                   std::ostream *os) {
  for (const MeCab::Node *node = lattice->bos_node();
       node; node = node->next) {
    if (node->stat == MECAB_BOS_NODE || node->stat == MECAB_EOS_NODE) {
      continue;
    }
    os->write(node->surface, node->length);
    *os << '\t' << node->feature
        << '\t' << -node->wcost << '\n';
  }
  *os << "EOS" << std::endl;
}
}

int main(int argc, char **argv) {
  extern char *optarg;
  int weight = 500;
  bool learn = false;

  std::string crfpp_param;
  std::string train_file = "train";
  std::string pointwise_model_file = "model";
  std::string dicdir = "./dic";

  const std::string arg = std::string("-d ") + dicdir;
  MeCab::Model *mecab_model = MeCab::createModel(arg.c_str());
  if (!mecab_model) {
    std::cerr << MeCab::getLastError() << std::endl;
    return -1;
  }

  MeCab::Tagger *mecab_tagger = mecab_model->createTagger();

  int opt;
  while ((opt = getopt(argc, argv, "m:t:lp:d:w:")) != -1) {
    switch(opt) {
      case 'l':
        learn = true;
        break;
      case 'm':
        pointwise_model_file = std::string(optarg);
        break;
      case 't':
        train_file = std::string(optarg);
        break;
      case 'w':
        weight = atoi(optarg);
        break;
      case 'd':
        dicdir = std::string(optarg);
        break;
      case 'p':
        crfpp_param = std::string(optarg);
        break;
      default:
        std::cerr << "unknown parameter" << std::endl;
        return -1;
    }
  }

  if (weight < 0) {
    std::cout << "weight must be positive value";
    return -1;
  }

  if (learn) {
    return dual::PointwiseTagger::learn(train_file.c_str(),
                                        pointwise_model_file.c_str(),
                                        crfpp_param.c_str());
  }

  dual::PointwiseTagger pointwise_tagger;
  dual::Decomposer decomposer;

  if (!pointwise_tagger.open(pointwise_model_file.c_str()) ||
      !decomposer.open(mecab_model)) {
    std::cerr << "Usage: " << argv[0]
              << " -m model -d dicdir < files .."  << std::endl;
    return -1;
  }

  std::string line;
  MeCab::Lattice *lattice = MeCab::createLattice();

  while (std::getline(std::cin, line)) {
    if (line.empty()) {
      continue;
    }
    std::vector<float> score;
    if (!pointwise_tagger.parse(line.data(), line.size(), &score)) {
      return -1;
    }

    OutputPointwise(line.data(), line.size(), score, &std::cout);
    lattice->set_sentence(line.data(), line.size());
    mecab_tagger->parse(lattice);
    OutputLattice(lattice, &std::cout);
    decomposer.decompose(weight, score, lattice);
    OutputLattice(lattice, &std::cout);
  }

  delete lattice;
  delete mecab_tagger;
  delete mecab_model;

  return 0;
}
