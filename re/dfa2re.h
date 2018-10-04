#ifndef dfa2re_h_
#define dfa2re_h_

#include "libls/string_.h"
#include "dfa.h"

LSString
dfa2re(DFA dfa, const char *alphabet, const char *alphabet_end);

#endif
