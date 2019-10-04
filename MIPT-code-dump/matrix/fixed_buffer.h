#pragma once
#include <memory>
#include <algorithm>
#include <stddef.h>

template<typename T, unsigned Size>
class FixedBuffer
{
    T *data_;

public:
    FixedBuffer()
        : data_(new T[Size]())
    {}

    FixedBuffer(const FixedBuffer &that)
        : data_(new T[Size])
    {
        std::uninitialized_copy(that.begin(), that.end(), data_);
    }

    FixedBuffer(FixedBuffer &&that)
        : data_(that.data_)
    {
        that.data_ = nullptr;
    }

    FixedBuffer(const T &value)
        : data_(new T[Size])
    {
        std::uninitialized_fill(data_, data_ + Size, value);
    }

    FixedBuffer& operator =(const FixedBuffer &that) &
    {
        if (&that != this) {
            std::copy(that.begin(), that.end(), begin());
        }
        return *this;
    }

    FixedBuffer& operator =(FixedBuffer &&that) &
    {
        if (&that != this) {
            data_ = that.data_;
            that.data_ = nullptr;
        }
        return *this;
    }

          T* begin()       { return data_; }
    const T* begin() const { return data_; }

          T* end()       { return data_ + Size; }
    const T* end() const { return data_ + Size; }

          T& operator [](size_t index)       { return data_[index]; }
    const T& operator [](size_t index) const { return data_[index]; }

          T* data()       { return data_; }
    const T* data() const { return data_; }

    ~FixedBuffer() { delete[] data_; }
};
