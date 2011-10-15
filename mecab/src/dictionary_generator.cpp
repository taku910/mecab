// MeCab -- Yet Another Part-of-Speech and Morphological Analyzer
//
//  $Id: dictionary_generator.cpp 173 2009-04-18 08:10:57Z taku-ku $;
//
//  Copyright(C) 2001-2006 Taku Kudo <taku@chasen.org>
//  Copyright(C) 2004-2006 Nippon Telegraph and Telephone Corporation
#include <iostream>
#include <map>
#include <vector>
#include <string>
#include <fstream>
#include "common.h"
#include "utils.h"
#include "mecab.h"
#include "param.h"
#include "mmap.h"
#include "feature_index.h"
#include "context_id.h"
#include "dictionary_rewriter.h"
#include "dictionary.h"
#include "char_property.h"

namespace MeCab {

short int tocost(double d, int n, int default_cost) {
  if (d == 0.0) return default_cost;
  static const short max = +32767;
  static const short min = -32767;
  return static_cast<short>(_max(_min(
                                     -n * d,
                                     static_cast<double>(max)),
                                 static_cast<double>(min)) );
}

void copy(const char *src, const char *dst) {
  std::cout << "copying " << src << " to " <<  dst << std::endl;
  Mmap<char> mmap;
  CHECK_DIE(mmap.open(src)) << mmap.what();
  std::ofstream ofs(dst, std::ios::binary|std::ios::out);
  CHECK_DIE(ofs) << "permission denied: " << dst;
  ofs.write(reinterpret_cast<char*>(mmap.begin()), mmap.size());
  ofs.close();
}

class DictionaryGenerator {
 private:
  static void gencid_bos(const std::string &bos_feature,
                         DictionaryRewriter *rewrite,
                         ContextID *cid) {
    std::string ufeature, lfeature, rfeature;
    rewrite->rewrite2(bos_feature, &ufeature, &lfeature, &rfeature);
    cid->addBOS(lfeature.c_str(), rfeature.c_str());
  }

  static void gencid(const char *filename,
                     DictionaryRewriter *rewrite,
                     ContextID *cid) {
    std::ifstream ifs(filename);
    CHECK_DIE(ifs) << "no such file or directory: " << filename;
    char line[BUF_SIZE];
    std::cout << "reading " << filename << " ... " << std::flush;
    size_t num = 0;
    std::string feature, ufeature, lfeature, rfeature;
    char *col[8];
    while (ifs.getline(line, sizeof(line))) {
      const size_t n = tokenizeCSV(line, col, 5);
      CHECK_DIE(n == 5) << "format error: " << line;
      feature = col[4];
      rewrite->rewrite2(feature, &ufeature, &lfeature, &rfeature);
      cid->add(lfeature.c_str(), rfeature.c_str());
      ++num;
    }
    std::cout << num << std::endl;
    ifs.close();
  }

  static bool genmatrix(const char *filename,
                        const ContextID &cid,
                        DecoderFeatureIndex *fi,
                        int factor,
                        int default_cost) {
    std::ofstream ofs(filename);
    CHECK_DIE(ofs) << "permission denied: " << filename;

    LearnerPath path;
    LearnerNode rnode;
    LearnerNode lnode;
    rnode.stat = lnode.stat = MECAB_NOR_NODE;
    rnode.rpath = &path;
    lnode.lpath = &path;
    path.lnode = &lnode;
    path.rnode = &rnode;

    const std::map<std::string, int> &left =  cid.left_ids();
    const std::map<std::string, int> &right = cid.right_ids();

    CHECK_DIE(left.size())  << "left id size is empty";
    CHECK_DIE(right.size()) << "right id size is empty";

    ofs << right.size() << ' ' << left.size() << std::endl;

    size_t l = 0;
    for (std::map<std::string, int>::const_iterator rit = right.begin();
         rit != right.end();
         ++rit) {
      ++l;
      progress_bar("emitting matrix      ", l+1, right.size());
      for (std::map<std::string, int>::const_iterator lit = left.begin();
           lit != left.end();
           ++lit) {
        path.rnode->wcost = 0;
        fi->buildBigramFeature(&path, rit->first.c_str(), lit->first.c_str());
        fi->calcCost(&path);
        ofs << rit->second << ' ' << lit->second << ' '
            << tocost(path.cost, factor, default_cost) << std::endl;
      }
    }

    return true;
  }

