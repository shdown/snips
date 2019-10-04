#pragma once
#include <stddef.h> // size_t
#include <assert.h> // assert
#include <limits.h> // INT_MIN

#include "utils.h"
#include "buffer.h"
#include "mergesort.h"
#include "biginteger.h"
#include "biginteger_utils.h"

class Permutation {
    Buffer<int> perm_;

    void inplaceNextOrPrevious_(bool next);

    template<typename TNewElemHandler, typename TCycleEndHandler>
    void traverseCycles_(TNewElemHandler newElemHandler, TCycleEndHandler cycleEndHandler) const;

public:
    explicit Permutation(unsigned size)
        : perm_(size)
    {
        for (size_t i = 0; i < perm_.size(); ++i) {
            perm_[i] = i;
        }
    }

    Permutation(unsigned size, int *data)
        : perm_(size)
    {
        for (size_t i = 0; i < perm_.size(); ++i) {
            perm_[i] = data[i];
        }
    }

    Permutation(const Permutation &that) = default;
    Permutation(Permutation &&that) = default;
    Permutation& operator =(const Permutation &that) & = default;
    Permutation& operator =(Permutation &&that) & = default;

    Permutation(unsigned size, BigInteger lexNumber);

    BigInteger lexNumber() const;

    Permutation operator *(const Permutation &that) const {
        assert(perm_.size() == that.perm_.size());
        Permutation ans(perm_.size());
        for (size_t i = 0; i < perm_.size(); ++i) {
            ans.perm_[i] = (*this)[that[i]];
        }
        return ans;
    }

    Permutation& operator *=(const Permutation &that) & {
        *this = *this * that;
        return *this;
    }

    Permutation& operator ++() & {
        inplaceNextOrPrevious_(true);
        return *this;
    }

    Permutation& operator --() & {
        inplaceNextOrPrevious_(false);
        return *this;
    }

    Permutation operator ++(int) & {
        Permutation old(*this);
        ++*this;
        return old;
    }

    Permutation operator --(int) & {
        Permutation old(*this);
        --*this;
        return old;
    }

    Permutation next() const {
        Permutation ans(*this);
        ++ans;
        return ans;
    }

    Permutation previous() const {
        Permutation ans(*this);
        --ans;
        return ans;
    }

    const int& operator [](size_t i) const {
        return perm_[i];
    }

    const Permutation inverse() const {
        Permutation ans(perm_.size());
        for (size_t i = 0; i < perm_.size(); ++i) {
            ans.perm_[perm_[i]] = i;
        }
        return ans;
    }

    void operator ()(int *data) const {
        Buffer<int> buf(perm_.size());
        for (size_t i = 0; i < perm_.size(); ++i) {
            buf[perm_[i]] = data[i];
        }
        for (size_t i = 0; i < perm_.size(); ++i) {
            data[i] = buf[i];
        }
    }

    size_t derangementsCount() const {
        Buffer<int> buf(perm_.size() * 2);
        for (size_t i = 0; i < perm_.size(); ++i) {
            buf[i] = perm_[i];
        }
        return sortCountInversions(buf.data(), buf.data() + perm_.size(),
                buf.data() + perm_.size());
    }

    int compare(const Permutation &that) const {
        assert(perm_.size() == that.perm_.size());
        for (size_t i = 0; i < perm_.size(); ++i) {
            if (perm_[i] != that.perm_[i]) {
                return perm_[i] < that.perm_[i] ? -1 : 1;
            }
        }
        return 0;
    }

    bool operator <(const Permutation &that) const {
        return compare(that) < 0;
    }

    bool operator >(const Permutation &that) const {
        return compare(that) > 0;
    }

    bool operator ==(const Permutation &that) const {
        return compare(that) == 0;
    }

    bool operator !=(const Permutation &that) const {
        return compare(that) != 0;
    }

    bool operator <=(const Permutation &that) const {
        return compare(that) <= 0;
    }

    bool operator >=(const Permutation &that) const {
        return compare(that) >= 0;
    }

    bool isEven() const;

    bool isOdd() const {
        return !isEven();
    }

    Permutation pow(int degree) const;

    Permutation& operator +=(int count) & {
        return *this = Permutation(perm_.size(), lexNumber() + count);
    }

