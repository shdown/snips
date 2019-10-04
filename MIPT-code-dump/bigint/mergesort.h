#pragma once
#include <stddef.h> // size_t

template<typename T>
size_t sortCountInversions(T *start, T *finish, T *buf) {
    const size_t n = finish - start;
    if (n <= 1) {
        return 0;
    }

    const size_t half = n / 2;
    T *p = start;
    T *pLim = p + half;
    T *q = p + half;
    T *qLim = finish;

    size_t ans = sortCountInversions(p, pLim, buf) + sortCountInversions(q, qLim, buf);

    T *bufptr = buf;
    while (p < pLim || q < qLim) {
        if (q == qLim || (p != pLim && *p <= *q)) {
            *bufptr++ = *p++;
        } else {
            ans += pLim - p;
            *bufptr++ = *q++;
        }
    }
    for (size_t i = 0; i < n; ++i) {
        start[i] = buf[i];
    }
    return ans;
}

