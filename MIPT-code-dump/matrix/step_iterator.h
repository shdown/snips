#pragma once
#include <stddef.h>
#include <unistd.h>
#include <iterator>

template<class T, unsigned Step>
class StepIterator : public std::iterator<std::random_access_iterator_tag, T>
{
    T *base_;
    size_t offset_;

public:
    typedef T value_type;
    typedef T& reference_type;
    typedef T* pointer_type;
    typedef ssize_t difference_type;

    StepIterator()
        : base_(nullptr)
        , offset_(0)
    {}

    explicit StepIterator(T *base)
        : base_(base)
        , offset_(0)
    {}

    StepIterator(const StepIterator &that) = default;
    StepIterator(StepIterator &&that) = default;
    StepIterator& operator =(const StepIterator &that) & = default;
    StepIterator& operator =(StepIterator &&that) & = default;

    StepIterator& operator ++() & { offset_ += Step; return *this; }
    StepIterator& operator --() & { offset_ -= Step; return *this; }

    StepIterator operator ++(int) & { auto saved = *this; ++*this; return saved; }
    StepIterator operator --(int) & { auto saved = *this; --*this; return saved; }

    bool operator ==(const StepIterator &that) const { return base_ == that.base_ && offset_ == that.offset_; }
    bool operator !=(const StepIterator &that) const { return !(*this == that); }

    bool operator <(const StepIterator &that) const  { return offset_ < that.offset_; }
    bool operator <=(const StepIterator &that) const { return offset_ <= that.offset_; }
    bool operator >(const StepIterator &that) const  { return offset_ > that.offset_; }
    bool operator >=(const StepIterator &that) const { return offset_ >= that.offset_; }

    reference_type operator *() const { return base_[offset_]; }
    pointer_type operator ->() const { return base_ + offset_; }
    reference_type operator [](difference_type off) const { return base_[offset_ + off * Step]; }

    StepIterator& operator +=(difference_type off) & { offset_ += off * Step; return *this; }
    StepIterator& operator -=(difference_type off) & { offset_ -= off * Step; return *this; }

    StepIterator operator +(difference_type off) const { auto tmp = *this; tmp += off; return tmp; }
    StepIterator operator -(difference_type off) const { auto tmp = *this; tmp -= off; return tmp; }

    difference_type operator -(const StepIterator &that) const { return (offset_ - that.offset_) / Step; }
};

template<class T, unsigned Step>
StepIterator<T, Step> operator +(typename StepIterator<T, Step>::difference_type off,
                                 StepIterator<T, Step> it)
{
    return it + off;
}
