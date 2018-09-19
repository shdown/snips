#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "libls/vector.h"
#include "libls/alloc_utils.h"

typedef struct State {
    struct State *jump[256];
    LS_VECTOR_OF(struct State *) eps_jumps;
} State;

typedef LS_VECTOR_OF(State *) StateList;

static inline
void
traverse(State *s, StateList *result, StateList *p_aux)
{
    const size_t aux_ignore_prefix = p_aux->size;
    for (size_t i = 0; i < 256; ++i) {
        if (s->jump[i]) {
            LS_VECTOR_PUSH(*p_aux, s->jump[i]);
            s->jump[i] = NULL;
        }
    }
    for (size_t i = 0; i < s->eps_jumps.size; ++i) {
        LS_VECTOR_PUSH(*p_aux, s->eps_jumps.data[i]);
    }
    LS_VECTOR_FREE(s->eps_jumps);
    LS_VECTOR_INIT(s->eps_jumps);

    if (p_aux->size != aux_ignore_prefix) {
        LS_VECTOR_PUSH(*result, s);
        for (size_t i = aux_ignore_prefix; i < p_aux->size; ++i) {
            traverse(p_aux->data[i], result, p_aux);
        }
        p_aux->size = aux_ignore_prefix;
    }
}

static
void
free_reachable_but_final(State *s)
{
    StateList result = LS_VECTOR_NEW();
    StateList aux = LS_VECTOR_NEW();
    traverse(s, &result, &aux);
    for (size_t i = 0; i < result.size; ++i) {
        free(result.data[i]);
    }
    LS_VECTOR_FREE(result);
    LS_VECTOR_FREE(aux);
}

static inline
State *
zalloc_state(void)
{
    return LS_XNEW0(State, 1);
}

static inline
void
add_eps_jump(State *from, State *to)
{
    LS_VECTOR_PUSH(from->eps_jumps, to);
}

typedef struct {
    State *start;
    State *final;
} Automata;

void
automata_free(Automata a)
{
    free_reachable_but_final(a.start);
    free(a.final);
}

#define has_error(A_) (!(A_).start)

static inline
Automata
error(void)
{
    return (Automata) {NULL, NULL};
}

static inline
Automata
empty(void)
{
    State *start = zalloc_state();
    State *final = zalloc_state();
    add_eps_jump(start, final);
    return (Automata) {start, final};
}

static inline
Automata
letter(unsigned char c)
{
    State *start = zalloc_state();
    State *final = zalloc_state();
    start->jump[c] = final;
    return (Automata) {start, final};
}

Automata
parse_re(const char *buf, const char *end)
{

    if (buf == end) {
        return empty();
    }

    Automata left;
    const char *left_end;

    if (buf[0] == '(') {
        int balance = 1;
        const char *p = buf + 1;

        while (1) {
            switch (*p) {
            case '\\':  if (++p == end) { return error(); } break;
            case '(':   ++balance; break;
            case ')':   --balance; break;
            }
            if (balance == 0) { break; }
            if (++p == end) { return error(); }
        }

        left = parse_re(buf + 1, p);
        left_end = p + 1;
        if (has_error(left)) { return error(); }

    } else if (buf[0] == ')') {
        return error();

    } else if (buf[0] == '|') {
        left = empty();
        left_end = buf;

    } else if (buf[0] == '*' || buf[0] == '+') {
        return error();

    } else {
        const char *p;
        if (buf[0] == '\\') {
            p = buf + 1;
            if (p == end) {
                return error();
            }
        } else {
            p = buf;
        }
        left = letter(*p);
        left_end = p + 1;
    }

    for (; left_end != end; ++left_end) {
        if (*left_end == '+') {
            add_eps_jump(left.final, left.start);

        } else if (*left_end == '*') {
            add_eps_jump(left.final, left.start);
            add_eps_jump(left.start, left.final);

        } else if (*left_end == '?') {
            add_eps_jump(left.start, left.final);

        } else {
            break;
        }
    }

    if (left_end == end) {
        return left;

    } else if (*left_end == '|') {
        Automata right = parse_re(left_end + 1, end);
        if (has_error(right)) { return error(); }

        State *start = zalloc_state();
        State *final = zalloc_state();

        add_eps_jump(start, left.start);
        add_eps_jump(start, right.start);

        add_eps_jump(left.final, final);
        add_eps_jump(right.final, final);

        return (Automata) {start, final};

    } else {
        Automata right = parse_re(left_end, end);
        if (has_error(right)) { return error(); }

        add_eps_jump(left.final, right.start);
        return (Automata) {left.start, right.final};
    }
}

typedef struct {
    State *state;
    const char *prefix;
} SuperState;

typedef LS_VECTOR_OF(SuperState) SuperStateList;

static inline
void
superstate_list_swap(SuperStateList *a, SuperStateList *b)
{
    SuperStateList tmp = *a;
    *a = *b;
    *b = tmp;
}

bool
match(Automata a, const char *buf, const char *end)
{
    SuperStateList superstates = LS_VECTOR_NEW();
    SuperStateList aux = LS_VECTOR_NEW();

    LS_VECTOR_PUSH(superstates, ((SuperState) {a.start, buf}));

    bool ans;

    for (;; ++buf) {
        if (!superstates.size) {
            ans = false;
            goto done;
        }
        for (size_t i = 0; i < superstates.size; ++i) {
            SuperState s = superstates.data[i];
            if (s.state == a.final && s.prefix == end) {
                ans = true;
                goto done;
            }
            if (s.prefix != end) {
                const unsigned char c = *s.prefix;
                State *to = s.state->jump[c];
                if (to) {
                    LS_VECTOR_PUSH(aux, ((SuperState) {to, s.prefix + 1}));
                }
            }
            for (size_t i = 0; i < s.state->eps_jumps.size; ++i) {
                LS_VECTOR_PUSH(aux, ((SuperState) {s.state->eps_jumps.data[i], s.prefix}));
            }
        }
        superstate_list_swap(&superstates, &aux);
        LS_VECTOR_CLEAR(aux);
    }

done:
    LS_VECTOR_FREE(superstates);
    LS_VECTOR_FREE(aux);
    return ans;
}

int
main()
{
    char re[1024], str[1024];
    if (!fgets(re, sizeof(re), stdin)) {
        return 1;
    }
    if (!fgets(str, sizeof(str), stdin)) {
        return 1;
    }

    Automata a = parse_re(re, re + strlen(re));
    if (has_error(a)) {
        fputs("Invalid regex.\n", stderr);
        return 1;
    }
    const bool m = match(a, str, str + strlen(str));
    puts(m ? "YES" : "NO");

    automata_free(a);
}
