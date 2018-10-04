#include "re2nfa.h"

#include "libls/vector.h"
#include <string.h>

#define ARRAY_SIZE(A_) (sizeof(A_) / sizeof(A_[0]))

#define add_eps_jump(From_, To_) LS_VECTOR_PUSH(arena.data[From_].eps_jumps, To_)

static NFA_State_vector arena;

typedef struct {
    size_t start, terminal;
} Temp_NFA;

static
size_t
alloc_state(void)
{
    LS_VECTOR_ENSURE(arena, arena.size + 1);
    const size_t index = arena.size++;
    NFA_State *s = &arena.data[index];
    memset(s->jumps, (unsigned char) -1, sizeof(s->jumps));
    LS_VECTOR_INIT(s->eps_jumps);
    return index;
}

#define error() ((Temp_NFA) {-1, -1})

#define has_error(TempNfa_) ((TempNfa_).start == (size_t) -1)

static inline
Temp_NFA
letter(unsigned char c)
{
    const size_t a = alloc_state();
    const size_t b = alloc_state();
    arena.data[a].jumps[c] = b;
    return (Temp_NFA) {a, b};
}

static inline
Temp_NFA
empty(void)
{
    const size_t a = alloc_state();
    const size_t b = alloc_state();
    add_eps_jump(a, b);
    return (Temp_NFA) {a, b};
}

static inline
Temp_NFA
any(void)
{
    const size_t a = alloc_state();
    const size_t b = alloc_state();
    for (unsigned i = 0; i < ARRAY_SIZE(arena.data[a].jumps); ++i) {
        arena.data[a].jumps[i] = b;
    }
    return (Temp_NFA) {a, b};
}

static
const char *
pb_find(const char *buf, const char *end, char c)
{
    int balance = 0;
    for (; buf != end; ++buf) {
        switch (*buf) {
            case '(': ++balance; break;
            case ')': if (--balance < 0) { return NULL; } break;
        }
        if (balance == 0 && *buf == c) {
            return buf;
        }
    }
    return NULL;
}

static
Temp_NFA
re2temp(const char *buf, const char *end)
{
    if (buf == end) {
        return empty();
    }

    const char *bar = pb_find(buf, end, '|');
    if (bar) {
        Temp_NFA left = re2temp(buf, bar);
        Temp_NFA right = re2temp(bar + 1, end);
        if (has_error(left) || has_error(right)) { return error(); }

        add_eps_jump(left.start, right.start);
        add_eps_jump(right.terminal, left.terminal);
        return (Temp_NFA) {left.start, left.terminal};
    }

    Temp_NFA left;
    const char *left_end;

    if (buf[0] == '(') {
        const char *p = pb_find(buf, end, ')');
        if (!p) { return error(); }
        left = re2temp(buf + 1, p);
        if (has_error(left)) { return error(); }
        left_end = p + 1;

    } else if (buf[0] == ')' || buf[0] == '*' || buf[0] == '+') {
        return error();

    } else if (buf[0] == '.') {
        left = any();
        left_end = buf + 1;

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
            add_eps_jump(left.terminal, left.start);

        } else if (*left_end == '*') {
            add_eps_jump(left.terminal, left.start);
            add_eps_jump(left.start, left.terminal);

        } else if (*left_end == '?') {
            add_eps_jump(left.start, left.terminal);

        } else {
            break;
        }
    }

    if (left_end == end) {
        return left;
    } else {
        Temp_NFA right = re2temp(left_end, end);
        if (has_error(right)) { return error(); }

        add_eps_jump(left.terminal, right.start);
        return (Temp_NFA) {left.start, right.terminal};
    }
}

NFA
re2nfa(const char *buf, const char *end)
{
    LS_VECTOR_INIT(arena);
    Temp_NFA t = re2temp(buf, end);
    return (NFA) {.states = arena, .start = t.start, .terminal = t.terminal};
}