  static void gendic(const char* ifile,
                     const char* ofile,
                     const CharProperty &property,
                     DictionaryRewriter *rewrite,
                     const ContextID &cid,
                     DecoderFeatureIndex *fi,
                     bool unk,
                     int factor,
                     int default_cost) {
    std::ifstream ifs(ifile);
    CHECK_DIE(ifs) << "no such file or directory: " << ifile;

    std::ofstream ofs(ofile);
    CHECK_DIE(ofs) << "permission denied: " << ofile;

    std::string w, feature, ufeature, lfeature, rfeature;
    int cost, lid, rid;

    std::cout <<  "emitting " << ofile << " ... " << std::flush;

    LearnerPath path;
    LearnerNode rnode;
    LearnerNode lnode;
    rnode.stat  = lnode.stat = MECAB_NOR_NODE;
    rnode.rpath = &path;
    lnode.lpath = &path;
    path.lnode  = &lnode;
    path.rnode  = &rnode;

    char line[BUF_SIZE];
    char *col[8];
    size_t num = 0;

    while (ifs.getline(line, sizeof(line))) {
      const size_t n = tokenizeCSV(line, col, 5);
      CHECK_DIE(n == 5) << "format error: " << line;

      w = std::string(col[0]);
      lid = std::atoi(col[1]);
      rid = std::atoi(col[2]);
      cost = std::atoi(col[3]);
      feature = std::string(col[4]);

      rewrite->rewrite2(feature, &ufeature, &lfeature, &rfeature);
      lid = cid.lid(lfeature.c_str());
      rid = cid.rid(rfeature.c_str());

      CHECK_DIE(lid > 0) << "CID is not found for " << lfeature;
      CHECK_DIE(rid > 0) << "CID is not found for " << rfeature;

      if (unk) {
        int c = property.id(w.c_str());
        CHECK_DIE(c >= 0) << "unknown property [" << w << "]";
        path.rnode->char_type = (unsigned char)c;
      } else {
        size_t mblen;
        CharInfo cinfo = property.getCharInfo(w.c_str(),
                                              w.c_str() + w.size(), &mblen);
        path.rnode->char_type = cinfo.default_type;
      }

      fi->buildUnigramFeature(&path, ufeature.c_str());
      fi->calcCost(&rnode);
      CHECK_DIE(escape_csv_element(&w)) << "invalid character found: " << w;

      ofs << w << ',' << lid << ',' << rid << ','
          << tocost(rnode.wcost, factor, default_cost)
          << ',' << feature << std::endl;
      ++num;
    }

    std::cout << num << std::endl;
  }

 public:

