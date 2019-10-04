#include "solve.h"
#include <string>
#include <exception>
#include <stdlib.h>

struct Input {
    std::string rpn;
    char x;
    unsigned k;
};

static
void
print_status(const Input &input)
{
    fprintf(stderr, "Testing: %s %c %u\n", input.rpn.c_str(), input.x, input.k);
}

static
void
expect_result(const Input &input, int answer)
{
    print_status(input);
    try {
        const int found = solve(input.rpn, input.x, input.k);
        if (found != answer) {
            fprintf(stderr, "FAIL\n");
            fprintf(stderr, "EXPECTED:\n%d\n", answer);
            fprintf(stderr, "FOUND:\n%d\n", found);
            exit(1);
        }
    } catch (...) {
        fprintf(stderr, "FAIL\n");
        fprintf(stderr, "EXPECTED:\n%d\n", answer);
        fprintf(stderr, "FOUND:\nfollowing exception:\n");
        throw;
    }
}

static
void
expect_error(const Input &input)
{
    print_status(input);
    try {
        const int found = solve(input.rpn, input.x, input.k);
        fprintf(stderr, "FAIL\n");
        fprintf(stderr, "EXPECTED:\nInvalidExpression exception\n");
        fprintf(stderr, "FOUND:\nanswer %d\n", found);
        exit(1);
    } catch (InvalidExpression &) {
    } catch (...) {
        fprintf(stderr, "FAIL\n");
        fprintf(stderr, "EXPECTED:\nInvalidExpression exception\n");
        fprintf(stderr, "FOUND:\nfollowing exception:\n");
        throw;
    }
}

int
main()
{
    const int INF = -1;

    expect_result({"ab+c.aba.*.bac.+.+*", 'c', 4}, INF);
    expect_result({"acb..bab.c.*.ab.ba.+.+*a.", 'b', 2}, 4);

    expect_result({"a*", 'a', 247}, 247);
    expect_result({"a*****", 'a', 247}, 247);
    expect_result({"a*", 'b', 1}, INF);
    expect_result({"a*", 'b', 0}, 0);

    expect_result({"ab.*", 'a', 1}, 2);
    expect_result({"ab.*", 'a', 2}, INF);
    expect_result({"ba.*", 'a', 1}, INF);

    expect_result({"aa.b.a.b.a+*", 'a', 5733}, 5733);
    expect_result({"aa.b.a.b.c+*", 'a', 5733}, INF);
    expect_result({"aa.b.a.b.c+*", 'a', 2}, 5);

    expect_result({"cc.a.b.a.c.c.c.bc*.+", 'c', 1}, 8);
    expect_result({"cc.a.b.a.c.c.c.bc*.+", 'c', 2}, 8);
    expect_result({"cc.a.b.a.c.c.c.bc*.+", 'c', 3}, INF);

    expect_result({"1", 'a', 0}, 0);
    expect_result({"1*", 'a', 0}, 0);
    expect_result({"1", 'a', 1}, INF);
    expect_result({"1*", 'a', 1}, INF);

    expect_error({"",       'a', 1});
    expect_error({"aa",     'a', 1});
    expect_error({"abcabc", 'a', 1});
    expect_error({"aa..",   'a', 1});
    expect_error({"a..",    'a', 1});
    expect_error({"aa++",   'a', 1});
    expect_error({"a++",    'a', 1});
    expect_error({"*",      'a', 1});
    expect_error({"d",      'a', 1});
    expect_error({"¹",      'a', 1});
    expect_error({"2",      'a', 1});

    fprintf(stderr, "PASSED\n");
    return 0;
}
