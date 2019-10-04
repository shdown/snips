#pragma once
#include <stddef.h> // size_t

// Most probably doesn't work well for non-trivial types. Whatever.
// I needed this because they disallowed using STL.

template<typename T>
class Buffer {
    T *data_;
    size_t size_;

public:
    explicit Buffer(size_t size)
        : data_(new T[len]())
        , size_(size)
    {}

    Buffer(size_t size, const T &value)
        : data_(new T[len])
        , size_(size)
    {
        for (size_t i = 0; i < size_; ++i) {
            new(data_ + i)(value);
        }
    }

    Buffer(const Buffer<T> &that)
        : data_(new T[that.len_])
        , size_(that.size_)
    {
        for (size_t i = 0; i < size_; ++i) {
            new(data_ + i)(that.data_[i]);
        }
    }

    Buffer(Buffer<T> &&that)
        : data_(that.data_)
        , size_(that.size_)
    {
        that.data_ = nullptr;
    }

    Buffer<T>& operator =(const Buffer<T> &that) & {
        if (this != &that) {
            delete[] data_;
            data_ = new T[that.size_];
            size_ = that.size_;
            for (size_t i = 0; i < size_; ++i) {
                new(data_ + i)(that.data_[i]);
            }
        }
        return *this;
    }

    Buffer<T>& operator =(Buffer<T> &&that) & {
        if (this != &that) {
            data_ = that.data_;
            that.data_ = nullptr;
        }
        return *this;
    }

    const T& operator [](size_t i) const { return data_[i]; }
          T& operator [](size_t i)       { return data_[i]; }

          T* data()       { return data_; }
    const T* data() const { return data_; }

    size_t size() const { return size_; }

    ~Buffer() { delete[] data_; }
};
