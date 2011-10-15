//  MeCab -- Yet Another Part-of-Speech and Morphological AnalyzerEnco
//
//  $Id: feature_index.cpp 173 2009-04-18 08:10:57Z taku-ku $;
//
//  Copyright(C) 2001-2006 Taku Kudo <taku@chasen.org>
//  Copyright(C) 2004-2006 Nippon Telegraph and Telephone Corporation
#include <fstream>
#include <string>
#include <cstring>
#include "param.h"
#include "common.h"
#include "utils.h"
#include "learner_node.h"
#include "feature_index.h"
#include "string_buffer.h"

#define BUFSIZE 2048
#define POSSIZE 64

#define ADDB(b) do { int id = this->id((b));            \
    if (id != -1) feature_.push_back(id); } while (0)

#define COPY_FEATURE(ptr) do {                                          \
    feature_.push_back(-1);                                             \
    (ptr) = feature_freelist_.alloc(feature_.size());                   \
    std::copy(feature_.begin(), feature_.end(), const_cast<int *>(ptr)); \
    feature_.clear(); } while (0)

namespace MeCab {

const char* FeatureIndex::getIndex(char **p, char ** column, size_t max) {
  ++(*p);

  bool flg = false;

  if (**p == '?') {
    flg = true;
    ++(*p);
  }  // undef flg

  CHECK_RETURN(**p =='[', static_cast<const char *>(0))
      << "getIndex(): unmatched '['";

  size_t n = 0;
  ++(*p);

  for (;; ++(*p)) {
    switch (**p) {
      case '0': case '1': case '2': case '3': case '4':
      case '5': case '6': case '7': case '8': case '9':
        n = 10 * n + (**p - '0');
        break;
      case ']':
        CHECK_RETURN(n < max, static_cast<const char *>(0))
            << "given index is out of range: " << n;

        if (flg == true && ((std::strcmp("*", column[n]) == 0)
                            || column[n][0] == '\0')) return 0;

        return column[n];  // return;

        break;
      default:
        CHECK_RETURN(false, static_cast<const char *>(0)) << "unmatched '['";
    }
  }

  return 0;
}

void FeatureIndex::set_alpha(const double *alpha) {
  alpha_ = alpha;
}

bool FeatureIndex::openTemplate(const Param &param) {
  if (param.get<bool>("identity-template")) {
    unigram_templs_.clear();
    bigram_templs_.clear();
    unigram_templs_.push_back("U:%u");
    bigram_templs_.push_back("B:%r/%l");
    return true;
  }

  std::string filename = create_filename(param.get<std::string>("dicdir"),
                                         FEATURE_FILE);
  std::ifstream ifs(filename.c_str());
  CHECK_FALSE(ifs) << "no such file or directory: " << filename;

  char buf[BUF_SIZE];
  char *column[4];

  unigram_templs_.clear();
  bigram_templs_.clear();

  while (ifs.getline(buf, sizeof(buf))) {
    if (buf[0] == '\0' || buf[0] == '#' || buf[0] == ' ') continue;
    CHECK_FALSE(tokenize2(buf, "\t ", column, 2) == 2)
        << "format error: " <<filename;

    if (std::strcmp(column[0], "UNIGRAM") == 0)
      unigram_templs_.push_back(this->strdup(column[1]));
    else if (std::strcmp(column[0], "BIGRAM") == 0 )
      bigram_templs_.push_back(this->strdup(column[1]));
    else
      CHECK_FALSE(false) << "format error: " <<  filename;
  }

  // second, open rewrite rules
  filename = create_filename(param.get<std::string>("dicdir"),
                             REWRITE_FILE);
  rewrite_.open(filename.c_str());

  return true;
}

bool EncoderFeatureIndex::open(const Param &param) {
  return openTemplate(param);
}

bool DecoderFeatureIndex::open(const Param &param) {
  const std::string modelfile = param.get<std::string>("model");

  CHECK_FALSE(mmap_.open(modelfile.c_str())) << mmap_.what();

  const char *ptr = mmap_.begin();
  unsigned int maxid;
  read_static<unsigned int>(&ptr, maxid);
  maxid_ = static_cast<size_t>(maxid);
  alpha_ = reinterpret_cast<const double *>(ptr);
  ptr += (sizeof(alpha_[0]) * maxid_);
  da_.set_array(reinterpret_cast<void *>(const_cast<char *>(ptr)));

  if (!openTemplate(param)) {
    close();
    return false;
  }

  return true;
}

void DecoderFeatureIndex::clear() {
  feature_freelist_.free();
}

void EncoderFeatureIndex::clear() {}

void EncoderFeatureIndex::clearcache() {
  feature_cache_.clear();
  rewrite_.clear();
}

void EncoderFeatureIndex::close() {
  dic_.clear();
  feature_cache_.clear();
  maxid_ = 0;
}

void DecoderFeatureIndex::close() {
  da_.clear();
  mmap_.close();
  maxid_ = 0;
}

void FeatureIndex::calcCost(LearnerNode *node) {
  node->wcost = 0.0;
  if (node->stat == MECAB_EOS_NODE) return;
  for (const int *f = node->fvector; *f != -1; ++f)
    node->wcost += alpha_[*f];
}

void FeatureIndex::calcCost(LearnerPath *path) {
  if (is_empty(path)) return;
  path->cost = path->rnode->wcost;
  for (const int *f = path->fvector; *f != -1; ++f)
    path->cost += alpha_[*f];
}

const char *FeatureIndex::strdup(const char *p) {
  size_t len = std::strlen(p);
  char *q = char_freelist_.alloc(len + 1);
  std::strncpy(q, p, len);
  return q;
}

bool DecoderFeatureIndex::buildFeature(LearnerPath *path) {
  path->rnode->wcost = path->cost = 0.0;

  std::string ufeature1;
  std::string lfeature1;
  std::string rfeature1;
  std::string ufeature2;
  std::string lfeature2;
  std::string rfeature2;

  CHECK_DIE(rewrite_.rewrite2(path->lnode->feature,
                              &ufeature1,
                              &lfeature1,
                              &rfeature1))
      << " cannot rewrite pattern: "
      << path->lnode->feature;

  CHECK_DIE(rewrite_.rewrite2(path->rnode->feature,
                              &ufeature2,
                              &lfeature2,
                              &rfeature2))
      << " cannot rewrite pattern: "
      << path->rnode->feature;

  if (!buildUnigramFeature(path, ufeature2.c_str()))
    return false;

  if (!buildBigramFeature(path, rfeature1.c_str(), lfeature2.c_str()))
    return false;

  return true;
}

bool EncoderFeatureIndex::buildFeature(LearnerPath *path) {
  path->rnode->wcost = path->cost = 0.0;

  std::string ufeature1;
  std::string lfeature1;
  std::string rfeature1;
  std::string ufeature2;
  std::string lfeature2;
  std::string rfeature2;

  CHECK_DIE(rewrite_.rewrite2(path->lnode->feature,
                              &ufeature1,
                              &lfeature1,
                              &rfeature1))
      << " cannot rewrite pattern: "
      << path->lnode->feature;

  CHECK_DIE(rewrite_.rewrite2(path->rnode->feature,
                              &ufeature2,
                              &lfeature2,
                              &rfeature2))
      << " cannot rewrite pattern: "
      << path->rnode->feature;

  {
    os_.clear();
    os_ << ufeature2 << ' ' << path->rnode->char_type << '\0';
    const std::string key(os_.str());
    std::map<std::string, std::pair<const int *, size_t> >::iterator
        it = feature_cache_.find(key);
    if (it != feature_cache_.end()) {
      path->rnode->fvector = it->second.first;
      it->second.second++;
    } else {
      if (!buildUnigramFeature(path, ufeature2.c_str())) return false;
      feature_cache_.insert(std::pair
                            <std::string, std::pair<const int *, size_t> >
                            (key,
                             std::make_pair<const int *, size_t>
                             (path->rnode->fvector, 1)));
    }
  }

  {
    os_.clear();
    os_ << rfeature1 << ' ' << lfeature2 << '\0';
    std::string key(os_.str());
    std::map<std::string, std::pair<const int *, size_t> >::iterator
        it = feature_cache_.find(key);
    if (it != feature_cache_.end()) {
      path->fvector = it->second.first;
      it->second.second++;
    } else {
      if (!buildBigramFeature(path, rfeature1.c_str(), lfeature2.c_str()))
        return false;
      feature_cache_.insert(std::pair
                            <std::string, std::pair<const int *, size_t> >
                            (key, std::make_pair<const int *, size_t>
                             (path->fvector, 1)));
    }
  }

  CHECK_DIE(path->fvector) << " fvector is NULL";
  CHECK_DIE(path->rnode->fvector) << "fevector is NULL";

  return true;
}

bool FeatureIndex::buildUnigramFeature(LearnerPath *path,
                                       const char *ufeature) {
  char ubuf[BUFSIZE];
  char *F[POSSIZE];

  feature_.clear();
  std::strncpy(ubuf, ufeature, BUFSIZE);
  size_t usize = tokenizeCSV(ubuf, F, POSSIZE);

  for (std::vector<const char*>::const_iterator it = unigram_templs_.begin();
       it != unigram_templs_.end(); ++it) {
    const char *p = *it;
    os_.clear();

    for (; *p; p++) {
      switch (*p) {
        default: os_ << *p; break;
        case '\\': os_ << getEscapedChar(*++p); break;
        case '%': {
          switch (*++p) {
            case 'F':  {
              const char *r = getIndex(const_cast<char **>(&p), F, usize);
              if (!r) goto NEXT;
              os_ << r;
            } break;
            case 't':  os_ << (size_t)path->rnode->char_type;     break;
            case 'u':  os_ << ufeature; break;
            default:
              CHECK_FALSE(false) << "unkonwn meta char: " <<  *p;
          }
        }
      }
    }

    os_ << '\0';
    ADDB(os_.str());

 NEXT: continue;
  }

  COPY_FEATURE(path->rnode->fvector);

  return true;
}

bool FeatureIndex::buildBigramFeature(LearnerPath *path,
                                      const char *rfeature,
                                      const char *lfeature) {
  char rbuf[BUFSIZE];
  char lbuf[BUFSIZE];
  char *R[POSSIZE];
  char *L[POSSIZE];

  feature_.clear();
  std::strncpy(lbuf,  rfeature, BUFSIZE);
  std::strncpy(rbuf,  lfeature, BUFSIZE);

  size_t lsize = tokenizeCSV(lbuf, L, POSSIZE);
  size_t rsize = tokenizeCSV(rbuf, R, POSSIZE);

  for (std::vector<const char*>::const_iterator it = bigram_templs_.begin();
       it != bigram_templs_.end(); ++it) {
    const char *p = *it;
    os_.clear();

    for (; *p; p++) {
      switch (*p) {
        default: os_ << *p; break;
        case '\\': os_ << getEscapedChar(*++p); break;
        case '%': {
          switch (*++p) {
            case 'L': {
              const char *r = getIndex(const_cast<char **>(&p), L, lsize);
              if (!r) goto NEXT;
              os_ << r;
            } break;
            case 'R': {
              const char *r = getIndex(const_cast<char **>(&p), R, rsize);
              if (!r) goto NEXT;
              os_ << r;
            } break;
            case 'l':  os_ << lfeature; break;  // use lfeature as it is
            case 'r':  os_ << rfeature; break;
            default:
              CHECK_FALSE(false) << "unkonwn meta char: " <<  *p;
          }
        }
      }
    }

    os_ << '\0';

    ADDB(os_.str());

 NEXT: continue;
  }

  COPY_FEATURE(path->fvector);

  return true;
}

int DecoderFeatureIndex::id(const char *key) {
  return da_.exactMatchSearch<Darts::DoubleArray::result_type>(key);
}

int EncoderFeatureIndex::id(const char *key) {
  std::map<std::string, int>::const_iterator it = dic_.find(key);
  if (it == dic_.end()) {
    dic_.insert(std::make_pair<std::string, int>(std::string(key), maxid_));
    return maxid_++;
  } else {
    return it->second;
  }
  return -1;
}

void EncoderFeatureIndex::shrink(size_t freq,
                                 std::vector<double> *observed) {
  if (freq <= 1) return;

  // count fvector
  std::vector<size_t> freqv(maxid_);
  std::fill(freqv.begin(), freqv.end(), 0);
  for (std::map<std::string, std::pair<const int*, size_t> >::const_iterator
           it = feature_cache_.begin();
       it != feature_cache_.end(); ++it) {
    for (const int *f = it->second.first; *f != -1; ++f)
      freqv[*f] += it->second.second;  // freq
  }

  // make old2new map
  maxid_ = 0;
  std::map<int, int> old2new;
  for (size_t i = 0; i < freqv.size(); ++i) {
    if (freqv[i] >= freq)
      old2new.insert(std::make_pair<int, int>(i, maxid_++));
  }

  // update dic_
  for (std::map<std::string, int>::iterator
           it = dic_.begin(); it != dic_.end();) {
    std::map<int, int>::const_iterator it2 = old2new.find(it->second);
    if (it2 != old2new.end()) {
      it->second = it2->second;
      ++it;
    } else {
      dic_.erase(it++);
    }
  }

  // update all fvector
  for (std::map<std::string, std::pair<const int*, size_t> >::const_iterator
           it = feature_cache_.begin(); it != feature_cache_.end(); ++it) {
    int *to = const_cast<int *>(it->second.first);
    for (const int *f = it->second.first; *f != -1; ++f) {
      std::map<int, int>::const_iterator it2 = old2new.find(*f);
      if (it2 != old2new.end()) {
        *to = it2->second;
        ++to;
      }
    }
    *to = -1;
  }

  // update observed vector
  std::vector<double> observed_new(maxid_);
  for (size_t i = 0; i < observed->size(); ++i) {
    std::map<int, int>::const_iterator it = old2new.find(static_cast<int>(i));
    if (it != old2new.end())
      observed_new[it->second] = (*observed)[i];
  }

  *observed = observed_new;  // copy

  return;
}

bool FeatureIndex::convert(const char* txtfile, const char *binfile) {
  std::ifstream ifs(txtfile);

  CHECK_DIE(ifs) << "no such file or directory: " << txtfile;

  char buf[BUF_SIZE];
  char *column[4];
  std::map<std::string, double> dic;

  while (ifs.getline(buf, sizeof(buf))) {
    CHECK_DIE(tokenize2(buf, "\t", column, 2) == 2)
        << "format error: " << buf;

    dic.insert(std::make_pair<std::string, double>
               (std::string(column[1]), atof(column[0]) ));
  }

  std::ofstream ofs(binfile, std::ios::out | std::ios::binary);
  CHECK_DIE(ofs) << "permission denied: " << binfile;

  std::vector<char *> key;
  unsigned int size = static_cast<unsigned int>(dic.size());
  ofs.write(reinterpret_cast<const char*>(&size), sizeof(unsigned int));

  for (std::map<std::string, double>::const_iterator
           it = dic.begin(); it != dic.end(); ++it) {
    key.push_back(const_cast<char*>(it->first.c_str()));
    ofs.write(reinterpret_cast<const char*>(&it->second), sizeof(double));
  }

  Darts::DoubleArray da;
  CHECK_DIE(da.build(key.size(), &key[0], 0, 0, 0) == 0)
      << "unkown error in building double array: " << binfile;

  ofs.write(reinterpret_cast<const char*>(da.array()),
            da.unit_size() * da.size());

  return true;
}

bool EncoderFeatureIndex::save(const char *filename) {
  std::ofstream ofs(filename);

  CHECK_FALSE(ofs) << "permission denied: " << filename;

  ofs.setf(std::ios::fixed, std::ios::floatfield);
  ofs.precision(24);

  for (std::map<std::string, int>::const_iterator it = dic_.begin();
       it != dic_.end(); ++it)
    ofs << alpha_[it->second] << "\t" << it->first << std::endl;

  return true;
}
}
