#ifndef dfa_h_
#define dfa_h_

#include <stdbool.h>
#include <stdlib.h>
#include <limits.h>
#include "libls/vector.h"

#define NFA_INVALID_INDEX ((size_t) -1)

typedef struct {
    size_t jumps[1 << CHAR_BIT];
    bool terminal;
} DFA_State;

typedef LS_VECTOR_OF(DFA_State) DFA_State_vector;

typedef struct {
    DFA_State_vector states;
    size_t start;
} DFA;

#define dfa_free(Dfa_) LS_VECTOR_FREE((Dfa_).states)

#endif
