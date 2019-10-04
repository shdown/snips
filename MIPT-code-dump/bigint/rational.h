#pragma once
#include "biginteger.h"
#include "biginteger_utils.h"
#include <ostream>

class Rational {
    BigInteger num_, denom_;

    void normalize_() {
        if (num_) {
            if (denom_.signum() < 0) {
                num_.negate();
                denom_.negate();
            }
        } else {
            denom_ = 1;
        }
    }

    void rawAddOrSubtract_(const Rational &that, bool add) & {
        BigInteger divisor = gcd(denom_, that.denom_);
        denom_ /= divisor;
        num_ *= that.denom_ / divisor;
        BigInteger delta = denom_ * that.num_;
        if (add) {
            num_ += delta;
        } else {
            num_ -= delta;
        }
        denom_ *= that.denom_;
        normalize_();
    }

public:
    void reduce() {
        BigInteger divisor = gcd(num_, denom_);
        num_ /= divisor;
        denom_ /= divisor;
    }

#ifdef RATIONAL_ENABLE_RAW_METHODS
    void rawAdd(const Rational &that) & { rawAddOrSubtract_(that, true); }

    void rawSubtract(const Rational &that) & { rawAddOrSubtract_(that, false); }

    Rational rawProduct(const Rational &that) const { return Rational(num_ * that.num_, denom_ * that.denom_); }
#endif

    void negate() { num_.negate(); }

    void swap(Rational &that) {
        num_.swap(that.num_);
        denom_.swap(that.denom_);
    }

    Rational()
        : num_(0)
        , denom_(1)
    {}

    Rational(BigInteger value)
        : num_(value)
        , denom_(1)
    {}

    Rational(int value)
        : num_(value)
        , denom_(1)
    {}

    Rational(BigInteger numerator, BigInteger denominator)
        : num_(numerator)
        , denom_(denominator)
    {
        normalize_();
    }

    explicit operator bool() const {
        return bool(num_);
    }

    Rational(const Rational &that) = default;
    Rational(Rational &&that) = default;
    Rational& operator =(const Rational &that) & = default;
    Rational& operator =(Rational &&that) & = default;

    Rational& operator +=(const Rational &that) & {
        rawAddOrSubtract_(that, true);
        reduce();
        return *this;
    }

    Rational& operator -=(const Rational &that) & {
        rawAddOrSubtract_(that, false);
        reduce();
        return *this;
    }

    Rational& operator *=(const Rational &that) & {
        num_ *= that.num_;
        denom_ *= that.denom_;
        normalize_();
        reduce();
        return *this;
    }

    Rational& operator /=(const Rational &that) & {
        num_ *= that.denom_;
        denom_ *= that.num_;
        normalize_();
        reduce();
        return *this;
    }

    Rational operator -() const {
        Rational ans(*this);
        ans.negate();
        return ans;
    }

    int signum() const {
        return num_.signum();
    }

    int compare(const Rational &that) const {
        int this_signum = num_.signum();
        int that_signum = that.num_.signum();
        if (this_signum != that_signum) {
            return this_signum < that_signum ? -1 : 1;
        }
        int cmp = (num_ * that.denom_).compare(that.num_ * denom_);
        return this_signum > 0 ? cmp : -cmp;
    }

    std::string toString() const {
        std::string ans(num_.toString());
        if (denom_ != 1) {
            ans.append("/").append(denom_.toString());
        }
        return ans;
    }

    std::string asDecimal(size_t precision=0) const;

    explicit operator double() const {
        return double(num_) / double(denom_);
    }

#ifdef RATIONAL_ENABLE_NONSENSE
    double toDouble() const { return double(*this); }
#endif
};

Rational operator +(const Rational &a, const Rational &b) {
    Rational ans(a);
    ans += b;
    return ans;
}

Rational operator -(const Rational &a, const Rational &b) {
    Rational ans(a);
    ans -= b;
    return ans;
}

Rational operator *(const Rational &a, const Rational &b) {
    Rational ans(a);
    ans *= b;
    return ans;
}

Rational operator /(const Rational &a, const Rational &b) {
    Rational ans(a);
    ans /= b;
    return ans;
}

bool operator ==(const Rational &a, const Rational &b) {
    return a.compare(b) == 0;
}

bool operator !=(const Rational &a, const Rational &b) {
    return a.compare(b) != 0;
}

bool operator <=(const Rational &a, const Rational &b) {
    return a.compare(b) <= 0;
}

bool operator >=(const Rational &a, const Rational &b) {
    return a.compare(b) >= 0;
}

bool operator <(const Rational &a, const Rational &b) {
    return a.compare(b) < 0;
}

bool operator >(const Rational &a, const Rational &b) {
    return a.compare(b) > 0;
}

std::string Rational::asDecimal(size_t precision) const {
    BigInteger quotient = num_ / denom_;
    BigInteger remainder = num_ % denom_;
    std::string ans(num_.signum() < 0 ? "-" : "");
    if (quotient.signum() < 0) {
        quotient.negate();
    }
    if (remainder.signum() < 0) {
        remainder.negate();
    }
    ans.append(quotient.toString());

    if (!precision) {
        return ans;
    }

    BigInteger ten(10);
    for (size_t i = 0; i < precision; ++i) {
        remainder *= ten;
    }
    remainder /= denom_;
    std::string rem_repr = remainder.toString();
    rem_repr.insert(0, precision - rem_repr.size(), '0');

    return ans.append(".").append(rem_repr);
}

std::ostream& operator <<(std::ostream &os, const Rational &r) {
    std::string repr(r.toString());
    os.write(repr.data(), repr.size());
    return os;
}

#ifdef RATIONAL_ENABLE_NONSENSE
std::istream& operator >>(std::istream &is, Rational &r) {
    BigInteger b;
    is >> b;
    r = b;
    return is;
}
#endif
