#ifndef solve_h_
#define solve_h_

#include <string>
#include <exception>

struct InvalidExpression : public std::exception {
    virtual const char *
    what() const noexcept override
    {
        return "invalid expression";
    }
};

int
solve(const std::string &rpn, char x, unsigned k);

#endif
