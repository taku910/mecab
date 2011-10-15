//  MeCab -- Yet Another Part-of-Speech and Morphological Analyzer
//
//  $Id: mempool.h 173 2009-04-18 08:10:57Z taku-ku $;
//
//  Copyright(C) 2001-2006 Taku Kudo <taku@chasen.org>
//  Copyright(C) 2004-2006 Nippon Telegraph and Telephone Corporation
#ifndef MECAB_MEM_POOL_H
#define MECAB_MEM_POOL_H

#include <map>
#ifndef MECAB_WITHOUT_SHARE_DIC

#include "mutex.h"

#define MMAP_OPEN(type, map, file, mode) do {                   \
    MemoryPool<std::string, Mmap<type> >& pool__ =              \
        getMemoryPool<std::string, Mmap<type> >();              \
    map = pool__.get((file));                                   \
    pool__.lock();                                              \
    if (map->begin() == 0 && !map->open(file.c_str(), mode)) {  \
      WHAT << map->what();                                      \
      close();                                                  \
      pool__.unlock();                                          \
      return false;                                             \
    }                                                           \
    pool__.unlock();                                            \
  } while (0)

#define MMAP_CLOSE(type, map) do {                      \
    MemoryPool<std::string, Mmap<type> >& pool__ =      \
        getMemoryPool<std::string, Mmap<type> >();      \
    pool__.release(map);                                \
    map = 0; } while (0)

namespace MeCab {

template <typename _Key, typename _Value> class MemoryPool {
 private:
  std::map<_Key, _Value*> pool_;
  std::map<_Value*, std::pair<_Key, size_t> > rpool_;
  Mutex mutex_;

 public:
  typedef _Key   key_type;
  typedef _Value value_type;

  explicit MemoryPool() {}

  virtual ~MemoryPool() {
    mutex_.lock();
    for (typename std::map<_Key, _Value*>::iterator it = pool_.begin();
         it != pool_.end(); it++)
      delete it->second;
    mutex_.unlock();
  }

  void lock() { mutex_.lock(); }
  void unlock() { mutex_.unlock(); }

  value_type* get(const key_type &key) {
    mutex_.lock();

    typename std::map<_Key, _Value*>::iterator it = pool_.find(key);
    _Value* m = 0;

    if (it == pool_.end()) {
      m = new _Value;
      pool_.insert(std::make_pair(key, m));
      rpool_[m] = std::make_pair(key, static_cast<size_t>(1));
    } else {
      m = it->second;
      rpool_[m].second++;  // reference count;
    }

    mutex_.unlock();

    return m;
  }

  void release(value_type *m = 0) {
    mutex_.lock();

    if (m) {
      typename std::map<_Value*, std::pair<_Key, size_t> >::iterator
          it = rpool_.find(m);
      if (it != rpool_.end()) {
        --it->second.second;
        if (it->second.second == 0) {
          typename std::map<_Key, _Value*>::iterator it2 =
              pool_.find(it->second.first);
          pool_.erase(it2);
          rpool_.erase(it);
          delete m;
          m = 0;
        }
      }
    }

    mutex_.unlock();
  }
};

template <class _Key, class _Value> MemoryPool<_Key, _Value>&
getMemoryPool() {
  static MemoryPool<_Key, _Value> mempool;
  return mempool;
}
}

#else

#define MMAP_OPEN(type, map, file) do {         \
    map = new Mmap<type>;                       \
    if (!map->open(file.c_str())) {             \
      WHAT(map->what());                        \
      close();                                  \
      return false;                             \
    }                                           \
  } while (0)

#define MMAP_CLOSE(type, map) do {              \
    delete map; } while (0)

#endif
#endif
