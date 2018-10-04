#include "nfa.h"

void
nfa_free(NFA nfa)
{
    for (size_t i = 0; i < nfa.states.size; ++i) {
        LS_VECTOR_FREE(nfa.states.data[i].eps_jumps);
    }
    LS_VECTOR_FREE(nfa.states);
}
