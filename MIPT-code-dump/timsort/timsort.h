#pragma once
#include <cstddef> // size_t
#include <vector> // vector
#include <utility> // pair
#include <functional> // less
#include <iterator> // iterator_traits
#include <cstdint> // uint32_t
#include "runs.h"
#include "inplace_merge.h"
#include "common.h"

namespace TimSort {

enum EWhatToMerge {
    WTM_Nothing,
    WTM_MergeXY,
    WTM_MergeYZ,
};

struct ITimSortParams {
    virtual std::uint32_t minRun(std::uint32_t n) const = 0;
    virtual bool needMerge(std::uint32_t x, std::uint32_t y) const = 0;
    virtual EWhatToMerge whatToMerge(std::uint32_t x, std::uint32_t y, std::uint32_t z) const = 0;
    virtual EWhatToMerge whatToMerge(std::uint32_t w, std::uint32_t x, std::uint32_t y, std::uint32_t z) const = 0;
    virtual std::uint32_t gallop() const = 0;
    virtual ~ITimSortParams() {}
};

struct DefaultTimSortParams : public ITimSortParams {

    virtual std::uint32_t minRun(std::uint32_t n) const override {
        std::uint32_t r = 0;
        while (n >= 64) {
            r |= (n & 1);
            n >>= 1;
        }
        return n + r;
    }

    virtual bool needMerge(std::uint32_t y, std::uint32_t z) const override {
        return y <= z;
    }

    virtual EWhatToMerge whatToMerge(std::uint32_t x, std::uint32_t y, std::uint32_t z) const override {
        return whatToMerge(0, x, y, z);
    }

    virtual EWhatToMerge whatToMerge(std::uint32_t w, std::uint32_t x, std::uint32_t y, std::uint32_t z) const override {
        if (x <= y + z || (w && w <= x + y)) {
            return x < z ? WTM_MergeXY : WTM_MergeYZ;
        }
        return y <= z ? WTM_MergeYZ : WTM_Nothing;
    }

    virtual std::uint32_t gallop() const override {
        return 7;
    }
};

namespace details {

template<class RAI, class Compare>
inline void mergeXY(
        std::vector<std::pair<RAI, RAI>> &stack,
        Compare comp,
        std::size_t gallop)
{
    const std::size_t n = stack.size();
    TIMSORT_ASSERT(stack[n - 3].second == stack[n - 2].first);
    inplaceMerge(stack[n - 3].first, stack[n - 3].second, stack[n - 2].second, comp, gallop);
    stack[n - 3].second = stack[n - 2].second;
    stack[n - 2] = stack[n - 1];
    stack.pop_back();
}

template<class RAI, class Compare>
inline void mergeYZ(
        std::vector<std::pair<RAI, RAI>> &stack,
        Compare comp,
        std::size_t gallop)
{
    const std::size_t n = stack.size();
    TIMSORT_ASSERT(stack[n - 2].second == stack[n - 1].first);
    inplaceMerge(stack[n - 2].first, stack[n - 2].second, stack[n - 1].second, comp, gallop);
    stack[n - 2].second = stack[n - 1].second;
    stack.pop_back();
}

template<class RAI, class Compare>
inline void mergeCollapse(
        std::vector<std::pair<RAI, RAI>> &stack,
        Compare comp,
        std::size_t gallop)
{
    while (1) {
        const std::size_t n = stack.size();
        if (n < 2) {
            break;
        } else if (n == 2) {
            details::mergeYZ(stack, comp, gallop);
            break;
        }
        const std::size_t z = stack[n - 1].second - stack[n - 1].first;
        const std::size_t x = stack[n - 3].second - stack[n - 3].first;
        if (x < z) {
            details::mergeXY(stack, comp, gallop);
        } else {
            details::mergeYZ(stack, comp, gallop);
        }
    }
}


template<class RAI, class Compare>
inline void mergeIfNeeeded(
    std::vector<std::pair<RAI, RAI>> &stack,
    Compare comp,
    std::size_t gallop,
    const ITimSortParams &params)
{
    while (1) {
        const std::size_t n = stack.size();
        if (n < 2) {
            break;
        }

        const std::size_t z = stack[n - 1].second - stack[n - 1].first;
        const std::size_t y = stack[n - 2].second - stack[n - 2].first;
        if (n == 2) {
            if (params.needMerge(y, z)) {
                details::mergeYZ(stack, comp, gallop);
            }
            break;
        }

        const std::size_t x = stack[n - 3].second - stack[n - 3].first;
        EWhatToMerge wtm;
        if (n == 3) {
            wtm = params.whatToMerge(x, y, z);
        } else {
            const std::size_t w = stack[n - 4].second - stack[n - 4].first;
            wtm = params.whatToMerge(w, x, y, z);
        }
        if (wtm == WTM_Nothing) {
            break;
        } else if (wtm == WTM_MergeXY) {
            details::mergeXY(stack, comp, gallop);
        } else if (wtm == WTM_MergeYZ) {
            details::mergeYZ(stack, comp, gallop);
        }
    }
}

} // namespace details

template<class RAI, class Compare>
void timSort(
        RAI first,
        RAI last,
        Compare comp,
        const ITimSortParams &params = DefaultTimSortParams())
{
    const std::size_t minRun = params.minRun(last - first);
    const std::size_t gallop = params.gallop();

    std::vector<std::pair<RAI, RAI>> stack;

    for (RAI runFirst = first; runFirst != last;) {
        // push new run onto the stack
        const RAI runLast = details::sortGetRunLast(runFirst, last, comp, minRun);
        TIMSORT_ASSERT(std::is_sorted(runFirst, runLast, comp));
        stack.emplace_back(runFirst, runLast);

        // advance runFirst
        runFirst = runLast;

        // merge
        details::mergeIfNeeeded(stack, comp, gallop, params);
    }

    details::mergeCollapse(stack, comp, gallop);
    TIMSORT_ASSERT(std::is_sorted(first, last, comp));
}

template<class RAI>
inline void timSort(RAI first, RAI last, const ITimSortParams &params = DefaultTimSortParams()) {
    timSort(first, last, std::less<typename std::iterator_traits<RAI>::value_type>(), params);
}

} // namespace TimSort
