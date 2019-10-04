#ifndef common_span_h_
#define common_span_h_

#include <stddef.h>
#include <string.h>
#include <stdbool.h>
#include "libls/compdep.h"

typedef struct {
    char *data;
    size_t size;
} Span;

typedef struct {
    const char *data;
    size_t size;
} ConstSpan;

LS_INHEADER ConstSpan span_to_const(Span s) {
    return (ConstSpan) {.data = s.data, .size = s.size};
}

LS_INHEADER bool span_equal(ConstSpan a, ConstSpan b) {
    return a.size == b.size && (!a.size || memcmp(a.data, b.data, a.size) == 0);
}

#endif
