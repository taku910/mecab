// MeCab -- Yet Another Part-of-Speech and Morphological Analyzer
//
//
//  Copyright(C) 2001-2006 Taku Kudo <taku@chasen.org>
//  Copyright(C) 2004-2006 Nippon Telegraph and Telephone Corporation
#include <cstring>
#include <fstream>
#include <iostream>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#ifdef HAVE_DIRENT_H
#include <dirent.h>
#endif

#ifdef HAVE_WINDOWS_H
#define NOMINMAX
#include <windows.h>
#endif

#if defined(_WIN32) && !defined(__CYGWIN__)
extern HINSTANCE DllInstance;
#endif

#include "common.h"
#include "mecab.h"
#include "param.h"
#include "utils.h"

namespace MeCab {

#if defined(_WIN32) && !defined(__CYGWIN__)
std::wstring Utf8ToWide(const std::string &input) {
  int output_length = ::MultiByteToWideChar(CP_UTF8, 0,
                                            input.c_str(), -1, NULL, 0);
  output_length = output_length <= 0 ? 0 : output_length - 1;
  if (output_length == 0) {
    return L"";
  }
  scoped_array<wchar_t> input_wide(new wchar_t[output_length + 1]);
  const int result = ::MultiByteToWideChar(CP_UTF8, 0, input.c_str(), -1,
                                           input_wide.get(), output_length + 1);
  std::wstring output;
  if (result > 0) {
    output.assign(input_wide.get());
  }
  return output;
}

std::string WideToUtf8(const std::wstring &input) {
  const int output_length = ::WideCharToMultiByte(CP_UTF8, 0,
                                                  input.c_str(), -1, NULL, 0,
                                                  NULL, NULL);
  if (output_length == 0) {
    return "";
  }

  scoped_array<char> input_encoded(new char[output_length + 1]);
  const int result = ::WideCharToMultiByte(CP_UTF8, 0, input.c_str(), -1,
                                           input_encoded.get(),
                                           output_length + 1, NULL, NULL);
  std::string output;
  if (result > 0) {
    output.assign(input_encoded.get());
  }
  return output;
}
#endif

int decode_charset(const char *charset) {
  std::string tmp = charset;
  toLower(&tmp);
  if (tmp == "sjis"  || tmp == "shift-jis" ||
      tmp == "shift_jis" || tmp == "cp932")
    return CP932;
  else if (tmp == "euc"   || tmp == "euc_jp" ||
           tmp == "euc-jp")
    return EUC_JP;
  else if (tmp == "utf8" || tmp == "utf_8" ||
           tmp == "utf-8")
    return UTF8;
  else if (tmp == "utf16" || tmp == "utf_16" ||
           tmp == "utf-16")
    return UTF16;
  else if (tmp == "utf16be" || tmp == "utf_16be" ||
           tmp == "utf-16be")
    return UTF16BE;
  else if (tmp == "utf16le" || tmp == "utf_16le" ||
           tmp == "utf-16le")
    return UTF16LE;
  else if (tmp == "ascii")
    return ASCII;

  return UTF8;  // default is UTF8
}

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
  int len = static_cast<int>(s->size()) - 1;
  bool ok = false;
  for (; len >= 0; --len) {
#if defined(_WIN32) && !defined(__CYGWIN__)
    if ((*s)[len] == '\\') {
      ok = true;
      break;
    }
#else
    if ((*s)[len] == '/')  {
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
  int len = static_cast<int>(s->size()) - 1;
  bool ok = false;
  for (; len >= 0; --len) {
#if defined(_WIN32) && !defined(__CYGWIN__)
    if ((*s)[len] == '\\') {
      ok = true;
      break;
    }
#else
    if ((*s)[len] == '/')  {
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
  const std::string::size_type pos = s->find(src);
  if (pos != std::string::npos) {
    s->replace(pos, src.size(), dst);
  }
}

void enum_csv_dictionaries(const char *path,
                           std::vector<std::string> *dics) {
  dics->clear();

#if defined(_WIN32) && !defined(__CYGWIN__)
  WIN32_FIND_DATAW wfd;
  HANDLE hFind;
  const std::wstring pat = Utf8ToWide(create_filename(path, "*.csv"));
  hFind = ::FindFirstFileW(pat.c_str(), &wfd);
  CHECK_DIE(hFind != INVALID_HANDLE_VALUE)
      << "Invalid File Handle. Get Last Error reports";
  do {
    std::string tmp = create_filename(path, WideToUtf8(wfd.cFileName));
    dics->push_back(tmp);
  } while (::FindNextFileW(hFind, &wfd));
  ::FindClose(hFind);
#else
  DIR *dir = opendir(path);
  CHECK_DIE(dir) << "no such directory: " << path;

  for (struct dirent *dp = readdir(dir);
       dp;
       dp = readdir(dir)) {
    const std::string tmp = dp->d_name;
    if (tmp.size() >= 5) {
      std::string ext = tmp.substr(tmp.size() - 4, 4);
      toLower(&ext);
      if (ext == ".csv") {
        dics->push_back(create_filename(path, tmp));
      }
    }
  }
  closedir(dir);
#endif
}

bool toLower(std::string *s) {
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

  int cur_percentage  = static_cast<int>(100.0 * current/total);
  int bar_len = static_cast<int>(1.0 * current*scale/total);

  if (prev != cur_percentage) {
    printf("%s: %3d%% |%.*s%*s| ", message, cur_percentage,
           bar_len, bar, scale - bar_len, "");
    if (cur_percentage == 100)
      printf("\n");
    else
      printf("\r");
    fflush(stdout);
  }

  prev = cur_percentage;

  return 1;
}

int load_request_type(const Param &param) {
  int request_type = MECAB_ONE_BEST;

  if (param.get<bool>("allocate-sentence")) {
    request_type |= MECAB_ALLOCATE_SENTENCE;
  }

  if (param.get<bool>("partial")) {
    request_type |= MECAB_PARTIAL;
  }

  if (param.get<bool>("all-morphs")) {
    request_type |= MECAB_ALL_MORPHS;
  }

  if (param.get<bool>("marginal")) {
    request_type |= MECAB_MARGINAL_PROB;
  }

  const int nbest = param.get<int>("nbest");
  if (nbest >= 2) {
    request_type |= MECAB_NBEST;
  }

  // DEPRECATED:
  const int lattice_level = param.get<int>("lattice-level");
  if (lattice_level >= 1) {
    request_type |= MECAB_NBEST;
  }

  if (lattice_level >= 2) {
    request_type |= MECAB_MARGINAL_PROB;
  }

  return request_type;
}

bool load_dictionary_resource(Param *param) {
  std::string rcfile = param->get<std::string>("rcfile");

#ifdef HAVE_GETENV
  if (rcfile.empty()) {
    const char *homedir = getenv("HOME");
    if (homedir) {
      const std::string s = MeCab::create_filename(std::string(homedir),
                                                   ".mecabrc");
      std::ifstream ifs(WPATH(s.c_str()));
      if (ifs) {
        rcfile = s;
      }
    }
  }

  if (rcfile.empty()) {
    const char *rcenv = getenv("MECABRC");
    if (rcenv) {
      rcfile = rcenv;
    }
  }
#endif

#if defined (HAVE_GETENV) && defined(_WIN32) && !defined(__CYGWIN__)
  if (rcfile.empty()) {
    scoped_fixed_array<wchar_t, BUF_SIZE> buf;
    const DWORD len = ::GetEnvironmentVariableW(L"MECABRC",
                                                buf.get(),
                                                buf.size());
    if (len < buf.size() && len > 0) {
      rcfile = WideToUtf8(buf.get());
    }
  }
#endif

#if defined(_WIN32) && !defined(__CYGWIN__)
  HKEY hKey;
  scoped_fixed_array<wchar_t, BUF_SIZE> v;
  DWORD vt;
  DWORD size = v.size() * sizeof(v[0]);

  if (rcfile.empty()) {
    ::RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"software\\mecab", 0, KEY_READ, &hKey);
    ::RegQueryValueExW(hKey, L"mecabrc", 0, &vt,
                       reinterpret_cast<BYTE *>(v.get()), &size);
    ::RegCloseKey(hKey);
    if (vt == REG_SZ) {
      rcfile = WideToUtf8(v.get());
    }
  }

  if (rcfile.empty()) {
    ::RegOpenKeyExW(HKEY_CURRENT_USER, L"software\\mecab", 0, KEY_READ, &hKey);
    ::RegQueryValueExW(hKey, L"mecabrc", 0, &vt,
                       reinterpret_cast<BYTE *>(v.get()), &size);
    ::RegCloseKey(hKey);
    if (vt == REG_SZ) {
      rcfile = WideToUtf8(v.get());
    }
  }

  if (rcfile.empty()) {
    vt = ::GetModuleFileNameW(DllInstance, v.get(), size);
    if (vt != 0) {
      scoped_fixed_array<wchar_t, _MAX_DRIVE> drive;
      scoped_fixed_array<wchar_t, _MAX_DRIVE> dir;
      _wsplitpath(v.get(), drive.get(), dir.get(), NULL, NULL);
      const std::wstring path =
          std::wstring(drive.get()) + std::wstring(dir.get()) + L"mecabrc";
      if (::GetFileAttributesW(path.c_str()) != -1) {
        rcfile = WideToUtf8(path);
      }
    }
  }
#endif

  if (rcfile.empty()) {
    rcfile = MECAB_DEFAULT_RC;
  }

  if (!param->load(rcfile.c_str())) {
    return false;
  }

  std::string dicdir = param->get<std::string>("dicdir");
  if (dicdir.empty()) {
    dicdir = ".";  // current
  }
  remove_filename(&rcfile);
  replace_string(&dicdir, "$(rcpath)", rcfile);
  param->set<std::string>("dicdir", dicdir, true);
  dicdir = create_filename(dicdir, DICRC);

  if (!param->load(dicdir.c_str())) {
    return false;
  }

  return true;
}
}  // namespace MeCab
