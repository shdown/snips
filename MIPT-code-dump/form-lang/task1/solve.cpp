#include "solve.h"

#include <stddef.h>
#include <limits.h>
#include <vector>
#include <string>
#include <utility>

static const char   NO_SYMBOL = '\0';
static const size_t NO_JUMP   = -1;

static const int INFINITY          = INT_MAX;
static const int MEMORY_NODATA     = -1;
static const int MEMORY_INPROGRESS = -2;

struct State {
    std::vector<size_t>     eps_jumps;
    std::pair<char, size_t> sym_jump;
};

static std::vector<State> states;

struct NFA {
    size_t start, finish;
};

static
size_t
new_state(
    size_t                  eps_jump = NO_JUMP,
    std::pair<char, size_t> sym_jump = {NO_SYMBOL, NO_JUMP})
{
    if (eps_jump == NO_JUMP) {
        states.emplace_back(State{{}, sym_jump});
    } else {
        states.emplace_back(State{{eps_jump}, sym_jump});
    }
    return states.size() - 1;
}

static
NFA
empty()
{
    size_t finish = new_state();
    size_t start  = new_state(finish);
    return NFA{start, finish};
}

static
NFA
symbol(char c)
{
    size_t finish = new_state();
    size_t start  = new_state(NO_JUMP, {c, finish});
    return NFA{start, finish};
}

static
void
juxtapose(NFA &left, NFA right)
{
    states[left.finish].eps_jumps.push_back(right.start);
    left.finish = right.finish;
}

static
void
alternate(NFA &a, NFA b)
{
    states[a.start].eps_jumps.push_back(b.start);
    states[a.finish].eps_jumps.push_back(b.finish);
    a.finish = b.finish;
}

static
void
iterate(NFA &a)
{
    states[a.start].eps_jumps.push_back(a.finish);
    states[a.finish].eps_jumps.push_back(a.start);
}

static inline
void
remin(int &answer, int summand, int result)
{
    if (result == INFINITY) {
        return;
    }
    result += summand;
    if (result < answer) {
        answer = result;
    }
}

static struct {
    std::vector<std::vector<int> > memory;
    size_t finish;
    char x;
} dfs_params;

static inline
int
dfs(size_t v, unsigned k)
{
    int &mem_ref = dfs_params.memory[v][k];
    if (mem_ref >= 0) {
        return mem_ref;
    } else if (mem_ref == MEMORY_INPROGRESS) {
        return INFINITY;
    } else {
        mem_ref = MEMORY_INPROGRESS;
    }

    int answer = INFINITY;
    if (k) {
        for (size_t jump : states[v].eps_jumps) {
            remin(answer, 0, dfs(jump, k));
        }
        if (states[v].sym_jump.first == dfs_params.x) {
            remin(answer, 1, dfs(states[v].sym_jump.second, k - 1));
        }

    } else {
        if (v == dfs_params.finish) {
            answer = 0;

        } else {
            for (size_t jump : states[v].eps_jumps) {
                remin(answer, 0, dfs(jump, 0));
            }
            if (states[v].sym_jump.first != NO_SYMBOL) {
                remin(answer, 1, dfs(states[v].sym_jump.second, 0));
            }
        }
    }

    mem_ref = answer;
    return answer;
}

static
NFA
parse_rpn(const std::string &rpn)
{
    std::vector<NFA> stack;

    for (char c : rpn) {
        switch (c) {
        case 'a':
        case 'b':
        case 'c':
            stack.emplace_back(symbol(c));
            break;
        case '1':
            stack.emplace_back(empty());
            break;
        case '.':
            {
                if (stack.size() < 2) {
                    throw InvalidExpression();
                }
                NFA arg = stack.back();
                stack.pop_back();
                juxtapose(stack.back(), arg);
            }
            break;
        case '+':
            {
                if (stack.size() < 2) {
                    throw InvalidExpression();
                }
                NFA arg = stack.back();
                stack.pop_back();
                alternate(stack.back(), arg);
            }
            break;
        case '*':
            if (stack.empty()) {
                throw InvalidExpression();
            }
            iterate(stack.back());
            break;
        default:
            throw InvalidExpression();
        }
    }

    if (stack.size() != 1) {
        throw InvalidExpression();
    }
    return stack[0];
}

static
int
solve_for_nfa(NFA nfa, char x, unsigned k)
{
    dfs_params.memory.resize(states.size(), std::vector<int>(k + 1, MEMORY_NODATA));
    dfs_params.finish = nfa.finish;
    dfs_params.x = x;
    const int answer = dfs(nfa.start, k);
    return answer == INFINITY ? -1 : answer;
}

int
solve(const std::string &rpn, char x, unsigned k)
{
    return solve_for_nfa(parse_rpn(rpn), x, k);
}
