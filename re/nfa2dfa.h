#ifndef nfa2dfa_h_
#define nfa2dfa_h_

#include "dfa.h"
#include "nfa.h"

DFA
nfa2dfa(NFA nfa, const char *alphabet, const char *alphabet_end);

#endif
