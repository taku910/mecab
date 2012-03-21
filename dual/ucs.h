#include "ctype.h"

namespace dual {
const char utf8_to_ucs2(const char *begin,
                        const char *end,
                        size_t *mblen) {
  const size_t len = end - begin;

  if (static_cast<unsigned char>(begin[0]) < 0x80) {
    *mblen = 1;
    return ctype_table[static_cast<unsigned char>(begin[0])];

  } else if (len >= 2 && (begin[0] & 0xe0) == 0xc0) {
    *mblen = 2;
    return ctype_table[((begin[0] & 0x1f) << 6) | (begin[1] & 0x3f)];

  } else if (len >= 3 && (begin[0] & 0xf0) == 0xe0) {
    *mblen = 3;
    return ctype_table[((begin[0] & 0x0f) << 12) |
                       ((begin[1] & 0x3f) << 6) | (begin[2] & 0x3f)];

  } else if (len >= 4 && (begin[0] & 0xf8) == 0xf0) {
    *mblen = 4;
    return 'u';

  } else if (len >= 5 && (begin[0] & 0xfc) == 0xf8) {
    *mblen = 5;
    return 'u';

  } else if (len >= 6 && (begin[0] & 0xfe) == 0xfc) {
    *mblen = 6;
    return 'u';
  }

  *mblen = 1;
  return 'u';
}
}
