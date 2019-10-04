#include "solve.h"
#include <string.h>
#include <iostream>

int
main()
{
    std::string rpn;
    char x;
    unsigned k;
    std::cin >> rpn >> x >> k;

    try {
        const int answer = solve(rpn, x, k);
        if (answer < 0) {
            std::cout << "INF" << std::endl;
        } else {
            std::cout << answer << std::endl;
        }
    } catch (InvalidExpression &) {
        std::cout << "ERROR" << std::endl;
    }

    return 0;
}
