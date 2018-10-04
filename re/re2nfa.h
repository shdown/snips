#ifndef re2nfa_h_
#define re2nfa_h_

#include <stdbool.h>
#include "nfa.h"

NFA
re2nfa(const char *buf, const char *end);

#endif
