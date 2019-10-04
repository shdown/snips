#pragma once
#define RATIONAL_ENABLE_RAW_METHODS
#define RATIONAL_ENABLE_NONSENSE
#include <algorithm>
#include <exception>
#include <utility>
#include <ostream>
#include <vector>
#include <stddef.h>
#include <assert.h>
#include "fixed_table.h"
#include "../bigint/rational.h"

namespace details {

template<typename Field>
struct FieldTraits
{
    static void negate(Field &x) { x = -x; }

    static void fastAdd(Field &a, const Field &b) { a += b; }

    static void fastSubtract(Field &a, const Field &b) { a -= b; }

    static Field fastProduct(const Field &a, const Field &b) { return a * b; }

    static Field abs(const Field &x) { return x < 0 ? -x : x; }
};

template<>
struct FieldTraits<Rational>
{
    static void negate(Rational &x) { x.negate(); }

    static void fastAdd(Rational &a, const Rational &b) { a.rawAdd(b); }

    static void fastSubtract(Rational &a, const Rational &b) { a.rawSubtract(b); }

    static Rational fastProduct(const Rational &a, const Rational &b) { return a.rawProduct(b); }

    static Rational abs(const Rational &x) { return x.signum() < 0 ? -x : x; }
};

} // namespace details

struct MatrixUninvertableException : public std::exception
{
    virtual const char* what() const noexcept override { return "matrix is uninvertable"; }
};

template<unsigned M, unsigned N, typename Field=Rational>
class Matrix : public FixedTable<M, N, Field>
{
    std::pair<Matrix, std::vector<size_t>> LUDecomposition_() const;

    void swapRows_(size_t i, size_t j)
    {
        if (i != j) {
            std::swap_ranges(
                FixedTable<M, N, Field>::rowBegin(i),
                FixedTable<M, N, Field>::rowEnd(i),
                FixedTable<M, N, Field>::rowBegin(j));
        }
    }

    size_t gaussianEliminateWith_(Matrix &twin) &;

public:
    Matrix()
        : FixedTable<M, N, Field>()
    {
        static_assert(N == M, "default constructor only makes sense for square matrices");
        for (size_t i = 0; i < N; ++i) {
            (*this)[i][i] = Field(1);
        }
    }

    explicit Matrix(const Field &value)
        : FixedTable<M, N, Field>(value)
    {}

    template<typename T>
    Matrix(const std::vector<std::vector<T>> &data)
        : FixedTable<M, N, Field>()
    {
        assert(data.size() == M);
        for (size_t i = 0; i < M; ++i) {
            assert(data[i].size() == N);
            for (size_t j = 0; j < N; ++j) {
                (*this)[i][j] = data[i][j];
            }
        }
    }

    Matrix(const Matrix &that) = default;
    Matrix(Matrix &&that) = default;
    Matrix& operator =(const Matrix &that) & = default;
    Matrix& operator =(Matrix &&that) & = default;

    void clear()
    {
        for (size_t i = 0; i < M; ++i) {
            for (size_t j = 0; j < N; ++j) {
                (*this)[i][j] = Field();
            }
        }
    }

    Matrix& operator +=(const Matrix &that) &
    {
        for (size_t i = 0; i < M; ++i) {
            for (size_t j = 0; j < N; ++j) {
                (*this)[i][j] += that[i][j];
            }
        }
        return *this;
    }

    Matrix& operator -=(const Matrix &that) &
    {
        for (size_t i = 0; i < M; ++i) {
            for (size_t j = 0; j < N; ++j) {
                (*this)[i][j] -= that[i][j];
            }
        }
        return *this;
    }

    Matrix& operator *=(const Field &x) &
    {
        for (size_t i = 0; i < M; ++i) {
            for (size_t j = 0; j < N; ++j) {
                (*this)[i][j] *= x;
            }
        }
        return *this;
    }

    Matrix& operator /=(const Field &x) &
    {
        for (size_t i = 0; i < M; ++i) {
            for (size_t j = 0; j < N; ++j) {
                (*this)[i][j] /= x;
            }
        }
        return *this;
    }

    Matrix operator -() const
    {
        Matrix result = *this;
        for (size_t i = 0; i < M; ++i) {
            for (size_t j = 0; j < N; ++j) {
                details::FieldTraits<Field>::negate(result[i][j]);
            }
        }
        return result;
    }

    Field det() const;

    Matrix<N, M, Field> transposed() const;

    unsigned rank() const;

