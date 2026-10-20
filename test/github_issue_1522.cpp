// Copyright 2026 Shen-Ta Hsieh
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//
// See: https://github.com/boostorg/decimal/issues/1522

#include <boost/decimal.hpp>
#include <boost/core/lightweight_test.hpp>
#include <limits>

using namespace boost::decimal;

// The exact product of finite x and y is finite, also above the largest value,
// so fma(x, y, z) is z for an infinite z
template <typename T>
void test_infinite_addend()
{
    #ifndef BOOST_DECIMAL_FAST_MATH
    const auto inf {std::numeric_limits<T>::infinity()};
    const auto big {(std::numeric_limits<T>::max)()};

    BOOST_TEST_EQ(fma(big, T(10), -inf), -inf);
    BOOST_TEST_EQ(fma(big, T(-10), inf), inf);
    BOOST_TEST_EQ(fma(-big, big, inf), inf);
    BOOST_TEST_EQ(fma(big, big, inf), inf);
    BOOST_TEST_EQ(fma(T(2), T(3), -inf), -inf);
    BOOST_TEST(isnan(fma(inf, T(-1), inf)));
    BOOST_TEST(isnan(fma(inf, T(0), T(1))));
    BOOST_TEST(isnan(fma(big, big, std::numeric_limits<T>::quiet_NaN())));
    #endif
}

int main()
{
    test_infinite_addend<decimal32_t>();
    test_infinite_addend<decimal64_t>();
    test_infinite_addend<decimal128_t>();
    test_infinite_addend<decimal_fast32_t>();
    test_infinite_addend<decimal_fast64_t>();
    test_infinite_addend<decimal_fast128_t>();

    return boost::report_errors();
}
