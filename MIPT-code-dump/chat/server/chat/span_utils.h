#ifndef chat_span_utils_h_
#define chat_span_utils_h_

#include <string.h>
#include "common/span.h"
#include "libls/alloc_utils.h"
#include "libls/compdep.h"

// Makes a pair suitable for use with printf-like functions and the /%.*s/ format specifier; the size of the string as
// /int/, and the string itself.
#define SPANU_PAIR(S_) (int) (S_).size, (S_).data

LS_INHEADER Span spanu_dup(ConstSpan s) {
    return (Span) {ls_xmemdup(s.data, s.size), s.size};
}

LS_INHEADER ConstSpan spanu_const_from_s(const char *s) {
    return (ConstSpan) {s, strlen(s)};
}

#endif
