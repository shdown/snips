#include "nfa_match.h"

#include <stdlib.h>
#include "libls/vector.h"

typedef struct {
    size_t state;
    const char *rest;
} State;

typedef LS_VECTOR_OF(State) Superposition;

bool
nfa_match(NFA nfa, const char *buf, const char *end)
{
    Superposition cur = LS_VECTOR_NEW();
    Superposition aux = LS_VECTOR_NEW();
    bool answer = false;

    LS_VECTOR_PUSH(cur, ((State) {nfa.start, buf}));

    do {
        for (size_t i = 0; i < cur.size; ++i) {
            State s = cur.data[i];
            if (s.state == nfa.terminal && s.rest == end) {
                answer = true;
                goto done;
            }
            if (s.rest != end) {
                const size_t jump = nfa.states.data[s.state].jumps[(unsigned char) *s.rest];
                if (jump != NFA_INVALID_INDEX) {
                    LS_VECTOR_PUSH(aux, ((State) {jump, s.rest + 1}));
                }
            }
            const size_t n_eps_jumps = nfa.states.data[s.state].eps_jumps.size;
            const size_t *eps_jumps = nfa.states.data[s.state].eps_jumps.data;
            for (size_t i = 0; i < n_eps_jumps; ++i) {
                LS_VECTOR_PUSH(aux, ((State) {eps_jumps[i], s.rest}));
            }
        }
        // swap /cur/ and /aux/
        Superposition tmp = cur;
        cur = aux;
        aux = tmp;
        // ...and clear /aux/
        LS_VECTOR_CLEAR(aux);
    } while (cur.size);
done:

    LS_VECTOR_FREE(cur);
    LS_VECTOR_FREE(aux);

    return answer;
}
