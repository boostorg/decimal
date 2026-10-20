// Copyright 2026 Shen-Ta Hsieh
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//
// See: https://github.com/boostorg/decimal/issues/1520

#include <boost/decimal.hpp>
#include <boost/core/lightweight_test.hpp>
#include <limits>

using namespace boost::decimal;
using namespace boost::decimal::literals;

// C17 F.10.2.1 and F.10.2.3: acosh is NaN below 1 and atanh is NaN outside [-1, 1]
template <typename T>
void test()
{
    const T inf {std::numeric_limits<T>::infinity()};
    const T tiny {std::numeric_limits<T>::denorm_min()};

    BOOST_TEST(isnan(acosh(tiny)));
    BOOST_TEST(isnan(acosh(-tiny)));
    BOOST_TEST(isnan(acosh(-inf)));
    BOOST_TEST(isnan(acosh(T {0})));
    BOOST_TEST(isnan(acosh(std::numeric_limits<T>::quiet_NaN())));
    BOOST_TEST_EQ(acosh(inf), inf);
    BOOST_TEST_EQ(acosh(T {1}), T {0});

    BOOST_TEST(isnan(atanh(inf)));
    BOOST_TEST(isnan(atanh(-inf)));
    BOOST_TEST(isnan(atanh(std::numeric_limits<T>::quiet_NaN())));
    BOOST_TEST_EQ(atanh(T {1}), inf);
    BOOST_TEST_EQ(atanh(T {-1}), -inf);
    BOOST_TEST_EQ(atanh(tiny), tiny);
    BOOST_TEST_EQ(atanh(-tiny), -tiny);
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

    BOOST_TEST(isnan(acosh(0.5_DF)));
    BOOST_TEST(isnan(atanh(1.5_DD)));
    #endif

    return boost::report_errors();
}
