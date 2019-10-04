#pragma once
#include <cstddef> // size_t
#include <algorithm> // swap_ranges, iter_swap
#include "common.h"

namespace TimSort {

namespace details {

template<class RAI, class RAI2>
inline RAI2 swapRangesBackwards(RAI first, RAI last, RAI2 last2) {
    while (last != first) {
        std::iter_swap(--last, --last2);
    }
    return last2;
}

template<class RAI, class BufRAI, class Compare>
void rightBufferMerge(
        RAI first,
        RAI middle,
        RAI last,
        BufRAI bufFirst,
        Compare comp,
        std::size_t gallop)
{
    TIMSORT_ASSERT(std::is_sorted(first, middle, comp));
    TIMSORT_ASSERT(std::is_sorted(middle, last, comp));

    auto       afterLeft       = middle;
    const auto afterLeftLimit  = first;
    auto       afterRight      = std::swap_ranges(middle, last, bufFirst);
    const auto afterRightLimit = bufFirst;
    auto       afterBuf        = last;

    std::size_t currentStreak = 0;
    bool currentStreakFromLeft = false;

    while (1) {
        if (afterLeft == afterLeftLimit) {
            std::swap_ranges(afterRightLimit, afterRight, first);
            break;
        }
        if (afterRight == afterRightLimit) {
            break;
        }
        const bool fromLeft = comp(*(afterRight - 1), *(afterLeft - 1));
        if (currentStreakFromLeft != fromLeft) {
            currentStreak = 0;
            currentStreakFromLeft = fromLeft;
        }
        if (++currentStreak == gallop) {
            if (currentStreakFromLeft) {
                const auto it = std::lower_bound(afterLeftLimit, afterLeft, *(afterRight - 1), comp);
                afterBuf = swapRangesBackwards(it, afterLeft, afterBuf);
                afterLeft = it;
            } else {
                const auto it = std::upper_bound(afterRightLimit, afterRight, *(afterLeft - 1), comp);
                afterBuf = swapRangesBackwards(it, afterRight, afterBuf);
                afterRight = it;
            }
        } else {
            if (fromLeft) {
                std::iter_swap(--afterLeft, --afterBuf);
            } else {
                std::iter_swap(--afterRight, --afterBuf);
            }
        }
    }

    TIMSORT_ASSERT(std::is_sorted(first, last, comp));
}

} // namespace details

} // namespace TimSort
