#pragma once
#include <stddef.h> // size_t

// I needed these ones because they disallowed using STL.

template<typename T>
inline void doSwap(T &a, T &b) {
    T tmp = a;
    a = b;
    b = tmp;
}

template<typename T>
inline void doReverse(T *start, T *finish) {
    size_t n = finish - start;
    for (size_t i = 0; i < n / 2; ++i) {
        doSwap(start[i], start[n - i - 1]);
    }
}

// This one is just convenient for generating next/previous permutation.
template<typename T>
inline bool doCompare(const T &a, const T &b, bool less) {
    return less ? (a < b) : (a > b);
}
