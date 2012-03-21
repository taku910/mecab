#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include "common.h"
#include "crfpp.h"
#include "ucs.h"
#include "tagger.h"
#include "utils.h"

namespace {
const char kCrfppTemplate[] =
    "U10:%x[-2,0]\n"
    "U11:%x[-1,0]\n"
    "U12:%x[0,0]\n"
    "U13:%x[1,0]\n"
    "U14:%x[2,0]\n"
    "U20:%x[-2,0]/%x[-1,0]\n"
    "U21:%x[-1,0]/%x[0,0]\n"
    "U21:%x[0,0]/%x[1,0]\n"     
    "U22:%x[1,0]/%x[2,0]\n"
    "U30:%x[-2,0]/%x[-1,0]/%x[0,0]\n"
    "U31:%x[-1,0]/%x[0,0]/%x[1,0]\n"
    "U32:%x[0,0]/%x[1,0]/%x[2,0]\n"
    "U40:%x[-2,1]\n"
    "U41:%x[-1,1]\n"
    "U42:%x[0,1]\n"
    "U43:%x[1,1]\n"
    "U44:%x[2,1]\n"
    "U50:%x[-2,1]/%x[-1,1]\n"
    "U51:%x[-1,1]/%x[0,1]\n"
    "U51:%x[0,1]/%x[1,1]\n"     
    "U52:%x[1,1]/%x[2,1]\n"
    "U60:%x[-2,1]/%x[-1,1]/%x[0,1]\n"
    "U61:%x[-1,1]/%x[0,1]/%x[1,1]\n"
    "U62:%x[0,1]/%x[1,1]/%x[2,1]\n";
//    "B\n";
}

namespace dual {

PointwiseTagger::PointwiseTagger()
    : tagger_(0) {}

PointwiseTagger::~PointwiseTagger() {
  close();
}

bool PointwiseTagger::open(const char *model) {
  std::string arg = "-m ";
  arg += model;
  tagger_ = CRFPP::createTagger(arg.c_str());
  if (!tagger_) {
    std::cerr << CRFPP::getTaggerError() << std::endl;
    return false;
  }
  return true;
}

void PointwiseTagger::close() {
  delete tagger_;
  tagger_ = 0;
}

bool PointwiseTagger::parse(const char *line,
                            size_t len,
                            std::vector<float> *score) {
  if (!tagger_ || !score) {
    return false;
  }

  if (len == 0) {
    return true;
  }

  score->clear();
  tagger_->clear();

  const char *begin = line;
  const char *end = line + len;

  char buf[32];
  while (begin < end) {
    size_t mblen = 0;
    const char c = utf8_to_ucs2(begin, end, &mblen);
    std::memcpy(buf, begin, mblen);
    if (buf[0] == ' ' || buf[0] == '\t') {
      buf[0] = '_';
    }
    buf[mblen] = '\t';
    buf[mblen + 1] = c;
    buf[mblen + 2] = '\0';
    tagger_->add(buf);
    begin += mblen;
  }

  if (!tagger_->parse()) {
    std::cerr << tagger_->what() << std::endl;
    return false;
  }

  for (size_t i = 0; i < tagger_->size(); ++i) {
    const size_t mblen = std::strlen(tagger_->x(i, 0));
    // 0.0 is reserved for INVALID_COST, so add
    // VERY_SMALL_COST just in case.
    score->push_back(tagger_->emission_cost(i, 0) +
                     VERY_SMALL_COST);
    for (size_t j = 1; j < mblen; ++j) {
      score->push_back(INVALID_COST);
    }
  }
  score->push_back(VERY_SMALL_COST);

  return true;
}

bool PointwiseTagger::learn(const char *train_file,
                            const char *model_file,
                            const char *crfpp_param) {
  std::string tmp_train_file = model_file;
  {
    std::ifstream ifs(train_file);
    CHECK_DIE(ifs) << "no such file or directory: " << train_file;

    // make training data
    {
      tmp_train_file += ".crfpp";
      std::ofstream ofs(tmp_train_file.c_str());
      CHECK_DIE(ofs) << "permission denied: " << tmp_train_file;
      std::cout << "reading training data: " << std::flush;

      char line[BUF_SIZE];
      std::vector<std::string> tokens;
      size_t num_sentenes = 0;
      while (ifs.getline(line, sizeof(line))) {
        // read MeCab format
        if (strcmp(line, "EOS") == 0) {
          if (num_sentenes++ % 1000 == 0) {
            std::cout << num_sentenes << " " << std::flush;
          }
          for (size_t i = 0; i < tokens.size(); ++i) {
            const size_t len = tokens[i].size();
            const char *begin = tokens[i].c_str();
            const char *end = tokens[i].c_str() + len;
            size_t mblen = 0;
            int num = 0;
            while (begin < end) {
              const char c = dual::utf8_to_ucs2(begin, end, &mblen);
              ofs.write(begin, mblen);
              ofs << '\t' << c;
              ofs << '\t' << (num == 0 ? 'B' : 'I') << std::endl;
              ++num;
              begin += mblen;
            }
          }
          ofs << std::endl;
          tokens.clear();
        } else {
          std::vector<char *> cols;
          const size_t kMaxColsize = 2;
          const size_t size = tokenize2(line, " \t",
                                        std::back_inserter(cols),
                                        kMaxColsize);
          CHECK_DIE(cols.size() == kMaxColsize);
          CHECK_DIE(cols.size() == size);
          tokens.push_back(cols[0]);
        }
      }
    }

    // call CRF++
    {
      std::string templ_file = model_file;
      templ_file += ".templ";
      {
        std::ofstream ofs(templ_file.c_str());
        CHECK_DIE(ofs) << "permission denied: " << templ_file;
        ofs << kCrfppTemplate;
      }

      std::vector<char *> argv;
      argv.push_back(const_cast<char *>("mozi_learn"));
      argv.push_back(const_cast<char *>(templ_file.c_str()));
      argv.push_back(const_cast<char *>(tmp_train_file.c_str()));
      argv.push_back(const_cast<char *>(model_file));

      char tmp[BUF_SIZE];
      std::strncpy(tmp, crfpp_param, BUF_SIZE);
      tokenize2(tmp, "\t ", std::back_inserter(argv), 128);
      argv.push_back(const_cast<char *>("-t"));

      CHECK_DIE(0 ==
                crfpp_learn(static_cast<int>(argv.size()),
                            &argv[0]))
          << "crfpp_learn execution error";

      ::unlink(tmp_train_file.c_str());
      ::unlink(templ_file.c_str());
    }

    return true;
  }
}
}  // dual
