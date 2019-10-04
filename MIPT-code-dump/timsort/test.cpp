#include <ctime>
#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <cstdio>
#include <vector>
#include <string>
#include <iterator>
#include "timsort.h"

template<class Operation>
double measure(Operation op) {
    std::clock_t timer = std::clock();
    op();
    return static_cast<double>(std::clock() - timer) / CLOCKS_PER_SEC;
}

template<class RAI, class Compare, class ITimSortParams>
void test(
        RAI first,
        RAI last,
        std::string testName,
        bool verbose,
        Compare comp,
        ITimSortParams params)
{
    std::vector<typename std::iterator_traits<RAI>::value_type> vec(first, last);
    const double timTime = measure([first, last, comp, params]() {
        TimSort::timSort(first, last, comp, params);
    });
    const double stdTime = measure([&vec, comp]() {
        std::sort(vec.begin(), vec.end(), comp);
    });
    if (!std::equal(first, last, vec.begin())) {
        std::fprintf(stderr, "%s n=%zu: FAIL\n", testName.c_str(), static_cast<std::size_t>(last - first));
        std::abort();
    }
    if (verbose) {
        std::fprintf(stderr,
            "%-50s n=%-10zu:    timSort: %3.3f,    std::sort: %3.3f\n",
            testName.c_str(), static_cast<std::size_t>(last - first), timTime, stdTime);
    }
}

template<class RAI, class Compare>
inline void test(
        RAI first,
        RAI last,
        std::string testName,
        bool verbose,
        Compare comp)
{
    test(first, last, testName, verbose, comp, TimSort::DefaultTimSortParams());
}

template<class RAI>
inline void test(
        RAI first,
        RAI last,
        std::string testName,
        bool verbose)
{
    test(first, last, testName, verbose, std::less<typename std::iterator_traits<RAI>::value_type>());
}


//------------------------------------------------------------------------------

class ArithProgGenerator {
    int cur_, first_, delta_, mod_;
public:
    ArithProgGenerator(int first, int delta, int modulo = 0)
        : cur_(first)
        , first_(first)
        , delta_(delta)
        , mod_(modulo)
    {}

    void reset() {
        cur_ = first_;
    }

    int operator ()() {
        int saved = cur_;
        if (mod_) {
            cur_ = (1LL * cur_ + delta_) % mod_;
        } else {
            cur_ += delta_;
        }
        return saved;
    }
};

class GeomProgGenerator {
    int cur_, first_, mul_, mod_;
public:
    GeomProgGenerator(int first, int mul, int modulo = 0)
        : cur_(first)
        , first_(first)
        , mul_(mul)
        , mod_(modulo)
    {}

    void reset() {
        cur_ = first_;
    }

    int operator ()() {
        int saved = cur_;
        if (mod_) {
            cur_ = (1LL * cur_ * mul_) % mod_;
        } else {
            cur_ *= mul_;
        }
        return saved;
    }
};

class RandomSeqGenerator {
    int mod_;
public:
    RandomSeqGenerator(int modulo = 0)
        : mod_(modulo)
    {}

    void reset() {}

    int operator ()() {
        int val = abs(rand());
        if (mod_) {
            val %= mod_;
        }
        return val;
    }
};

class RandomStringGenerator {
    int maxLen_;

public:
    RandomStringGenerator(int maxLen)
        : maxLen_(maxLen)
    {}

    void reset() {}

    std::string operator ()() {
        const std::size_t n = maxLen_ ? (abs(rand()) % maxLen_) : 0;
        std::string val(n, '@');
        for (std::size_t i = 0; i < n; ++i) {
            val[i] = 'a' + abs(rand()) % 26;
        }
        return val;
    }
};

struct Point3d {
    double x, y, z;

    Point3d(double x_, double y_, double z_)
        : x(x_)
        , y(y_)
        , z(z_)
    {}

    bool operator ==(Point3d that) const {
        return x == that.x && y == that.y && z == that.z;
    }

    bool operator !=(Point3d that) const {
        return !(*this == that);
    }
};

class RandomPoint3dGenerator {
    double maxCoordAbs_;

    static double randomDouble_() {
        return static_cast<double>(rand()) / RAND_MAX;
    }

public:
    RandomPoint3dGenerator(double maxCoordAbs)
        : maxCoordAbs_(maxCoordAbs)
    {}

    void reset() {}

    Point3d operator ()() {
        return Point3d(
            randomDouble_() * maxCoordAbs_,
            randomDouble_() * maxCoordAbs_,
            randomDouble_() * maxCoordAbs_);
    }
};

//------------------------------------------------------------------------------

template<class Generator, class Compare, class ITimSortParams>
void testPointerRAIOnSize(Generator gen, std::size_t n, std::string testName, bool verbose, Compare comp, ITimSortParams params) {
    std::vector<decltype(gen())> vec;
    vec.reserve(n);
    gen.reset();
    std::generate_n(std::back_inserter(vec), n, gen);
    test(vec.data(), vec.data() + n, testName, verbose, comp, params);
}

template<class Generator, class Compare>
inline void testPointerRAIOnSize(Generator gen, std::size_t n, std::string testName, bool verbose, Compare comp) {
    testPointerRAIOnSize(gen, n, testName, verbose, comp, TimSort::DefaultTimSortParams());
}

template<class Generator>
inline void testPointerRAIOnSize(Generator gen, std::size_t n, std::string testName, bool verbose) {
    testPointerRAIOnSize(gen, n, testName, verbose, std::less<decltype(gen())>());
}

//------------------------------------------------------------------------------

