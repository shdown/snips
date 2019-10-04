#pragma once
#include <cstddef> // size_t
#include <algorithm> // reverse, min
#include "lesser_sorts.h"
#include "common.h"

namespace TimSort {

namespace details {

template<class RAI>
struct SortedPrefix {
    RAI last;
    bool descending;
};

template<class RAI, class Compare>
inline SortedPrefix<RAI> getSortedPrefix(RAI first, RAI last, Compare comp) {
    RAI i = first;
    if (i == last) {
        return {last, false}; // 0 elements
    }
    ++i;
    if (i == last) {
        return {last, false}; // 1 element
    }
    const bool descending = comp(*i, *first);
    while (1) {
        RAI next = i;
        ++next;
        if (next == last || comp(*next, *i) != descending) {
            return {next, descending};
        }
        i = next;
    }
    // unreached
}

template<class RAI, class Compare>
inline RAI sortGetRunLast(RAI first, RAI last, Compare comp, std::size_t minRun) {
    const SortedPrefix<RAI> prefix = getSortedPrefix(first, last, comp);
    if (prefix.descending) {
        std::reverse(first, prefix.last);
    }
    const RAI atLeast = first + std::min<std::size_t>(minRun, last - first);
    if (prefix.last < atLeast) {
        insertionSort(first, atLeast, comp, prefix.last);
        return atLeast;
    }
    return prefix.last;
}

} // namespace details

} // namespace TimSort
