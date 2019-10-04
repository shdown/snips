#pragma once
#include <iostream>
#include <vector>
#include <string>
#include <locale>
#include <limits.h> // for operator int() only

#include "utils.h"

// One possible optimization would be modifying subtraction so that it never allocates/reallocates
// memory.

class BigInteger {
    const static int BASE = 1e9;
    const static int BASE_LOG10 = 9;

    bool positive_;
    std::vector<int> data_;

    void normalize_() {
        while (data_.size() > 1 && data_.back() == 0) {
            data_.pop_back();
        }
    }

    int rawCompare_(const BigInteger &that) const;

    void rawAdd_(const BigInteger &that);

    void rawIncrement_();

    void rawSubtract_(const BigInteger &that);

    void rawDecrement_();

    BigInteger multiplication_(const BigInteger &that) const;

    BigInteger quotientOrRemainder_(int divisor, bool returnQuotient) const;

    BigInteger quotientOrRemainder_(const BigInteger &divisor, bool returnQuotient) const;

    void addOrSubtract_(const BigInteger &that, bool add) & {
        if (!*this) {
            *this = that;
            if (!add) {
                negate();
            }
        } else if (positive_ == (that.positive_ == add)) {
            rawAdd_(that);
        } else {
            if (rawCompare_(that) >= 0) {
                rawSubtract_(that);
            } else {
                BigInteger tmp(that);
                tmp.rawSubtract_(*this);
                tmp.positive_ = !positive_;
                swap(tmp);
            }
        }
    }

    friend std::istream& operator >>(std::istream &is, BigInteger &bi);

public:
    BigInteger()
        : positive_(true)
        , data_({0})
    {}

    BigInteger(int value)
        : positive_(value >= 0)
        , data_({value >= 0 ? value : -value})
    {
        static_assert(INT_MAX / BASE < BASE, "this code implies BASE*BASE > INT_MAX");
        if (data_[0] >= BASE) {
            data_.push_back(data_[0] / BASE);
            data_[0] %= BASE;
        }
    }

    void swap(BigInteger &that) {
        doSwap(positive_, that.positive_);
        data_.swap(that.data_);
    }

    BigInteger operator -() const {
        BigInteger ans(*this);
        ans.negate();
        return ans;
    }

    BigInteger& operator +=(const BigInteger &that) & {
        addOrSubtract_(that, true);
        return *this;
    }

    BigInteger& operator -=(const BigInteger &that) & {
        addOrSubtract_(that, false);
        return *this;
    }

    BigInteger& operator *=(const BigInteger &that) & {
        BigInteger tmp(multiplication_(that));
        swap(tmp);
        return *this;
    }

    BigInteger& operator /=(const BigInteger &that) & {
        BigInteger tmp(quotientOrRemainder_(that, true));
        swap(tmp);
        return *this;
    }

    BigInteger& operator %=(const BigInteger &that) & {
        BigInteger tmp(quotientOrRemainder_(that, false));
        swap(tmp);
        return *this;
    }

    int signum() const {
        if (!*this) {
            return 0;
        }
        return positive_ ? 1 : -1;
    }

    void negate() & {
        positive_ = !positive_;
    }

    int compare(const BigInteger &that) const {
        int this_signum = signum();
        int that_signum = that.signum();
        if (this_signum != that_signum) {
            return this_signum < that_signum ? -1 : 1;
        }
        return this_signum == 1 ? rawCompare_(that) : -rawCompare_(that);
    }

    BigInteger& operator ++() & {
        if (signum() == -1) {
            rawDecrement_();
        } else {
            positive_ = true;
            rawIncrement_();
        }
        return *this;
    }

    BigInteger& operator --() & {
        if (signum() == 1) {
            rawDecrement_();
        } else {
            positive_ = false;
            rawIncrement_();
        }
        return *this;
    }

    BigInteger operator ++(int) & {
        BigInteger old(*this);
        ++*this;
        return old;
    }

    BigInteger operator --(int) & {
        BigInteger old(*this);
        --*this;
        return old;
    }

    std::string toString() const;

    explicit operator bool() const {
        return data_.size() != 1 || data_[0];
    }

    explicit operator double() const;

    explicit operator int() const;
};

