#pragma once
#include "biginteger.h"

const BigInteger factorial(unsigned n) {
    BigInteger ans = 1;
    for (unsigned i = 2; i <= n; ++i) {
        ans *= i;
    }
    return ans;
}

const BigInteger gcd(BigInteger a, BigInteger b) {
    if (a.signum() < 0) {
        a.negate();
    }
    if (b.signum() < 0) {
        b.negate();
    }
    if (!a) {
        return b;
    }
    if (!b) {
        return a;
    }
    while (a %= b) {
        a.swap(b);
    }
    return b;
}

