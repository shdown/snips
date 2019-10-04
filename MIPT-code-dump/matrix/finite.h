#pragma once
#include <ostream>

template<int N>
class Finite
{
    static int fastPowModN_(int base, int exponent);

    static int multiplicativeInverse_(int value)
    {
        return fastPowModN_(value, N - 2);
    }

    static int normalize_(long long value)
    {
        const int signedMod = value % N;
        return signedMod < 0 ? signedMod + N : signedMod;
    }

    int value_;

public:
    Finite()
        : value_(0)
    {}

    Finite(int value)
        : value_(normalize_(value))
    {}

    Finite(long long value)
        : value_(normalize_(value))
    {}

    Finite(const Finite &that) = default;
    Finite(Finite &&that) = default;
    Finite& operator =(const Finite &that) & = default;
    Finite& operator =(Finite &&that) & = default;

    operator int() const { return value_; }

    explicit operator bool() const { return value_; }

    Finite operator -() const
    {
        return Finite(-value_);
    }

    Finite operator +(Finite that) const { return Finite(0LL + value_ + that.value_); }
    Finite operator -(Finite that) const { return Finite(0LL + value_ - that.value_); }

    Finite& operator +=(Finite that) & { value_ = normalize_(0LL + value_ + that.value_); return *this; }
    Finite& operator -=(Finite that) & { value_ = normalize_(0LL + value_ - that.value_); return *this; }

    Finite& operator ++() & { value_ = normalize_(value_ + 1LL); return *this; }
    Finite& operator --() & { value_ = normalize_(value_ - 1LL); return *this; }

    Finite operator ++(int) & { auto saved = *this; ++*this; return saved; }
    Finite operator --(int) & { auto saved = *this; --*this; return saved; }

    Finite operator *(Finite that) const
    {
        return Finite(1LL * value_ * that.value_);
    }

    Finite& operator *=(Finite that) &
    {
        value_ = normalize_(1LL * value_ * that.value_);
        return *this;
    }

    Finite operator /(Finite that) const
    {
        return Finite(1LL * value_ * multiplicativeInverse_(that.value_));
    }

    Finite& operator /=(Finite that) &
    {
        value_ = normalize_(1LL * value_ * multiplicativeInverse_(that.value_));
        return *this;
    }

    ~Finite() { static_assert(N >= 1, "bad modulo"); }
};

template<int N>
inline int Finite<N>::fastPowModN_(int base, int exponent)
{
    long long currentBase = base;
    long long answer = 1 % N;
    for (; exponent; exponent >>= 1) {
        if (exponent & 1) {
            answer *= currentBase;
            answer %= N;
        }
        currentBase *= currentBase;
        currentBase %= N;
    }
    return answer;
}

template<int N>
std::ostream& operator <<(std::ostream &os, const Finite<N> &x)
{
    return os << int(x);
}
