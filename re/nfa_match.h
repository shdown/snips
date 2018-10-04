#ifndef nfa_match_h_
#define nfa_match_h_

#include <stdbool.h>
#include "nfa.h"

bool
nfa_match(NFA nfa, const char *buf, const char *end);

#endif
