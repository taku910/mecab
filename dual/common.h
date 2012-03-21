#ifndef DUAL_COMMON_H_
#define DUAL_COMMON_H_

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <string>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <cmath>

#if defined(_WIN32) && !defined(__CYGWIN__)
# define OUTPUT_MODE std::ios::binary|std::ios::out
#else
# define OUTPUT_MODE std::ios::out
#endif

#define INVALID_COST (0.0)
#define VERY_SMALL_COST (0.000001)
#define BUF_SIZE 8192

namespace dual {

class die {
 public:
  die() {}
  ~die() {
    std::cerr << std::endl;
    exit(-1);
  }
  int operator&(std::ostream&) { return 0; }
};

struct whatlog {
  std::ostringstream stream_;
  std::string str_;
  const char *str() {
    str_ = stream_.str();
    return str_.c_str();
  }
};

class wlog {
 public:
  wlog(whatlog *what) : what_(what) {
    what_->stream_.clear();
  }
  bool operator&(std::ostream &) {
    return false;
  }
 private:
  whatlog *what_;
};
}  // dual

#define CHECK_FALSE(condition) \
 if (condition) {} else return \
   wlog(&what_) & what_.stream_ <<              \
      __FILE__ << "(" << __LINE__ << ") [" << #condition << "] "

#define CHECK_DIE(condition) \
(condition) ? 0 : die() & std::cerr << __FILE__ << \
"(" << __LINE__ << ") [" << #condition << "] "

#endif
