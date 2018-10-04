#ifndef dfa_match_h_
#define dfa_match_h_

#include "dfa.h"

#include <stdbool.h>

bool
dfa_match(DFA dfa, const char *buf, const char *end);

#endif