    void invert() &;

    Matrix inverted() const
    {
        Matrix result = *this;
        result.invert();
        return result;
    }

    Field trace() const
    {
        static_assert(N == M, "trace is only defined for square matrices");

        Field result(0);
        for (size_t i = 0; i < N; ++i) {
            result += (*this)[i][i];
        }
        return result;
    }
};

template<unsigned N, typename Field=Rational>
using SquareMatrix = Matrix<N, N, Field>;

template<unsigned N, typename Field>
bool isIdentityMatrix(const Matrix<N, N, Field> &mat)
{
    for (size_t i = 0; i < N; ++i) {
        for (size_t j = 0; j < N; ++j) {
            if (mat[i][j] != Field(int(i == j))) {
                return false;
            }
        }
    }
    return true;
}

namespace details {

constexpr unsigned ceiledHalf(unsigned n) { return n / 2 + n % 2; }

constexpr unsigned max2(unsigned a, unsigned b)
{
    return a > b ? a : b;
}

constexpr unsigned max3(unsigned a, unsigned b, unsigned c)
{
    return max2(max2(a, b), c);
}

template<unsigned M, unsigned N, typename Field>
void assignTransposed(Matrix<M, N, Field> &a, const Matrix<N, M, Field> &b)
{
    for (size_t i = 0; i < M; ++i) {
        for (size_t j = 0; j < N; ++j) {
            a[i][j] = b[j][i];
        }
    }
}

// Here be dragons.

template<unsigned M, unsigned N, typename Field>
using DisassembledMatrix = std::tuple<
    Matrix<ceiledHalf(M), ceiledHalf(N), Field>,
    Matrix<ceiledHalf(M), ceiledHalf(N), Field>,
    Matrix<ceiledHalf(M), ceiledHalf(N), Field>,
    Matrix<ceiledHalf(M), ceiledHalf(N), Field>
>;

template<unsigned M, unsigned N, typename Field>
const Field& accessDisassembled(const DisassembledMatrix<M, N, Field> &mat, size_t row, size_t column)
{
    if (row < ceiledHalf(M)) {
        if (column < ceiledHalf(N)) {
            return std::get<0>(mat)[row][column];
        } else {
            return std::get<1>(mat)[row][column - ceiledHalf(N)];
        }
    } else {
        if (column < ceiledHalf(N)) {
            return std::get<2>(mat)[row - ceiledHalf(M)][column];
        } else {
            return std::get<3>(mat)[row - ceiledHalf(M)][column - ceiledHalf(N)];
        }
    }
}

template<unsigned M, unsigned N, typename Field>
Field& accessDisassembled(DisassembledMatrix<M, N, Field> &mat, size_t row, size_t column)
{
    return const_cast<Field&>(accessDisassembled<M, N, Field>(
        const_cast<const DisassembledMatrix<M, N, Field>&>(mat), row, column));
}

template<unsigned M, unsigned N, typename Field>
DisassembledMatrix<M, N, Field> disassemble(const Matrix<M, N, Field> &mat)
{
    DisassembledMatrix<M, N, Field> result(
        Matrix<ceiledHalf(M), ceiledHalf(N), Field>(0),
        Matrix<ceiledHalf(M), ceiledHalf(N), Field>(0),
        Matrix<ceiledHalf(M), ceiledHalf(N), Field>(0),
        Matrix<ceiledHalf(M), ceiledHalf(N), Field>(0)
    );
    for (size_t i = 0; i < M; ++i) {
        for (size_t j = 0; j < N; ++j) {
            accessDisassembled<M, N, Field>(result, i, j) = mat[i][j];
        }
    }
    return result;
}

template<unsigned M, unsigned N, typename Field>
Matrix<M, N, Field> assemble(const DisassembledMatrix<M, N, Field> &mat)
{
    Matrix<M, N, Field> result(0);
    for (size_t i = 0; i < M; ++i) {
        for (size_t j = 0; j < N; ++j) {
            result[i][j] = accessDisassembled<M, N, Field>(mat, i, j);
        }
    }
    return result;
}

// O(N^2). In this case this is OK since this function is only used once in det(), which is
// O(N^2*log N) anyway.
bool isOddPermutation(const std::vector<size_t> &perm)
{
    const size_t n = perm.size();
    bool result = false;
    for (size_t i = 0; i < n; ++i) {
        for (size_t j = 0; j < i; ++j) {
            if (perm[j] > perm[i]) {
                result = !result;
            }
        }
    }
    return result;
}

template<unsigned K, unsigned M, unsigned N, typename Field>
Matrix<M, K, Field> naivelyMultiply(const Matrix<M, N, Field> &a,
                                    const Matrix<N, K, Field> &b)
{
    Matrix<M, K, Field> result(0);
    for (size_t i = 0; i < M; ++i) {
        for (size_t j = 0; j < K; ++j) {
            for (size_t k = 0; k < N; ++k) {
                details::FieldTraits<Field>::fastAdd(
                    result[i][j],
                    details::FieldTraits<Field>::fastProduct(a[i][k], b[k][j]));
            }
        }
    }
    return result;
}

} // namespace details

