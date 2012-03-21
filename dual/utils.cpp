#include <iostream>
#include <fstream>
#include <cstring>
#include "common.h"
#include "utils.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

namespace mozilozy {

  std::string create_filename(const std::string &path,
                              const std::string &file) {
    std::string s = path;
#if defined(_WIN32) && !defined(__CYGWIN__)
    if (s.size() && s[s.size()-1] != '\\') s += '\\';
#else
    if (s.size() && s[s.size()-1] != '/') s += '/';
#endif
    s += file;
    return s;
  }

  void remove_filename(std::string *s) {
    int len = static_cast<int> (s->size() - 1);
    bool ok = false;
    for (; len >= 0; --len) {
#if defined(_WIN32) && !defined(__CYGWIN__)
      if ((*s)[len] == '\\') {
        ok = true;
        break;
      }
#else
      if ((*s)[len] == '/') {
        ok = true;
        break;
      }
#endif
    }
    if (ok)
      *s = s->substr(0, len);
    else
      *s = ".";
  }

  void remove_pathname(std::string *s) {
    int len = static_cast<int> (s->size() - 1);
    bool ok = false;
    for (; len >= 0; --len) {
#if defined(_WIN32) && !defined(__CYGWIN__)
      if ((*s)[len] == '\\') {
        ok = true;
        break;
      }
#else
      if ((*s)[len] == '/') {
        ok = true;
        break;
      }
#endif
    }
    if (ok)
      *s = s->substr(len + 1, s->size() - len);
    else
      *s = ".";
  }

  void replace_string(std::string *s,
                      const std::string &src,
                      const std::string &dst) {
    size_t pos = s->find(src);
    if (pos != std::string::npos)
      s->replace(pos, src.size(), dst);
  }

  bool to_lower(std::string *s) {
    for (size_t i = 0; i < s->size(); ++i) {
      char c = (*s)[i];
      if ((c >= 'A') && (c <= 'Z')) {
        c += 'a' - 'A';
        (*s)[i] = c;
      }
    }
    return true;
  }

  bool escape_csv_element(std::string *w) {
    if (w->find(',') != std::string::npos ||
        w->find('"') != std::string::npos) {
      std::string tmp = "\"";
      for (size_t j = 0; j < w->size(); j++) {
        if ((*w)[j] == '"') tmp += '"';
        tmp += (*w)[j];
      }
      tmp += '"';
      *w = tmp;
    }
    return true;
  }

  int progress_bar(const char* message, size_t current, size_t total) {
    static char bar[] = "###########################################";
    static int scale = sizeof(bar) - 1;
    static int prev = 0;

    int cur_percentage  = static_cast<int> (100.0 * current/total);
    int bar_len = static_cast<int> (1.0 * current*scale/total);

    if (prev != cur_percentage) {
      printf("%s: %3d%% |%.*s%*s| ",
             message, cur_percentage, bar_len, bar, scale - bar_len, "");
      if (cur_percentage == 100)
        printf("\n");
      else
        printf("\r");
      fflush(stdout);
    }

    prev = cur_percentage;

    return 1;
  }
}
