#include "dfa_match.h"

#include <stdlib.h>

bool
dfa_match(DFA dfa, const char *buf, const char *end)
{
    size_t state = dfa.start;
    for (; buf != end; ++buf) {
        state = dfa.states.data[state].jumps[(unsigned char) *buf];
        if (state == NFA_INVALID_INDEX) {
            return false;
        }
    }
    return dfa.states.data[state].terminal;
}