template<class Generator, class Compare, class ITimSortParams>
void testOnSize(Generator gen, std::size_t n, std::string testName, bool verbose, Compare comp, ITimSortParams params) {
    std::vector<decltype(gen())> vec;
    vec.reserve(n);
    gen.reset();
    std::generate_n(std::back_inserter(vec), n, gen);
    test(vec.begin(), vec.end(), testName, verbose, comp, params);
}

template<class Generator, class Compare>
inline void testOnSize(Generator gen, std::size_t n, std::string testName, bool verbose, Compare comp) {
    testOnSize(gen, n, testName, verbose, comp, TimSort::DefaultTimSortParams());
}

template<class Generator>
inline void testOnSize(Generator gen, std::size_t n, std::string testName, bool verbose) {
    testOnSize(gen, n, testName, verbose, std::less<decltype(gen())>());
}

//------------------------------------------------------------------------------

template<class Generator, class Compare, class ITimSortParams>
void testOnDifferentSizes(Generator gen, std::string testName, Compare comp, ITimSortParams params) {
    for (std::size_t i = 1; i < 70; ++i) {
        testOnSize(gen, i, testName, false, comp, params);
    }
    testOnSize(gen, 1e3, testName, false, comp, params);
    testOnSize(gen, 1e6, testName, true, comp, params);
    testOnSize(gen, 1e7, testName, true, comp, params);
}

template<class Generator, class Compare>
inline void testOnDifferentSizes(Generator gen, std::string testName, Compare comp) {
    testOnDifferentSizes(gen, testName, comp, TimSort::DefaultTimSortParams());
}

template<class Generator>
inline void testOnDifferentSizes(Generator gen, std::string testName) {
    testOnDifferentSizes(gen, testName, std::less<decltype(gen())>());
}

//------------------------------------------------------------------------------

int main() {
    srand(1477990773);

    testOnSize(ArithProgGenerator(0, 0), 0, "empty range (vector iterators)", true);

    testOnDifferentSizes(ArithProgGenerator(0, 0),  "-, asc");
    testOnDifferentSizes(ArithProgGenerator(0, 0),  "-, desc", std::greater<int>());
    testOnDifferentSizes(ArithProgGenerator(0, 1),  "/, asc");
    testOnDifferentSizes(ArithProgGenerator(0, 1),  "/, desc", std::greater<int>());
    testOnDifferentSizes(ArithProgGenerator(0, -1), "\\, asc");
    testOnDifferentSizes(ArithProgGenerator(0, -1), "\\, desc", std::greater<int>());

    testOnDifferentSizes(ArithProgGenerator(0, 1e6, 1e9 + 7), "~ (1), asc");
    testOnDifferentSizes(ArithProgGenerator(0, 1e6, 1e9 + 7), "~ (1), desc", std::greater<int>());
    testOnDifferentSizes(ArithProgGenerator(0, 1e6, 1e9 + 9), "~ (2), asc");
    testOnDifferentSizes(ArithProgGenerator(0, 1e6, 1e9 + 9), "~ (2), desc", std::greater<int>());
    testOnDifferentSizes(ArithProgGenerator(0, 31, 1033),     "~ (3), asc");
    testOnDifferentSizes(ArithProgGenerator(0, 31, 1033),     "~ (3), desc", std::greater<int>());
    testOnDifferentSizes(ArithProgGenerator(0, 1, 2),         "zero-one, asc");
    testOnDifferentSizes(ArithProgGenerator(0, 1, 2),         "zero-one, desc", std::greater<int>());

    testPointerRAIOnSize(ArithProgGenerator(0, 0), 0,   "empty range (pointers)", true);
    testPointerRAIOnSize(ArithProgGenerator(0, 1), 1e7, "/, asc (pointers)", true);
    testPointerRAIOnSize(ArithProgGenerator(0, 1), 1e7, "/, desc (pointers)", true, std::greater<int>());

    testOnSize(ArithProgGenerator(0, 1, 1024), 1024 * 10000, "partially sorted (1)", true);
    testOnSize(ArithProgGenerator(0, 1, 128),  128  * 1000,  "partially sorted (2)", true);
    testOnSize(ArithProgGenerator(0, 1, 80),   80   * 100,   "partially sorted (3)", true);

    testOnDifferentSizes(GeomProgGenerator(1, 2, 1e9 + 7), "~ (4), asc");
    testOnDifferentSizes(GeomProgGenerator(1, 2, 1e9 + 7), "~ (4), desc", std::greater<int>());
    testOnDifferentSizes(GeomProgGenerator(1, 2, 1e9 + 9), "~ (5), asc");
    testOnDifferentSizes(GeomProgGenerator(1, 2, 1e9 + 9), "~ (5), desc", std::greater<int>());
    testOnDifferentSizes(GeomProgGenerator(1, 2, 1033),    "~ (6), asc");
    testOnDifferentSizes(GeomProgGenerator(1, 3, 1033),    "~ (6), desc", std::greater<int>());

    testOnDifferentSizes(RandomSeqGenerator(),     "? (1)");
    testOnDifferentSizes(RandomSeqGenerator(2),    "? (2)");
    testOnDifferentSizes(RandomSeqGenerator(3),    "? (3)");
    testOnDifferentSizes(RandomSeqGenerator(4),    "? (4)");
    testOnDifferentSizes(RandomSeqGenerator(5),    "? (5)");
    testOnDifferentSizes(RandomSeqGenerator(100),  "? (6)");
    testOnDifferentSizes(RandomSeqGenerator(1000), "? (7)");

    testOnSize(RandomStringGenerator(16), 1e6, "random strings", true);
    testOnSize(RandomPoint3dGenerator(1000), 1e6, "random 3d points", true,
        [](Point3d a, Point3d b) {
            if (a.x != b.x) {
                return a.x < b.x;
            }
            if (a.y != b.y) {
                return a.y < b.y;
            }
            return a.z < b.z;
        }
    );
}
