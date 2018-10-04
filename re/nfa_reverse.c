#include "nfa_reverse.h"

void
nfa_reverse(NFA *nfa)
{
    const size_t n = nfa->states.size;

    LS_VECTOR_PUSH(nfa->states, ((NFA_State) {}));

    for (size_t i = 0; i < n; ++i) {
        NFA_State *s = nfa->states.data[i];
        for (size_t j = i + 1; j < sizeof(s->jumps) / sizeof(s->jumps[0]); ++j) {
            const size_t c = s->jumps[j];
            if (c != NFA_INVALID_INDEX) {
                s->jumps[j] = NFA_INVALID_INDEX;
                nfa->states.data[j].jumps[c] = i;
            }
        }
    }
}