  static int run(int argc, char **argv) {
    static const MeCab::Option long_options[] = {
      { "dicdir",  'd',  ".",   "DIR", "set DIR as dicdir(default \".\" )" },
      { "outdir",  'o',  ".",   "DIR", "set DIR as output dir" },
      { "model",   'm',  0,     "FILE",   "use FILE as model file" },
      { "version", 'v',  0,   0,  "show the version and exit"  },
      { "training-algorithm", 'a',  "crf",    "(crf|hmm)",
        "set training algorithm" },
      { "default-emission-cost", 'E', "4000", "INT",
        "set default emission cost for HMM" },
      { "default-transition-cost", 'T', "4000", "INT",
        "set default transition cost for HMM" },
      { "help",    'h',  0,   0,  "show this help and exit."      },
      { 0, 0, 0, 0 }
    };

    Param param;

    if (!param.open(argc, argv, long_options)) {
      std::cout << param.what() << "\n\n" <<  COPYRIGHT
                << "\ntry '--help' for more information." << std::endl;
      return -1;
    }

    if (!param.help_version()) return 0;

    ContextID cid;
    DecoderFeatureIndex fi;
    DictionaryRewriter rewrite;

    const std::string dicdir = param.get<std::string>("dicdir");
    const std::string outdir = param.get<std::string>("outdir");
    const std::string model = param.get<std::string>("model");

#define DCONF(file) create_filename(dicdir, std::string(file)).c_str()
#define OCONF(file) create_filename(outdir, std::string(file)).c_str()

    CHECK_DIE(param.load(DCONF(DICRC)))
        << "no such file or directory: " << DCONF(DICRC);

    std::string charset;
    {
      Dictionary dic;
      CHECK_DIE(dic.open(DCONF(SYS_DIC_FILE), "r"));
      charset = dic.charset();
      CHECK_DIE(!charset.empty());
    }

    int default_emission_cost = 0;
    int default_transition_cost = 0;

    std::string type = param.get<std::string>("training-algorithm");
    toLower(&type);

    if (type == "hmm") {
      default_emission_cost =
          param.get<int>("default-emission-cost");
      default_transition_cost =
          param.get<int>("default-transition-cost");
      CHECK_DIE(default_transition_cost > 0)
          << "default transition cost must be > 0";
      CHECK_DIE(default_emission_cost > 0)
          << "default transition cost must be > 0";
      param.set("identity-template", 1);
    }

    CharProperty property;
    CHECK_DIE(property.open(param));
    property.set_charset(charset.c_str());

    const std::string bos = param.get<std::string>("bos-feature");
    const int factor = param.get<int>("cost-factor");

    std::vector<std::string> dic;
    enum_csv_dictionaries(dicdir.c_str(), &dic);

    {
      CHECK_DIE(dicdir != outdir) <<
          "output directory = dictionary directory! "
          "Please specify different directory.";
      CHECK_DIE(!outdir.empty()) << "output directory is empty";
      CHECK_DIE(!model.empty()) << "model file is empty";
      CHECK_DIE(fi.open(param)) << fi.what();
      CHECK_DIE(factor > 0)   << "cost factor needs to be positive value";
      CHECK_DIE(!bos.empty()) << "bos-feature is empty";
      CHECK_DIE(dic.size()) << "no dictionary is found in " << dicdir;
      CHECK_DIE(rewrite.open(DCONF(REWRITE_FILE)));
    }

    gencid_bos(bos, &rewrite, &cid);
    gencid(DCONF(UNK_DEF_FILE), &rewrite, &cid);

    for (std::vector<std::string>::const_iterator it = dic.begin();
         it != dic.end();
         ++it) {
      gencid(it->c_str(), &rewrite, &cid);
    }

    std::cout << "emitting "
              << OCONF(LEFT_ID_FILE) << "/ "
              << OCONF(RIGHT_ID_FILE) << std::endl;

    cid.build();
    cid.save(OCONF(LEFT_ID_FILE), OCONF(RIGHT_ID_FILE));

    gendic(DCONF(UNK_DEF_FILE), OCONF(UNK_DEF_FILE), property,
           &rewrite, cid, &fi, true, factor, default_emission_cost);

    for (std::vector<std::string>::const_iterator it = dic.begin();
         it != dic.end();
         ++it) {
      std::string file =  *it;
      remove_pathname(&file);
      gendic(it->c_str(), OCONF(file.c_str()), property,
             &rewrite, cid, &fi, false, factor, default_emission_cost);
    }

    genmatrix(OCONF(MATRIX_DEF_FILE), cid, &fi,
              factor, default_transition_cost);

    copy(DCONF(CHAR_PROPERTY_DEF_FILE), OCONF(CHAR_PROPERTY_DEF_FILE));
    copy(DCONF(REWRITE_FILE), OCONF(REWRITE_FILE));
    copy(DCONF(DICRC), OCONF(DICRC));

    if (type == "crf")
      copy(DCONF(FEATURE_FILE), OCONF(FEATURE_FILE));

#undef OCONF
#undef DCONF

    std::cout <<  "\ndone!\n";

    return 0;
  }
};
}

// export functions
int mecab_dict_gen(int argc, char **argv) {
  return MeCab::DictionaryGenerator::run(argc, argv);
}
