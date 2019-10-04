#pragma once
#include <algorithm> // iter_swap
#include "common.h"

namespace TimSort {

namespace details {

template<class RAI, class Compare>
void insertionSort(RAI first, RAI last, Compare comp, RAI firstUnsorted) {
    for (RAI i = firstUnsorted; i != last; ++i) {
        RAI j = i;
        while (j != first) {
            RAI prev = j;
            --prev;
            if (!comp(*j, *prev)) {
                break;
            }
            std::iter_swap(j, prev);
            j = prev;
        }
    }
}

template<class FWI, class Compare>
void selectionSort(FWI first, FWI last, Compare comp) {
    for (FWI i = first; i != last; ++i) {
        FWI min = i;
        FWI j = i;
        while (1) {
            ++j;
            if (j == last) {
                break;
            }
            if (comp(*j, *min)) {
                min = j;
            }
        }
        if (i != min) {
            std::iter_swap(i, min);
        }
    }
}

} // namespace details

} // namespace TimSort
