#include "dfa_complement.h"

#include <stdlib.h>

void
dfa_complement(DFA *dfa)
{
    const size_t nstates = dfa->states.size;
    DFA_State *states = dfa->states.data;
    for (size_t i = 0; i < nstates; ++i) {
        states[i].terminal = !states[i].terminal;
    }
}
