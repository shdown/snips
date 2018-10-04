#ifndef nfa_h_
#define nfa_h_

#include <stdlib.h>
#include <limits.h>
#include "libls/vector.h"

#define NFA_INVALID_INDEX ((size_t) -1)

#define nfa_has_error(Nfa_) ((Nfa_).start == NFA_INVALID_INDEX)

typedef struct NFA_State {
    size_t jumps[1 << CHAR_BIT];
    LS_VECTOR_OF(size_t) eps_jumps;
} NFA_State;

typedef LS_VECTOR_OF(NFA_State) NFA_State_vector;

typedef struct {
    NFA_State_vector states;
    size_t start, terminal;
} NFA;

void
nfa_free(NFA nfa);

#endif
