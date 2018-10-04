#include "dfa2re.h"
#include <assert.h>
#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdlib.h>

typedef enum {
    NK_ZERO,
    NK_ONE,
    NK_LETTER,
    NK_STAR,
    NK_JUX,
    NK_ALT,
} NodeKind;

typedef struct Node {
    unsigned nrefs;
    unsigned char kind;

    char letter;
    struct Node *child;
    struct Node *right;
} Node;

typedef Node *PNODE;

static inline
void
incref(PNODE pn)
{
    ++pn->nrefs;
}

static inline
void
decref(PNODE pn)
{
    if (!--pn->nrefs) {
        if (pn->child) {
            decref(pn->child);
        }
        if (pn->right) {
            decref(pn->right);
        }
        free(pn);
    }
}

static inline
void
move(PNODE *dst, PNODE src)
{
    if (*dst != src) {
        decref(*dst);
        *dst = src;
    }
}

#define MK_NODE(...) ((PNODE) ls_xmemdup((Node [1]) {{__VA_ARGS__, .nrefs = 1}}, sizeof(Node)))

#define mk_zero() MK_NODE(.kind = NK_ZERO)
#define mk_one() MK_NODE(.kind = NK_ONE)
#define mk_letter(C_) MK_NODE(.kind = NK_LETTER, .letter = C_)

static inline
PNODE
mk_star(PNODE child)
{
    switch (child->kind) {
    case NK_ZERO:
    case NK_ONE:
    case NK_STAR:
        incref(child);
        return child;
    default:
        incref(child);
        return MK_NODE(.kind = NK_STAR, .child = child);
    }
}

static inline
PNODE
mk_jux(PNODE left, PNODE right)
{
    if (left->kind == NK_ZERO) {
        incref(left);
        return left;
    }
    if (right->kind == NK_ZERO) {
        incref(right);
        return right;
    }

    if (left->kind == NK_ONE) {
        incref(right);
        return right;
    }
    if (right->kind == NK_ONE) {
        incref(left);
        return left;
    }

    incref(right);
    incref(left);
    return MK_NODE(.kind = NK_JUX, .child = left, .right = right);
}

static
bool
is_certainly_superset(PNODE haystack, PNODE needle)
{
    if (haystack == needle || needle->kind == NK_ZERO) {
        return true;
    }
    switch (haystack->kind) {
    case NK_ZERO: return false;
    case NK_ONE: return needle->kind == NK_ONE;
    case NK_LETTER:
        return
            needle->kind == NK_LETTER &&
            needle->letter == haystack->letter;
    case NK_STAR:
        return
            is_certainly_superset(haystack->child, needle) || (
                needle->kind == NK_STAR &&
                is_certainly_superset(haystack->child, needle->child));
    default:
        return false;
    }
}

static inline
PNODE
mk_alt(PNODE left, PNODE right)
{
    if (is_certainly_superset(left, right)) {
        incref(left);
        return left;
    }
    if (is_certainly_superset(right, left)) {
        incref(right);
        return right;
    }

    incref(left);
    incref(right);
    return MK_NODE(.kind = NK_ALT, .child = left, .right = right);
}

static
void
construct_re(PNODE pn, LSString *out)
{
    switch (pn->kind) {
    case NK_ZERO: ls_string_append_s(out, "∅"); break;
    case NK_ONE: break;
    case NK_LETTER: ls_string_append_c(out, pn->letter); break;

    case NK_STAR:
        {
            const bool paren = pn->child->kind != NK_LETTER;
            if (paren) {
                ls_string_append_c(out, '(');
            }
            construct_re(pn->child, out);
            if (paren) {
                ls_string_append_c(out, ')');
            }
            ls_string_append_c(out, '*');
        }
        break;

    case NK_JUX:
        // left
        {
            const bool paren = pn->child->kind == NK_ALT;
            if (paren) {
                ls_string_append_c(out, '(');
            }
            construct_re(pn->child, out);
            if (paren) {
                ls_string_append_c(out, ')');
            }
        }
        // right
        {
            const bool paren = pn->right->kind == NK_ALT;
            if (paren) {
                ls_string_append_c(out, '(');
            }
            construct_re(pn->right, out);
            if (paren) {
                ls_string_append_c(out, ')');
            }
        }
        break;

    case NK_ALT:
        construct_re(pn->child, out);
        ls_string_append_c(out, '|');
        construct_re(pn->right, out);
        break;

    default:
        LS_UNREACHABLE();
    }
}

LSString
dfa2re(DFA dfa, const char *alphabet, const char *alphabet_end)
{
    const size_t n = dfa.states.size;

    PNODE ***r = LS_XNEW(PNODE **, n);
    // allocate + initialize
    for (size_t i = 0; i < n; ++i) {
        r[i] = LS_XNEW(PNODE *, n);
        for (size_t j = 0; j < n; ++j) {
            r[i][j] = LS_XNEW(PNODE, /*!!!*/ n + 1);
            r[i][j][0] = (i == j) ? mk_one() : mk_zero();

            for (const char *ptr = alphabet; ptr != alphabet_end; ++ptr) {
                const unsigned char c = *ptr;
                if (dfa.states.data[i].jumps[c] == j) {
                    PNODE letter = mk_letter(c);
                    PNODE alt = mk_alt(r[i][j][0], letter);
                    move(&r[i][j][0], alt);
                    decref(letter);
                }
            }
        }
    }

    // find the transitive closure
    for (size_t k = 1; k <= n; ++k) {
        for (size_t i = 0; i < n; ++i) {
            for (size_t j = 0; j < n; ++j) {
                PNODE star = mk_star(r[k - 1][k - 1][k - 1]);
                PNODE jux1 = mk_jux(r[i][k - 1][k - 1], star);
                PNODE jux2 = mk_jux(jux1, r[k - 1][j][k - 1]);
                r[i][j][k] = mk_alt(r[i][j][k - 1], jux2);
                decref(jux2);
                decref(jux1);
                decref(star);
            }
        }
    }

    // assemble the result
    PNODE e = mk_zero();
    for (size_t i = 0; i < n; ++i) {
        if (dfa.states.data[i].terminal) {
            PNODE alt = mk_alt(e,  r[dfa.start][i][n]);
            move(&e, alt);
        }
    }

    LSString result = LS_VECTOR_NEW();
    construct_re(e, &result);
    decref(e);
    return result;
}
