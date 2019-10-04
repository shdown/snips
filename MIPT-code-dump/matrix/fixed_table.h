#pragma once
#include <vector>
#include <stddef.h>
#include "step_iterator.h"
#include "fixed_buffer.h"

template<unsigned M, unsigned N, typename T>
class FixedTable
{
public:
    static const size_t size = M * N;

    FixedTable()
        : buf_()
    {}

    FixedTable(const T &value)
        : buf_(value)
    {}

    FixedTable(const FixedTable &that) = default;
    FixedTable(FixedTable &&that) = default;
    FixedTable& operator =(const FixedTable &that) & = default;
    FixedTable& operator =(FixedTable &&that) & = default;

          T* rowBegin(size_t index)       { return buf_.data() + N * index; }
    const T* rowBegin(size_t index) const { return buf_.data() + N * index; }

          T* rowEnd(size_t index)       { return buf_.data() + N * (index + 1); }

    const T* rowEnd(size_t index) const { return buf_.data() + N * (index + 1); }

    StepIterator<T,       N> columnBegin(size_t index)       { return StepIterator<T,       N>(buf_.data() + index); }
    StepIterator<const T, N> columnBegin(size_t index) const { return StepIterator<const T, N>(buf_.data() + index); }

    StepIterator<T,       N> columnEnd(size_t index)       { return columnBegin(index) + M; }
    StepIterator<const T, N> columnEnd(size_t index) const { return columnBegin(index) + M; }

    std::vector<T> getRow(size_t index)    const { return std::vector<T>(rowBegin(index),    rowEnd(index)); }
    std::vector<T> getColumn(size_t index) const { return std::vector<T>(columnBegin(index), columnEnd(index)); }

          T* operator [](size_t index)       { return rowBegin(index); }
    const T* operator [](size_t index) const { return rowBegin(index); }

private:
    FixedBuffer<T, size> buf_;
};
