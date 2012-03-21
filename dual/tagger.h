#ifndef DUAL_TAGGER_H_
#define DUAL_TAGGER_H_

#include <vector>

namespace CRFPP {
class Tagger;
}

namespace dual {
class PointwiseTagger {
 public:
  bool open(const char *model);
  void close();

  bool parse(const char *line, size_t len,
             std::vector<float> *score);

  static bool learn(const char *file,
                    const char *model,
                    const char *crfpp_param);

  explicit PointwiseTagger();
  virtual ~PointwiseTagger();

 private:
  CRFPP::Tagger *tagger_;
};
}
#endif
