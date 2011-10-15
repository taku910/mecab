//  MeCab -- Yet Another Part-of-Speech and Morphological Analyzer
//
//  $Id: learner.cpp 173 2009-04-18 08:10:57Z taku-ku $;
//
//  Copyright(C) 2001-2006 Taku Kudo <taku@chasen.org>
//  Copyright(C) 2004-2006 Nippon Telegraph and Telephone Corporation
#include <vector>
#include <string>
#include <fstream>
#include "param.h"
#include "common.h"
#include "lbfgs.h"
#include "utils.h"
#include "thread.h"
#include "learner_tagger.h"
#include "freelist.h"
#include "feature_index.h"
#include "string_buffer.h"

namespace {
double toLogProb(double f1, double f2) {
  return std::log(1.0 * f1 / f2) - VERY_SMALL_LOGPROB;  // avoid 0
}
}

namespace MeCab {

#define DCONF(file) create_filename(dicdir, std::string(file)).c_str()

class HMMLearner {
 public:
  static int run(Param *param) {
    DictionaryRewriter rewrite;


    const std::string dicdir = param->get<std::string>("dicdir");
    CHECK_DIE(param->load(DCONF(DICRC)))
        << "no such file or directory: " << DCONF(DICRC);

    CHECK_DIE(rewrite.open(DCONF(REWRITE_FILE)))
        << "no such file or directory: " << DCONF(REWRITE_FILE);

    const std::vector<std::string> files = param->rest_args();
    if (files.size() != 2) {
      std::cout << "Usage: " <<
          param->program_name() << " corpus model" << std::endl;
      return -1;
    }

    const std::string ifile = files[0];
    const std::string model = files[1];

    const bool text_only = param->get<bool>("text-only");
    const bool em_hmm = param->get<bool>("em-hmm");
    const std::string bos_feature = param->get<std::string>("bos-feature");

    CHECK_DIE(!bos_feature.empty()) << "bos-feature is empty";

    char line[BUF_SIZE];
    char *col[8];
    std::string word, feature;
    std::string ufeature, lfeature, rfeature;
    std::string plfeature, prfeature;
    std::map<std::string, std::map<std::string, double> > emission;
    std::map<std::string, std::map<std::string, double> > transition;

    // corpus
    if (em_hmm) {
      std::ifstream ifs(ifile.c_str());
      CHECK_DIE(ifs) << "no such file or directory: " << ifile;
      size_t size = 0;
      while (ifs.getline(line, sizeof(line))) {
        if (std::strcmp("EOS", line) == 0) {
          if (++size % 100 == 0)
            std::cout << size << "... " << std::flush;
          continue;
        }

        CHECK_DIE(tokenize(line, "\t", col, 4) == 4)
            << "format error\n";
        CHECK_DIE(std::strcmp("B", col[0]) == 0 &&
                  std::strcmp("U", col[0]) == 0)
            << "format error\n";
        if (col[0][0] == 'B') {  // bigram
          feature = col[1];
          CHECK_DIE(rewrite.rewrite(feature,
                                    &ufeature,
                                    &lfeature,
                                    &rfeature))
              << "rewrite failed";
          prfeature = rfeature;
          feature = col[2];
          CHECK_DIE(rewrite.rewrite(feature,
                                    &ufeature,
                                    &lfeature,
                                    &rfeature))
              << "rewrite failed";
          plfeature = lfeature;
          transition[prfeature][plfeature] += std::atof(col[3]);
        } else {   // unigram
          feature = col[2];
          CHECK_DIE(rewrite.rewrite(feature,
                                    &ufeature,
                                    &lfeature,
                                    &rfeature))
              << "rewrite failed";
          std::strncpy(line, ufeature.c_str(), sizeof(line));
          size_t n = tokenize2(line, "\t ", col, 2);
          CHECK_DIE(n == 2) << "format error in rewrite.def: " << ufeature;
          ufeature = col[0];
          word = col[1];
          emission[ufeature][word] += atof(col[3]);
        }
        ++size;
      }
    } else {
      std::ifstream ifs(ifile.c_str());
      CHECK_DIE(ifs) << "no such file or directory: " << ifile;

      CHECK_DIE(rewrite.rewrite(bos_feature,
                                &ufeature,
                                &plfeature,
                                &prfeature)) << "rewrite failed";

      size_t size = 0;
      while (ifs.getline(line, sizeof(line))) {
        if (std::strcmp("EOS", line) == 0) {
          if (++size % 100 == 0)
            std::cout << size << "... " << std::flush;
          feature = bos_feature;
        } else {
          CHECK_DIE(tokenize(line, "\t", col, 2) == 2)
              << "format error\n";
          feature = col[1];
        }

        CHECK_DIE(rewrite.rewrite(feature,
                                  &ufeature,
                                  &lfeature,
                                  &rfeature))
            << "rewrite failed";

        std::strncpy(line, ufeature.c_str(), sizeof(line));
        // unigram rule must contain ' '
        const size_t n = tokenize2(line, "\t ", col, 2);
        CHECK_DIE(n == 2) << "format error in rewrite.def: " << ufeature;
        ufeature = col[0];
        word = col[1];
        transition[prfeature][lfeature] += 1.0;
        emission[ufeature][word] += 1.0;
        plfeature = lfeature;
        prfeature = rfeature;
      }
    }

    // dictionary
    {
      std::vector<std::string> dic;
      enum_csv_dictionaries(dicdir.c_str(), &dic);

      const double freq = param->get<double>("default-emission-freq");
      CHECK_DIE(freq >= 0.0) << " default-emission-freq must be >= 0 "
                             << freq;

      for (std::vector<std::string>::const_iterator it = dic.begin();
           it != dic.end(); ++it) {
        std::cout << "reading " << *it << " ... " << std::flush;

        std::ifstream ifs(it->c_str());
        CHECK_DIE(ifs) << "no such file or directory: " << *it;

        while (ifs.getline(line, sizeof(line))) {
          CHECK_DIE(tokenizeCSV(line, col, 5) == 5) << "format error";
          feature = col[4];
          CHECK_DIE(rewrite.rewrite(feature,
                                    &ufeature,
                                    &lfeature,
                                    &rfeature)) << "rewrite failed";
          std::strncpy(line, ufeature.c_str(), sizeof(line));
          const size_t n = tokenize2(line, "\t ", col, 2);
          CHECK_DIE(n == 2) << "format error: " << ufeature;
          ufeature = col[0];
          word = col[1];
          emission[ufeature][word] += freq;
        }

        std::cout << std::endl;
      }
    }

    {
      std::cout << std::endl;
      std::string txtfile = model;
      txtfile += ".txt";

      std::ofstream ofs(txtfile.c_str());
      CHECK_DIE(ofs) << "permission denied: " << model;

      ofs.setf(std::ios::fixed, std::ios::floatfield);
      ofs.precision(24);

      // bigram
      for (std::map<std::string, std::map<std::string, double> >
               ::const_iterator
               it = transition.begin();
           it != transition.end(); ++it) {
        double freq = 0.0;
        for (std::map<std::string, double>::
                 const_iterator it2 = it->second.begin();
             it2 != it->second.end(); ++it2) {
          freq += it2->second;
        }

        for (std::map<std::string, double>
                 ::const_iterator it2 = it->second.begin();
             it2 != it->second.end(); ++it2)
          ofs << toLogProb(it2->second, freq) << '\t'
              << 'B' << ':' << it->first << '/' << it2->first << std::endl;
      }

      // unigram
      for (std::map<std::string, std::map<std::string, double> >
               ::const_iterator
               it = emission.begin();
           it != emission.end(); ++it) {
        double freq = 0.0;
        for (std::map<std::string, double>
                 ::const_iterator it2 = it->second.begin();
             it2 != it->second.end(); ++it2)
          freq += it2->second;

        for (std::map<std::string, double>
                 ::const_iterator it2 = it->second.begin();
             it2 != it->second.end(); ++it2) {
          std::string w = it2->first;
          CHECK_DIE(escape_csv_element(&w));
          ofs << toLogProb(it2->second, freq) << '\t'
              << 'U' << ':' << it->first << ' ' << w << std::endl;
        }
      }

      ofs.close();

      if (!text_only) {
        EncoderFeatureIndex feature_index;
        CHECK_DIE(feature_index.convert(txtfile.c_str(), model.c_str()))
            << "unexpected error in LBFGS routin";
      }

      std::cout << "Done!" << std::endl;
    }

    return 0;
  }
};

class OLLearner {
 public:
  static int run(Param *param) {
    const std::string dicdir = param->get<std::string>("dicdir");
    CHECK_DIE(param->load(DCONF(DICRC)))
        << "no such file or directory: " << DCONF(DICRC);

    const std::vector<std::string> files = param->rest_args();
    if (files.size() != 2) {
      std::cout << "Usage: " <<
          param->program_name() << " corpus model" << std::endl;
      return -1;
    }

    const std::string ifile = files[0];
    const std::string model = files[1];

    const double C = param->get<double>("cost");
    const bool   text_only = param->get<bool>("text-only");
    const size_t eval_size = param->get<size_t>("eval-size");
    const size_t unk_eval_size = param->get<size_t>("unk-eval-size");
    const size_t iter = param->get<size_t>("iteration");
    const size_t freq = param->get<size_t>("freq");

    EncoderFeatureIndex feature_index;
    LearnerTokenizer tokenizer;
    FreeList<LearnerPath> path_freelist(PATH_FREELIST_SIZE);
    std::vector<double> expected;
    std::vector<double> observed;
    std::vector<double> alpha;

    std::cout.setf(std::ios::fixed, std::ios::floatfield);
    std::cout.precision(5);

    {
      CHECK_DIE(C > 0) << "cost parameter is out of range: " << C;
      CHECK_DIE(eval_size > 0) << "eval-size is out of range: " << eval_size;
      CHECK_DIE(unk_eval_size > 0) <<
          "unk-eval-size is out of range: " << unk_eval_size;
      CHECK_DIE(tokenizer.open(*param)) << tokenizer.what();
      CHECK_DIE(feature_index.open(*param)) << feature_index.what();
      CHECK_DIE(iter >= 1 && iter <= 100) << "iteration should be <= 100";
      CHECK_DIE(freq == 1) << "freq must be 1";
    }

    std::cout << "reading corpus ..." << std::flush;

    EncoderLearnerTagger x;
    for (size_t i = 0; i < 10; ++i) {
      std::ifstream ifs(ifile.c_str());
      CHECK_DIE(ifs) << "no such file or directory: " << ifile;
      while (ifs) {
        path_freelist.free();
        tokenizer.clear();
        std::fill(expected.begin(), expected.end(), 0.0);
        std::fill(observed.begin(), observed.end(), 0.0);

        CHECK_DIE(x.open(&tokenizer,
                         &path_freelist,
                         &feature_index,
                         eval_size,
                         unk_eval_size)) << x.what();
        CHECK_DIE(x.read(&ifs, &observed)) << x.what();

        if (x.empty()) {
          continue;
        }

        alpha.resize(feature_index.size());
        expected.resize(feature_index.size());
        observed.resize(feature_index.size());
        feature_index.set_alpha(&alpha[0]);

        x.online_update(&expected[0]);

        size_t micro_p = 0;
        size_t micro_r = 0;
        size_t micro_c = 0;
        size_t err = x.eval(&micro_c, &micro_p, &micro_r);
        std::cout << micro_p << " " << micro_r << " " << micro_c << " " << err << std::endl;

        // gradient
        double margin = 0.0;
        double s = 0.0;
        for (size_t k = 0; k < feature_index.size(); ++k) {
          const double tmp = (observed[k] - expected[k]);
          margin += alpha[k] * tmp;
          s += tmp * tmp;
        }

        // Passive Aggressive I algorithm
        if (s > 0.0) {
          const double diff = _max(0.0, 10 - margin) / s;
          if (diff > 0.0) {
            for (size_t k = 0; k < feature_index.size(); ++k) {
              alpha[k] += diff * (observed[k] - expected[k]);
            }
          }
        }
      }
    }

    std::cout << "\nDone! writing model file ... " << std::endl;

    std::string txtfile = model;
    txtfile += ".txt";

    CHECK_DIE(feature_index.save(txtfile.c_str()))
        << feature_index.what();

    if (!text_only) {
      CHECK_DIE(feature_index.convert(txtfile.c_str(), model.c_str()))
          << feature_index.what();
    }

    return true;
  }
};


#ifdef MECAB_USE_THREAD
class learner_thread: public thread {
 public:
  unsigned short start_i;
  unsigned short thread_num;
  size_t size;
  size_t micro_p;
  size_t micro_r;
  size_t micro_c;
  size_t err;
  double f;
  EncoderLearnerTagger **x;
  std::vector<double> expected;
  void run() {
    micro_p = micro_r = micro_c = err = 0;
    f = 0.0;
    std::fill(expected.begin(), expected.end(), 0.0);
    for (size_t i = start_i; i < size; i += thread_num) {
      f += x[i]->gradient(&expected[0]);
      err += x[i]->eval(&micro_c, &micro_p, &micro_r);
    }
  }
};
#endif

class CRFLearner {
 public:
  static int run(Param *param) {
    const std::string dicdir = param->get<std::string>("dicdir");
    CHECK_DIE(param->load(DCONF(DICRC)))
        << "no such file or directory: " << DCONF(DICRC);

    const std::vector<std::string> &files = param->rest_args();
    if (files.size() != 2) {
      std::cout << "Usage: " <<
          param->program_name() << " corpus model" << std::endl;
      return -1;
    }

    const std::string ifile = files[0];
    const std::string model = files[1];

    const double C = param->get<double>("cost");
    const double eta = param->get<double>("eta");
    const bool text_only = param->get<bool>("text-only");
    const size_t eval_size = param->get<size_t>("eval-size");
    const size_t unk_eval_size = param->get<size_t>("unk-eval-size");
    const size_t freq = param->get<size_t>("freq");
    const size_t thread_num = param->get<size_t>("thread");

    EncoderFeatureIndex feature_index;
    LearnerTokenizer tokenizer;
    FreeList<LearnerPath> path_freelist(PATH_FREELIST_SIZE);
    std::vector<double> expected;
    std::vector<double> observed;
    std::vector<double> alpha;
    std::vector<EncoderLearnerTagger *> x_;

    std::cout.setf(std::ios::fixed, std::ios::floatfield);
    std::cout.precision(5);

    std::ifstream ifs(ifile.c_str());
    {
      CHECK_DIE(C > 0) << "cost parameter is out of range: " << C;
      CHECK_DIE(eta > 0) "eta is out of range: " << eta;
      CHECK_DIE(eval_size > 0) << "eval-size is out of range: " << eval_size;
      CHECK_DIE(unk_eval_size > 0) <<
          "unk-eval-size is out of range: " << unk_eval_size;
      CHECK_DIE(freq > 0) <<
          "freq is out of range: " << unk_eval_size;
      CHECK_DIE(thread_num > 0 && thread_num <= 512)
          << "# thread is invalid: " << thread_num;
      CHECK_DIE(tokenizer.open(*param)) << tokenizer.what();
      CHECK_DIE(feature_index.open(*param)) << feature_index.what();
      CHECK_DIE(ifs) << "no such file or directory: " << ifile;
    }

    std::cout << "reading corpus ..." << std::flush;

    while (ifs) {
      EncoderLearnerTagger *_x = new EncoderLearnerTagger();

      CHECK_DIE(_x->open(&tokenizer, &path_freelist,
                         &feature_index,
                         eval_size,
                         unk_eval_size))
          << _x->what();

      CHECK_DIE(_x->read(&ifs, &observed)) << _x->what();

      if (!_x->empty())
        x_.push_back(_x);
      else
        delete _x;

      if (x_.size() % 100 == 0)
        std::cout << x_.size() << "... " << std::flush;
    }

    feature_index.shrink(freq, &observed);
    feature_index.clearcache();

    int converge = 0;
    double old_f = 0.0;
    size_t psize = feature_index.size();
    observed.resize(psize);
    LBFGS lbfgs;

    alpha.resize(psize);
    expected.resize(psize);
    std::fill(alpha.begin(), alpha.end(), 0.0);

    feature_index.set_alpha(&alpha[0]);

    std::cout << std::endl;
    std::cout << "Number of sentences: " << x_.size() << std::endl;
    std::cout << "Number of features:  " << psize     << std::endl;
    std::cout << "eta:                 " << eta       << std::endl;
    std::cout << "freq:                " << freq      << std::endl;
#ifdef MECAB_USE_THREAD
    std::cout << "threads:             " << thread_num << std::endl;
#endif
    std::cout << "C(sigma^2):          " << C          << std::endl
              << std::endl;

#ifdef MECAB_USE_THREAD
    std::vector<learner_thread> thread;
    if (thread_num > 1) {
      thread.resize(thread_num);
      for (size_t i = 0; i < thread_num; ++i) {
        thread[i].start_i = i;
        thread[i].size = x_.size();
        thread[i].thread_num = thread_num;
        thread[i].x = &x_[0];
        thread[i].expected.resize(expected.size());
      }
    }
#endif

    for (size_t itr = 0; ;  ++itr) {
      std::fill(expected.begin(), expected.end(), 0.0);

      double f = 0.0;
      size_t err = 0;
      size_t micro_p = 0;
      size_t micro_r = 0;
      size_t micro_c = 0;

#ifdef MECAB_USE_THREAD
      if (thread_num > 1) {
        for (size_t i = 0; i < thread_num; ++i)
          thread[i].start();

        for (size_t i = 0; i < thread_num; ++i)
          thread[i].join();

        for (size_t i = 0; i < thread_num; ++i) {
          f += thread[i].f;
          err += thread[i].err;
          micro_r += thread[i].micro_r;
          micro_p += thread[i].micro_p;
          micro_c += thread[i].micro_c;
          for (size_t k = 0; k < psize; ++k)
            expected[k] += thread[i].expected[k];
        }
      }
      else
#endif
      {
        for (size_t i = 0; i < x_.size(); ++i) {
          f += x_[i]->gradient(&expected[0]);
          err += x_[i]->eval(&micro_c, &micro_p, &micro_r);
        }
      }

      const double p = 1.0 * micro_c / micro_p;
      const double r = 1.0 * micro_c / micro_r;
      const double micro_f = 2 * p * r /(p + r);

      for (size_t i = 0; i < psize; ++i) {
        f += (alpha[i] * alpha[i]/(2.0 * C));
        expected[i] = expected[i] - observed[i] + alpha[i]/C;
      }

      double diff = (itr == 0 ? 1.0 : std::fabs(1.0 *(old_f - f) )/old_f);
      std::cout << "iter="    << itr
                << " err="    << 1.0 * err/x_.size()
                << " F="      << micro_f
                << " target=" << f
                << " diff="   << diff << std::endl;
      old_f = f;

      if (diff < eta)
        converge++;
      else
        converge = 0;

      if (converge == 3)
        break;  // 3 is ad-hoc

      int ret = lbfgs.optimize(psize, &alpha[0], f, &expected[0], false, C);

      CHECK_DIE(ret > 0) << "unexpected error in LBFGS routin";
    }

    std::cout << "\nDone! writing model file ... " << std::endl;

    std::string txtfile = model;
    txtfile += ".txt";

    CHECK_DIE(feature_index.save(txtfile.c_str()))
        << feature_index.what();

    if (!text_only) {
      CHECK_DIE(feature_index.convert(txtfile.c_str(), model.c_str()))
          << feature_index.what();
    }

    return 0;
  }
};

class Learner {
 public:
  static bool run(int argc, char **argv) {
    static const MeCab::Option long_options[] = {
      { "dicdir",   'd',  ".",     "DIR",
        "set DIR as dicdir(default \".\" )" },
      { "cost",     'c',  "1.0",   "FLOAT",
        "set FLOAT for cost C for constraints violatoin" },
      { "training-algorithm",   'a',  "crf",
        "(crf|hmm|oll)", "set training algorithm" },
      { "em-hmm", 'E', 0, 0,       "use EM in HMM training (experimental)" },
      { "freq",     'f',  "1",     "INT",
        "set the frequency cut-off (default 1)" },
      { "default-emission-freq", 'E',  "0.5",     "FLOAT",
        "set the default emission frequency for HMM (default 0.5)" },
      { "default-transition-freq", 'T',  "0.5",     "FLOAT",
        "set the default transition frequency for HMM (default 0.0)" },
      { "eta",      'e',  "0.001", "DIR",
        "set FLOAT for tolerance of termination criterion" },
      { "iteration", 'N', "10",    "INT",
        "numer of iterations in online learning (default 1)" },
      { "thread",   'p',  "1",     "INT",    "number of threads(default 1)" },
      { "build",    'b',  0,  0,   "build binary model from text model"},
      { "text-only", 'y',  0,  0,   "output text model only" },
      { "version",  'v',  0,   0,  "show the version and exit"  },
      { "help",     'h',  0,   0,  "show this help and exit."      },
      { 0, 0, 0, 0 }
    };

    Param param;

    if (!param.open(argc, argv, long_options)) {
      std::cout << param.what() << "\n\n" <<  COPYRIGHT
                << "\ntry '--help' for more information." << std::endl;
      return -1;
    }

    if (!param.help_version()) {
      return 0;
    }

    // build mode
    {
      const bool build = param.get<bool>("build");
      if (build) {
        const std::vector<std::string> files = param.rest_args();
        if (files.size() != 2) {
          std::cout << "Usage: " <<
              param.program_name() << " corpus model" << std::endl;
          return -1;
        }
        const std::string ifile = files[0];
        const std::string model = files[1];
        EncoderFeatureIndex feature_index;
        CHECK_DIE(feature_index.convert(ifile.c_str(), model.c_str()))
            << feature_index.what();
        return 0;
      }
    }

    std::string type = param.get<std::string>("training-algorithm");
    toLower(&type);
    if (type == "crf") {
      return CRFLearner::run(&param);
    } else if (type == "hmm") {
      return HMMLearner::run(&param);
    } else if (type == "oll") {
      return OLLearner::run(&param);
    } else {
      std::cerr << "unknown type: " << type << std::endl;
      return -1;
    }

    return 0;
  }
};
}

int mecab_cost_train(int argc, char **argv) {
  return MeCab::Learner::run(argc, argv);
}