template<unsigned M, unsigned N, typename Field>
std::pair<Matrix<M, N, Field>, std::vector<size_t>>
    Matrix<M, N, Field>::LUDecomposition_() const
{
    static_assert(N == M, "LU Decomposition_() can only be applied to a square matrix");

    Matrix result = *this;

    std::vector<size_t> permutation(N);
    for (size_t i = 0; i < N; ++i) {
        permutation[i] = i;
    }

    for (size_t k = 0; k < N; ++k) {
        const size_t pivotRow =
            std::max_element(
                result.columnBegin(k) + k, result.columnEnd(k),
                [](const Field &a, const Field &b) {
                    return details::FieldTraits<Field>::abs(a) < details::FieldTraits<Field>::abs(b);
                })
            - result.columnBegin(k);

        if (!result[pivotRow][k]) {
            result.clear();
            break;
        }

        result.swapRows_(pivotRow, k);
        std::swap(permutation[pivotRow], permutation[k]);

        for (size_t i = k + 1; i < N; ++i) {
            result[i][k] /= result[k][k];
            for (size_t j = k + 1; j < N; ++j) {
                result[i][j] -= details::FieldTraits<Field>::fastProduct(result[i][k], result[k][j]);
            }
        }
    }

    return {result, permutation};
}

template<unsigned M, unsigned N, typename Field>
size_t Matrix<M, N, Field>::gaussianEliminateWith_(Matrix &twin) &
{
    size_t row = 0;
    for (size_t col = 0; col < N; ++col) {
        for (size_t i = row; i < M; ++i) {
            if (!(*this)[i][col]) {
                continue;
            }
            if (i != row) {
                for (size_t j = 0; j < N; ++j) {
                    std::swap((*this)[i][j], (*this)[row][j]);
                    std::swap(   twin[i][j],    twin[row][j]);

                    details::FieldTraits<Field>::negate((*this)[i][j]);
                    details::FieldTraits<Field>::negate(twin   [i][j]);
                }
            }
            break;
        }
        if (!(*this)[row][col]) {
            continue;
        }
        for (size_t i = row + 1; i < M; ++i) {
            Field coeff = (*this)[i][col] / (*this)[row][col];
            for (size_t j = 0; j < N; ++j) {
                (*this)[i][j] -= details::FieldTraits<Field>::fastProduct(coeff, (*this)[row][j]);
                twin   [i][j] -= details::FieldTraits<Field>::fastProduct(coeff,    twin[row][j]);
            }
        }
        ++row;
    }
    return std::min<size_t>(row, M);
}

template<unsigned M, unsigned N, typename Field>
void Matrix<M, N, Field>::invert() &
{
    static_assert(M == N, "only a square matrix can be inverted");

    Matrix tmp;
    if (gaussianEliminateWith_(tmp) != N) {
        throw MatrixUninvertableException();
    }
    for (size_t i = size_t(N) - 1; i != size_t(-1); --i) {
        Field divisor = (*this)[i][i];
        for (size_t j = 0; j < N; ++j) {
            (*this)[i][j] /= divisor;
            tmp[i][j] /= divisor;
        }
        for (size_t j = i + 1; j < N; ++j) {
            Field coeff = (*this)[i][j];
            if (!coeff) {
                continue;
            }
            for (size_t k = 0; k < N; ++k) {
                tmp[i][k] -= details::FieldTraits<Field>::fastProduct(coeff, tmp[j][k]);
            }
            (*this)[i][j] = Field();
        }
    }
    *this = tmp;
}

template<unsigned M, unsigned N, typename Field>
Field Matrix<M, N, Field>::det() const
{
    static_assert(N == M, "determinant is only defined for square matrices");

    const auto decomp = LUDecomposition_();
    const Matrix<M, N, Field> &lu = decomp.first;
    const std::vector<size_t> &luPerm = decomp.second;

    Field result(details::isOddPermutation(luPerm) ? -1 : 1);
    for (size_t i = 0; i < N; ++i) {
        result *= lu[i][i];
    }
    return result;
}

