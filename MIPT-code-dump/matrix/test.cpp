#include "matrix.h"
#include "finite.h"
#include <iostream>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <assert.h>

template<unsigned K, unsigned M, unsigned N, typename Field>
Matrix<M, K, Field> expectedProduct(const Matrix<M, N, Field> &a,
                                            const Matrix<N, K, Field> &b)
{
    Matrix<M, K, Field> result(0);
    for (size_t i = 0; i < M; ++i) {
        for (size_t j = 0; j < K; ++j) {
            for (size_t k = 0; k < N; ++k) {
                result[i][j] += a[i][k] * b[k][j];
            }
        }
    }
    return result;
}

template<typename Field, unsigned M, unsigned N>
void fillRandom(Matrix<M, N, Field> &m)
{
    for (size_t i = 0; i < M; ++i) {
        for (size_t j = 0; j < N; ++j) {
            m[i][j] = Field(rand());
        }
    }
}

template<unsigned N, typename Field>
const Field
expectedDet(const Matrix<N, N, Field> &m)
{
    return m.det();
}

template<typename Field>
const Field
expectedDet(const Matrix<1, 1, Field> &m)
{
    return m[0][0];
}

template<typename Field>
const Field
expectedDet(const Matrix<2, 2, Field> &m)
{
    return m[0][0] * m[1][1] - m[0][1] * m[1][0];
}

template<typename Field>
const Field
expectedDet(const Matrix<3, 3, Field> &m)
{
    return m[0][0] * (m[1][1] * m[2][2] - m[1][2] * m[2][1]) -
           m[0][1] * (m[1][0] * m[2][2] - m[2][0] * m[1][2]) +
           m[0][2] * (m[1][0] * m[2][1] - m[1][1] * m[2][0]);
}

template<typename Field, unsigned N>
void testDetRandom(int tests)
{
    Matrix<N, N, Field> m(0);
    for (int t = 0; t < tests; ++t) {
        std::cerr << "testDetRandom() iter " << t << std::endl;
        fillRandom(m);
        const Field expected = expectedDet(m);
        const Field found = m.det();
        if (expected != found) {
            std::cerr << "testDetRandom: FAIL: expected " << expected << ", found " << found << std::endl;
            std::cerr << "matrix:" << std::endl << m << std::endl;
            exit(EXIT_FAILURE);
        }
    }
}

template<typename Field, unsigned N>
void testInvertRandom(int tests)
{
    Matrix<N, N, Field> m(0);
    for (int t = 0; t < tests; ++t) {
        std::cerr << "testInvertRandom() iter " << t << std::endl;
        fillRandom(m);
        try {
            const Matrix<N, N, Field> inv = m.inverted();
            const Matrix<N, N, Field> e = expectedProduct(m, inv);
            if (!isIdentityMatrix(e)) {
                std::cerr << "testInvertRandom: FAIL: m * m.inverted() != E" << std::endl;
                std::cerr << "m:" << std::endl << m << std::endl;
                std::cerr << "m.inverted():" << std::endl << inv << std::endl;
                std::cerr << "m * m.inverted():" << std::endl << e << std::endl;
                exit(EXIT_FAILURE);
            }
        } catch (MatrixUninvertableException) {
            const Field det = expectedDet(m);
            if (det) {
                std::cerr << "testInvertRandom: FAIL: inverted() on a non-degenerate matrix thrown MatrixUninvertableException" << std::endl;
                std::cerr << "matrix:" << std::endl << m << std::endl;
                exit(EXIT_FAILURE);
            }
        }
    }
}

template<typename Field, unsigned M, unsigned N, unsigned K>
void testMulRandom(int tests)
{
    Matrix<M, N, Field> a(0);
    Matrix<N, K, Field> b(0);
    Matrix<M, K, Field> expected(0);
    Matrix<M, K, Field> found(0);
    for (int t = 0; t < tests; ++t) {
        std::cerr << "testMulRandom() iter " << t << std::endl;
        std::cerr << "fillRandom(a)" << std::endl;
        fillRandom(a);
        std::cerr << "fillRandom(b)" << std::endl;
        fillRandom(b);
        std::cerr << "expectedProduct()" << std::endl;
        expected = expectedProduct(a, b);
        std::cerr << "a * b" << std::endl;
        found = a * b;
        std::cerr << "!=" << std::endl;
        if (expected != found) {
            std::cerr << "testMulRandom: FAIL" << std::endl;
            std::cerr << "a:" << std::endl << a << std::endl;
            std::cerr << "b:" << std::endl << b << std::endl;
            std::cerr << "expected:" << std::endl << expected << std::endl;
            std::cerr << "found:" << std::endl << found << std::endl;
            exit(EXIT_FAILURE);
        }
        std::cerr << "OK" << std::endl;
    }
}

template<typename Field, unsigned M, unsigned N>
void assertRankIs(const Matrix<M, N, Field> &m, unsigned expectedRank)
{
    unsigned foundRank = m.rank();
    if (expectedRank != foundRank) {
        std::cerr << "assertRankIs: FAIL: expectedRank = " << expectedRank << ", foundRank = " << foundRank << std::endl;
        std::cerr << "matrix:" << std::endl << m << std::endl;
        exit(EXIT_FAILURE);
    }
}

template<typename Field, unsigned M, unsigned N>
void testRank()
{
    std::cerr << "testRank()" << std::endl;
    Matrix<M, N, Field> m(0);

    assertRankIs(m, 0);

    m[0][0] = Field(1);
    assertRankIs(m, 1);

    for (size_t i = 1; i < N; ++i) {
        m[0][i] = Field(1);
    }
    assertRankIs(m, 1);

    if (std::min(M, N) >= 2) {
        for (size_t i = 1; i < M; ++i) {
            m[i][0] = Field(1);
        }
        assertRankIs(m, 2);
    }

    m.clear();
    for (size_t i = 0; i < N; ++i) {
        m[0][i] = Field(1);
    }
    assertRankIs(m, 1);

    m.clear();
    for (size_t i = 0; i < std::min(M, N); ++i) {
        m[i][i] = Field(1);
    }
    assertRankIs(m, std::min(M, N));
}

int main() {
    const int tests = 10;
    srand(time(NULL));

    testDetRandom<Rational, 1>(tests);
    testDetRandom<Rational, 2>(tests);
    testDetRandom<Rational, 3>(tests);

    testDetRandom<Finite<43>, 1>(tests);
    testDetRandom<Finite<43>, 2>(tests);
    testDetRandom<Finite<43>, 3>(tests);

    testInvertRandom<Rational, 2>(tests);
    testInvertRandom<Rational, 3>(tests);
    testInvertRandom<Finite<17>, 4>(100);
    testInvertRandom<Finite<43>, 4>(100);

    testMulRandom<Rational, 20, 20, 15>(tests);
    testMulRandom<Rational, 1, 1, 1>(tests);

    testRank<Rational, 10, 10>();
    testRank<Rational, 1, 1>();
    testRank<Rational, 10, 1>();
    testRank<Rational, 1, 10>();
}