int BigInteger::rawCompare_(const BigInteger &that) const {
    if (data_.size() != that.data_.size()) {
        return data_.size() < that.data_.size() ? -1 : 1;
    }
    for (auto it1 = data_.rbegin(), it2 = that.data_.rbegin(); it1 != data_.rend(); ++it1, ++it2) {
        if (*it1 != *it2) {
            return *it1 < *it2 ? -1 : 1;
        }
    }
    return 0;
}

void BigInteger::rawAdd_(const BigInteger &that) {
    bool carry = false;
    for (size_t i = 0; i < data_.size() || i < that.data_.size() || carry; ++i) {
        if (i == data_.size()) {
            data_.push_back(0);
        }
        data_[i] += !!carry + (i < that.data_.size() ? that.data_[i] : 0);
        carry = data_[i] >= BASE;
        if (carry) {
            data_[i] -= BASE;
        }
    }
}

void BigInteger::rawIncrement_() {
    for (size_t i = 0; ++data_[i] == BASE; data_[i++] = 0) {
        if (i == data_.size() - 1) {
            data_.push_back(0);
        }
    }
}

void BigInteger::rawSubtract_(const BigInteger &that) {
    //assert(rawCompare_(that) >= 0);
    bool carry = false;
    for (size_t i = 0; i < that.data_.size() || carry; ++i) {
        data_[i] -= !!carry + (i < that.data_.size() ? that.data_[i] : 0);
        carry = data_[i] < 0;
        if (carry) {
            data_[i] += BASE;
        }
    }
    normalize_();
}

void BigInteger::rawDecrement_() {
    //assert(*this);
    for (size_t i = 0; --data_[i] < 0; data_[i++] += BASE) {}
    normalize_();
}

BigInteger BigInteger::multiplication_(const BigInteger &that) const {
    BigInteger ans;
    ans.data_.assign(data_.size() + that.data_.size(), 0);
    for (size_t i = 0; i < data_.size(); ++i) {
        int carry = 0;
        for (size_t j = 0; j < that.data_.size() || carry; ++j) {
            long long cur =
                ans.data_[i + j] +
                1LL * data_[i] * (j < that.data_.size() ? that.data_[j] : 0) +
                carry;
            ans.data_[i + j] = cur % BigInteger::BASE;
            carry = cur / BigInteger::BASE;
        }
    }
    ans.positive_ = positive_ == that.positive_;
    ans.normalize_();
    return ans;
}

std::string BigInteger::toString() const {
    std::string ans;
    switch (signum()) {
    case 0:
        ans.append("0");
        break;
    case -1:
        ans.append("-");
        break;
    }
    char buf[BigInteger::BASE_LOG10];
    for (auto it = data_.rbegin(); it != data_.rend(); ++it) {
        int start = BigInteger::BASE_LOG10;
        int j = BigInteger::BASE_LOG10 - 1;
        for (int tmp = *it; j >= 0; --j, tmp /= 10) {
            int digit = tmp % 10;
            if (digit) {
                start = j;
            }
            buf[j] = '0' + digit;
        }
        if (it == data_.rbegin()) {
            ans.append(buf + start, BigInteger::BASE_LOG10 - start);
        } else {
            ans.append(buf, BigInteger::BASE_LOG10);
        }
    }
    return ans;
}

std::istream& operator >>(std::istream &is, BigInteger &bi) {
    bi.positive_ = true;
    bi.data_.clear();
    bool readSmth = false;
    int curVal = 0;
    int digits = 0;
    std::istream::sentry s(is);
    if (s) {
        while (is.good()) {
            char c = is.get();
            if (std::isspace(c, is.getloc())) {
                break;
            } else if (!readSmth && c == '-') {
                readSmth = true;
                bi.positive_ = false;
            } else if (c >= '0' && c <= '9') {
                readSmth = true;
                curVal *= 10;
                curVal += c - '0';
                ++digits;
                if (digits == BigInteger::BASE_LOG10) {
                    bi.data_.push_back(curVal);
                    curVal = 0;
                    digits = 0;
                }
            } else {
                // ?
                break;
            }
        }
    }
    doReverse(bi.data_.data(), bi.data_.data() + bi.data_.size());
    if (bi.data_.empty()) {
        bi.data_.push_back(0);
    }
    bi.normalize_();
    int mul = 1;
    for (int i = 0; i < digits; ++i) {
        mul *= 10;
    }
    bi *= mul;
    bi.rawAdd_(curVal);
    return is;
}