template<unsigned M, unsigned N, typename Field>
Matrix<N, M, Field> Matrix<M, N, Field>::transposed() const
{
    Matrix<N, M, Field> result(0);
    details::assignTransposed(result, *this);
    return result;
}

template<unsigned M, unsigned N, typename Field>
unsigned Matrix<M, N, Field>::rank() const
{
    Matrix tmp = *this;
    Matrix twin(0);
    return tmp.gaussianEliminateWith_(twin);
}

template<unsigned M, unsigned N, typename Field>
Matrix<M, N, Field> operator +(const Matrix<M, N, Field> &a,
                               const Matrix<M, N, Field> &b)
{
    auto result = a;
    result += b;
    return result;
}

template<unsigned M, unsigned N, typename Field>
Matrix<M, N, Field> operator -(const Matrix<M, N, Field> &a,
                               const Matrix<M, N, Field> &b)
{
    auto result = a;
    result -= b;
    return result;
}

template<unsigned M, unsigned N, unsigned K, typename Field>
Matrix<M, K, Field> operator *(const Matrix<M, N, Field> &a,
                               const Matrix<N, K, Field> &b)
{
    if (constexpr bool naively = details::max3(M, N, K) <= 20u) {
        return details::naivelyMultiply(a, b);
    }

    const auto aParts = details::disassemble(a);
    const auto bParts = details::disassemble(b);

    const auto &a11 = std::get<0>(aParts);
    const auto &a12 = std::get<1>(aParts);
    const auto &a21 = std::get<2>(aParts);
    const auto &a22 = std::get<3>(aParts);

    const auto &b11 = std::get<0>(bParts);
    const auto &b12 = std::get<1>(bParts);
    const auto &b21 = std::get<2>(bParts);
    const auto &b22 = std::get<3>(bParts);

    const auto m1 = (a11 + a22) * (b11 + b22);
    const auto m2 = (a21 + a22) * b11;
    const auto m3 = a11 * (b12 - b22);
    const auto m4 = a22 * (b21 - b11);
    const auto m5 = (a11 + a12) * b22;
    const auto m6 = (a21 - a11) * (b11 + b12);
    const auto m7 = (a12 - a22) * (b21 + b22);

    return details::assemble<M, K, Field>(details::DisassembledMatrix<M, K, Field>(
        m1 + m4 - m5 + m7,
        m3 + m5,
        m2 + m4,
        m1 - m2 + m3 + m6));
}

template<unsigned M, unsigned N, typename Field>
Matrix<M, N, Field> operator *(const Matrix<M, N, Field> &mat, const Field &x)
{
    auto result = mat;
    result *= x;
    return result;
}

template<unsigned M, unsigned N, typename Field>
Matrix<M, N, Field> operator *(const Field &x, const Matrix<M, N, Field> &mat)
{
    auto result = mat;
    result *= x;
    return result;
}

template<unsigned M, unsigned N, typename Field>
Matrix<M, N, Field> operator /(const Matrix<M, N, Field> &mat, const Field &x)
{
    auto result = mat;
    result /= x;
    return result;
}

template<unsigned M, unsigned N, typename Field>
Matrix<M, N, Field>& operator *=(Matrix<M, N, Field> &a, const Matrix<N, N, Field> &b)
{
    return a = a * b;
}

template<unsigned M, unsigned N, typename Field>
bool operator ==(const Matrix<M, N, Field> &a, const Matrix<M, N, Field> &b)
{
    for (size_t i = 0; i < M; ++i) {
        for (size_t j = 0; j < N; ++j) {
            if (a[i][j] != b[i][j]) {
                return false;
            }
        }
    }
    return true;
}

template<unsigned M, unsigned N, typename Field>
bool operator !=(const Matrix<M, N, Field> &a, const Matrix<M, N, Field> &b)
{
    return !(a == b);
}

template<unsigned M, unsigned N, typename Field>
std::ostream& operator <<(std::ostream &os, const Matrix<M, N, Field> &mat)
{
    for (size_t i = 0; i < M; ++i) {
        if (i) {
            os << std::endl;
        }
        for (size_t j = 0; j < N; ++j) {
            if (j) {
                os << ' ';
            }
            os << mat[i][j];
        }
    }
    return os;
}
