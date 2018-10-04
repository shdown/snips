#include "nfa2dfa.h"

#include <assert.h>
#include "zu_map.h"

static size_t eps_closure_result;
static NFA topic;

static inline
void
eps_closure_dfs(size_t i)
{
    const size_t m = 1ULL << i;
    if (eps_closure_result & m) {
        return;
    }
    eps_closure_result |= m;
    const size_t n_eps_jumps = topic.states.data[i].eps_jumps.size;
    const size_t *eps_jumps = topic.states.data[i].eps_jumps.data;
    for (size_t i = 0; i < n_eps_jumps; ++i) {
        eps_closure_dfs(eps_jumps[i]);
    }
}

static inline
size_t
eps_closure_single(size_t i)
{
    eps_closure_result = 0;
    eps_closure_dfs(i);
    return eps_closure_result;
}

static inline
size_t
eps_closure_mask(size_t mask)
{
    eps_closure_result = 0;
    for (unsigned i = 0; mask; mask >>= 1, ++i) {
        if (mask & 1) {
            eps_closure_dfs(i);
        }
    }
    return eps_closure_result;
}

static inline
size_t
move(size_t mask, unsigned char c)
{
    size_t result = 0;
    for (unsigned i = 0; mask; mask >>= 1, ++i) {
        if (mask & 1) {
            const size_t jump = topic.states.data[i].jumps[c];
            if (jump != NFA_INVALID_INDEX) {
                result |= 1ULL << jump;
            }
        }
    }
    return result;
}

DFA
nfa2dfa(NFA nfa, const char *alphabet, const char *alphabet_end)
{
    assert(nfa.states.size <= 64);

    topic = nfa;

    const size_t start_set = eps_closure_single(nfa.start);
    const size_t terminal_mask = 1ULL << nfa.terminal;

    DFA result = {.states = LS_VECTOR_NEW_RESERVE(DFA_State, 1), .start = 0};
    ++result.states.size;

    LS_VECTOR_OF(size_t) queue = LS_VECTOR_NEW();
    LS_VECTOR_PUSH(queue, start_set);

    Zu_Map *map = zu_map_new();
    zu_map_set(map, start_set, 0);

    do {
        const size_t set = queue.data[--queue.size];
        const size_t i = zu_map_get(map, set);
        result.states.data[i].terminal = !!(set & terminal_mask);
        for (const char *s = alphabet; s != alphabet_end; ++s) {
            const unsigned char c = *s;
            const size_t dest = eps_closure_mask(move(set, c));
            const size_t j = zu_map_get_or_set(map, dest, result.states.size);
            if (j == result.states.size) {
                LS_VECTOR_ENSURE(result.states, result.states.size + 1);
                ++result.states.size;
                LS_VECTOR_PUSH(queue, dest);
            }
            result.states.data[i].jumps[(unsigned char) c] = j;
        }
    } while (queue.size);

    zu_map_free(map);
    LS_VECTOR_FREE(queue);
    return result;
}
