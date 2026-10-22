// Copyright 2026 Shen-Ta Hsieh
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//
// See: https://github.com/boostorg/decimal/issues/1525

#include <boost/decimal.hpp>
#include <boost/core/lightweight_test.hpp>
#include <limits>

using namespace boost::decimal;

template <typename T>
bool is_pos_zero(const T x)
{
    return x == 0 && !signbit(x);
}

template <typename T>
bool is_quiet_nan(const T x)
{
    return isnan(x) && !issignaling(x);
}

// fdim(x, y) is +0 when x <= y and x - y when x > y, also for an infinite x or y.
// A NaN y gives NaN.
template <typename T>
void test_infinities()
{
    const auto inf {std::numeric_limits<T>::infinity()};
    const auto nan {std::numeric_limits<T>::quiet_NaN()};

    BOOST_TEST(is_pos_zero(fdim(-inf, T(1))));
    BOOST_TEST(is_pos_zero(fdim(-inf, -inf)));
    BOOST_TEST(is_pos_zero(fdim(inf, inf)));
    BOOST_TEST(isnan(fdim(inf, nan)));
    BOOST_TEST(isnan(fdim(-inf, nan)));
    BOOST_TEST_EQ(fdim(inf, T(1)), inf);
    BOOST_TEST_EQ(fdim(inf, -inf), inf);
    BOOST_TEST_EQ(fdim(T(1), -inf), inf);
    BOOST_TEST(is_pos_zero(fdim(T(1), inf)));
}

// fdim gives a quiet NaN when x or y is a signaling NaN
template <typename T>
void test_signaling_nan()
{
    const auto snan {std::numeric_limits<T>::signaling_NaN()};
    const auto inf {std::numeric_limits<T>::infinity()};
    const auto nan {std::numeric_limits<T>::quiet_NaN()};

    for (const auto other : {T(1), T(-1), T(0), inf, -inf, nan})
    {
        BOOST_TEST(is_quiet_nan(fdim(snan, other)));
        BOOST_TEST(is_quiet_nan(fdim(other, snan)));
        BOOST_TEST(is_quiet_nan(fdim(-snan, other)));
        BOOST_TEST(is_quiet_nan(fdim(other, -snan)));
    }
}

template <typename T>
void test()
{
    test_infinities<T>();
    test_signaling_nan<T>();
}

int main()
{
    #ifndef BOOST_DECIMAL_FAST_MATH
    test<decimal32_t>();
    test<decimal64_t>();
    test<decimal128_t>();
    test<decimal_fast32_t>();
    test<decimal_fast64_t>();
    test<decimal_fast128_t>();
    #endif

    return boost::report_errors();
}
