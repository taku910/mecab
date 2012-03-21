#ifndef DUAL_UTILS_H_
#define DUAL_UTILS_H_

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <string>
#include <algorithm>
#include <string>
#include <iostream>
#include <vector>
#include "common.h"

namespace dual {

template <class Iterator>
inline size_t tokenizeCSV(char *str,
                          Iterator out, size_t max) {
  char *eos = str + std::strlen(str);
  char *start = 0;
  char *end = 0;
  size_t n = 0;

  for (; str < eos; ++str) {
    while (*str == ' ' || *str == '\t') ++str; // skip white spaces
    bool inquote = false;
    if (*str == '"') {
      start = ++str;
      end = start;
      for (; str < eos; ++str) {
        if (*str == '"') {
          str++;
          if (*str != '"')
            break;
        }
        *end++ = *str;
      }
      inquote = true;
      str = std::find(str, eos, ',');
    } else {
      start = str;
      str = std::find(str, eos, ',');
      end = str;
    }
    if (max-- > 1) *end = '\0';
    *out++ = start;
    ++n;
    if (max == 0) break;
  }

  return n;
}

template <class Iterator>
inline size_t tokenize(char *str, const char *del,
                       Iterator out, size_t max) {
  char *stre = str + std::strlen(str);
  const char *dele = del + std::strlen(del);
  size_t size = 0;

  while (size < max) {
    char *n = std::find_first_of(str, stre, del, dele);
    *n = '\0';
    *out++ = str;
    ++size;
    if (n == stre) break;
    str = n + 1;
  }

  return size;
}

// continus run of space is regarded as one space
template <class Iterator>
inline size_t tokenize2(char *str, const char *del,
                        Iterator out, size_t max) {
  char *stre = str + std::strlen(str);
  const char *dele = del + std::strlen(del);
  size_t size = 0;

  while (size < max) {
    char *n = std::find_first_of(str, stre, del, dele);
    *n = '\0';
    if (*str != '\0') {
      *out++ = str;
      ++size;
    }
    if (n == stre) break;
    str = n + 1;
  }

  return size;
}
}
#endif