std::ostream& operator <<(std::ostream &os, const BigInteger &bi) {
    std::string repr(bi.toString());
    os.write(repr.data(), repr.size());
    return os;
}

BigInteger operator +(const BigInteger &a, const BigInteger &b) {
    BigInteger ans(a);
    ans += b;
    return ans;
}

BigInteger operator -(const BigInteger &a, const BigInteger &b) {
    BigInteger ans(a);
    ans -= b;
    return ans;
}

BigInteger operator *(const BigInteger &a, const BigInteger &b) {
    BigInteger ans(a);
    ans *= b;
    return ans;
}

BigInteger operator /(const BigInteger &a, const BigInteger &b) {
    BigInteger ans(a);
    ans /= b;
    return ans;
}

BigInteger operator %(const BigInteger &a, const BigInteger &b) {
    BigInteger ans(a);
    ans %= b;
    return ans;
}

bool operator !=(const BigInteger &a, const BigInteger &b) {
    return a.compare(b) != 0;
}

bool operator ==(const BigInteger &a, const BigInteger &b) {
    return a.compare(b) == 0;
}

bool operator <=(const BigInteger &a, const BigInteger &b) {
    return a.compare(b) <= 0;
}

bool operator >=(const BigInteger &a, const BigInteger &b) {
    return a.compare(b) >= 0;
}

bool operator <(const BigInteger &a, const BigInteger &b) {
    return a.compare(b) < 0;
}

bool operator >(const BigInteger &a, const BigInteger &b) {
    return a.compare(b) > 0;
}

BigInteger BigInteger::quotientOrRemainder_(int divisor, bool returnQuotient) const {
    int absDivisor = divisor < 0 ? -divisor : divisor;
    BigInteger quotient = *this;
    int carry = 0;
    for (auto it = quotient.data_.rbegin(); it != quotient.data_.rend(); ++it) {
        long long cur = *it + 1LL * carry * BigInteger::BASE;
        *it = cur / absDivisor;
        carry = cur % absDivisor;
    }
    quotient.normalize_();
    quotient.positive_ = positive_ == (divisor > 0);
    if (!positive_) {
        carry = -carry;
    }
    return returnQuotient ? quotient : BigInteger(carry);
}

BigInteger BigInteger::quotientOrRemainder_(const BigInteger &divisor, bool returnQuotient) const {
    if (divisor.data_.size() == 1) {
        return quotientOrRemainder_(divisor.positive_ ? divisor.data_[0] : -divisor.data_[0], returnQuotient);
    }

    BigInteger absDivisor(divisor);
    if (absDivisor.signum() < 0) {
        absDivisor.negate();
    }

    BigInteger remainder;
    BigInteger quotient;
    quotient.data_.resize(data_.size());

    auto it1 = data_.rbegin();
    auto it2 = quotient.data_.rbegin();
    for (; it1 != data_.rend(); ++it1, ++it2) {
        remainder.data_.insert(remainder.data_.begin(), *it1);
        remainder.normalize_();
        int result;
        // binary search
        int lbound = 0, rbound = BASE;
        while (rbound - lbound > 1) {
            int middle = (lbound + rbound) / 2;
            if (absDivisor * middle <= remainder) {
                lbound = middle;
            } else {
                rbound = middle;
            }
        }
        result = lbound;

        remainder -= absDivisor * result;
        *it2 = result;
    }
    quotient.positive_ = positive_ == divisor.positive_;
    quotient.normalize_();
    remainder.positive_ = positive_;
    return returnQuotient ? quotient : remainder;
}

BigInteger::operator double() const {
    double ans = 0;
    for (auto it = data_.rbegin(); it != data_.rend(); ++it) {
        ans *= BigInteger::BASE;
        ans += *it;
    }
    return positive_ ? ans : -ans;
}

BigInteger::operator int() const {
    int ans = 0;
    for (auto it = data_.rbegin(); it != data_.rend(); ++it) {
        ans *= BigInteger::BASE;
        ans += *it;
    }
    return positive_ ? ans : -ans;
}
