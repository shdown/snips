#pragma once
#include <cstddef> // size_t
#include <algorithm> // swap_ranges
#include <cmath> // sqrt
#include <iterator> // iterator_traits
#include "lesser_sorts.h"
#include "buf_merge.h"
#include "common.h"

namespace TimSort {

namespace details {

template<class RAI>
struct InplaceMergeBlockReference {
    RAI first;
    typename std::iterator_traits<RAI>::difference_type size;

    friend void swap(InplaceMergeBlockReference &a, InplaceMergeBlockReference &b) {
        if (a.first != b.first) {
            std::swap_ranges(a.first, a.first + a.size, b.first);
        }
    }

    InplaceMergeBlockReference(RAI first_, typename std::iterator_traits<RAI>::difference_type size_)
        : first(first_)
        , size(size_)
    {}

    InplaceMergeBlockReference(const InplaceMergeBlockReference &that) = delete;
    InplaceMergeBlockReference& operator =(const InplaceMergeBlockReference &that) & = delete;
};

template<class RAI>
class InplaceMergeBlockIterator {
    InplaceMergeBlockReference<RAI> ref_;

public:
    InplaceMergeBlockIterator(RAI first, typename std::iterator_traits<RAI>::difference_type size)
        : ref_(first, size)
    {}

    InplaceMergeBlockIterator(const InplaceMergeBlockIterator &that)
        : ref_(that.ref_.first, that.ref_.size)
    {}

    InplaceMergeBlockIterator& operator =(const InplaceMergeBlockIterator &that) & {
        ref_.first = that.ref_.first;
        ref_.size = that.ref_.size;
        return *this;
    }

    InplaceMergeBlockReference<RAI>& operator *() {
        return ref_;
    }

    InplaceMergeBlockReference<RAI>* operator ->() {
        return &ref_;
    }

    InplaceMergeBlockIterator<RAI>& operator ++() & {
        ref_.first += ref_.size;
        return *this;
    }

    InplaceMergeBlockIterator<RAI> operator ++(int) & {
        InplaceMergeBlockIterator<RAI> saved = *this;
        ++*this;
        return saved;
    }

    bool operator ==(const InplaceMergeBlockIterator<RAI> &that) const {
        return ref_.first == that.ref_.first;
    }

    bool operator !=(const InplaceMergeBlockIterator<RAI> &that) const {
        return ref_.first != that.ref_.first;
    }
};

template<class RAI, class Compare>
void inplaceMerge(
        RAI first,
        RAI middle,
        RAI last,
        Compare comp,
        std::size_t gallop)
{
    TIMSORT_ASSERT(std::is_sorted(first, middle, comp));
    TIMSORT_ASSERT(std::is_sorted(middle, last, comp));

    //std::inplace_merge(first, middle, last, comp);
    const std::size_t n = last - first;
    if (n < 10) {
        selectionSort(first, last, comp);
        return;
    }
    const std::size_t blockSize = sqrt(n);
    const RAI boundaryBlockFirst  = middle - (middle - first) % blockSize;
    const RAI lastBlockFirst = last - blockSize - (last - first) % blockSize;
    if (boundaryBlockFirst < lastBlockFirst) {
        std::swap_ranges(boundaryBlockFirst, boundaryBlockFirst + blockSize, lastBlockFirst);
        selectionSort(
            InplaceMergeBlockIterator<RAI>(first,          blockSize),
            InplaceMergeBlockIterator<RAI>(lastBlockFirst, blockSize),
            [comp](const InplaceMergeBlockReference<RAI> &a, const InplaceMergeBlockReference<RAI> &b) {
                // As I’ve heard, this is not stable; for stable sorting,
                // a.back() and b.back() should be used for breaking ties if
                // *a.first == *b.first.
                return comp(*a.first, *b.first);
            }
        );
        for (RAI middle = first + blockSize; middle != lastBlockFirst; middle += blockSize) {
            rightBufferMerge(middle - blockSize, middle, middle + blockSize, lastBlockFirst, comp, gallop);
        }
    }
    const RAI sortBorder = last - 2 * (last - lastBlockFirst);
    selectionSort(sortBorder, last, comp);
    rightBufferMerge(first, sortBorder, lastBlockFirst, lastBlockFirst, comp, gallop);
    selectionSort(lastBlockFirst, last, comp);

    TIMSORT_ASSERT(std::is_sorted(first, last, comp));
}

} // namespace details

} // namespace TimSort