    Permutation& operator -=(int count) & {
        return *this += -count;
    }

    Permutation operator +(int count) const {
        Permutation ans(*this);
        ans += count;
        return ans;
    }

    Permutation operator -(int count) const {
        Permutation ans(*this);
        ans -= count;
        return ans;
    }
};

void Permutation::inplaceNextOrPrevious_(bool next) {
    if (!perm_.size()) {
        return;
    }
    // For what k and l are, see
    // https://en.wikipedia.org/wiki/Permutation#Generation_in_lexicographic_order
    size_t k = perm_.size();
    for (size_t i = perm_.size() - 1; i; --i) {
        if (doCompare(perm_[i - 1], perm_[i], next)) {
            k = i - 1;
            break;
        }
    }
    if (k == perm_.size()) {
        // no next/previous permutation
        doReverse(perm_.data(), perm_.data() + perm_.size());
    } else {
        size_t l;
        for (l = perm_.size() - 1; ; --l) {
            if (doCompare(perm_[k], perm_[l], next)) {
                break;
            }
        }
        doSwap(perm_[k], perm_[l]);
        doReverse(perm_.data() + k + 1, perm_.data() + perm_.size());
    }
}

Permutation::Permutation(unsigned size, BigInteger lexNumber)
    : perm_(size)
{
    assert(size);

    BigInteger fact(factorial(size - 1));
    Buffer<char> used(size, 0);
    for (size_t i = 0; i < size; ++i) {
        int indexInUnused = static_cast<int>(lexNumber / fact);
        lexNumber %= fact;
        for (size_t j = 0; j < size; ++j) {
            if (used[j]) {
                continue;
            }
            if (indexInUnused == 0) {
                perm_[i] = j;
                used[j] = 1;
                break;
            }
            --indexInUnused;
        }

        size_t quot = size - i - 1;
        if (quot) {
            fact /= quot;
        }
    }
}

BigInteger Permutation::lexNumber() const {
    assert(perm_.size());

    BigInteger fact(factorial(perm_.size() - 1));
    Buffer<char> seen(perm_.size(), 0);
    BigInteger ans;
    for (size_t i = 0; i < perm_.size(); ++i) {
        int elem = perm_[i];
        int indexInUnseen = 0;
        for (int j = 0; j < elem; ++j) {
            if (!seen[j]) {
                ++indexInUnseen;
            }
        }
        ans += fact * indexInUnseen;
        seen[elem] = 1;

        size_t quot = perm_.size() - i - 1;
        if (quot) {
            fact /= quot;
        }
    }
    return ans;
}

template<typename TNewElemHandler, typename TCycleEndHandler>
void Permutation::traverseCycles_(TNewElemHandler newElemHandler, TCycleEndHandler cycleEndHandler) const
{
    Buffer<char> visited(perm_.size(), 0);
    for (size_t i = 0; i < perm_.size(); ++i) {
        if (visited[i]) {
            continue;
        }
        size_t j = i;
        do {
            newElemHandler(j);
            visited[j] = 1;
            j = perm_[j];
        } while (j != i);
        cycleEndHandler();
    }
}

bool Permutation::isEven() const {
    bool even = true;
    bool curCycleEven = true;
    traverseCycles_(
        [&curCycleEven](size_t /*elem*/){
            curCycleEven = !curCycleEven;
        },
        [&even, &curCycleEven](){
            if (curCycleEven) {
                even = !even;
            }
            curCycleEven = true;
        }
    );
    return even;
}

const Permutation Permutation::pow(int degree) const {
    if (degree < 0) {
        assert(degree != INT_MIN); // -INT_MIN is undefined behaviour
        return pow(-degree).inverse();
    } else if (degree == 0) {
        return Permutation(perm_.size());
    }
    Permutation ans(perm_.size());
    Buffer<int> cycle(perm_.size());
    size_t cycleLen = 0;
    traverseCycles_(
        [&cycle, &cycleLen](size_t elem){
            cycle[cycleLen++] = elem;
        },
        [this, degree, &ans, &cycle, &cycleLen](){
            for (size_t i = 0; i < cycleLen; ++i) {
                ans.perm_[cycle[i]] = this->perm_[cycle[(i + degree - 1) % cycleLen]];
            }
            cycleLen = 0;
        }
    );
    return ans;
}
