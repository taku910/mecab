//  MeCab -- Yet Another Part-of-Speech and Morphological Analyzer
//
//
//  Copyright(C) 2001-2006 Taku Kudo <taku@chasen.org>
//  Copyright(C) 2004-2006 Nippon Telegraph and Telephone Corporation
#ifndef MECAB_CONTEXT_ID_H
#define MECAB_CONTEXT_ID_H

#include <map>
#include <string>
#include <vector>

namespace MeCab {

class Param;
class Iconv;

class ContextID {
 public:
  void clear();
  void add(const char *l, const char *r);
  void addBOS(const char *l, const char *r);
  bool save(const char* lfile,
            const char* rfile);
  bool build();
  bool open(const char *lfile,
            const char *rfile,
            Iconv *iconv = 0);
  int  lid(const char *l) const;
  int  rid(const char *r) const;

  size_t left_size() const { return left_size_; }
  size_t right_size() const { return right_size_; }

  const std::map<std::string, int>& left_ids()  const { return left_; }
  const std::map<std::string, int>& right_ids() const { return right_; }

  bool is_valid(size_t lid, size_t rid) {
    return (lid >= 0 && lid < left_size() &&
            rid >= 0 && rid < right_size());
  }

  ContextID() : left_size_(0), right_size_(0) {}

 private:
  std::map<std::string, int>  left_;
  std::map<std::string, int>  right_;
  std::string                 left_bos_;
  std::string                 right_bos_;
  size_t                      left_size_;
  size_t                      right_size_;
};
}
#endif
