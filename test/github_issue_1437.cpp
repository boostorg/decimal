// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//
// See: https://github.com/boostorg/decimal/issues/1437

#include <boost/decimal.hpp>
#include <boost/core/lightweight_test.hpp>
#include <limits>

using namespace boost::decimal;

// IEEE 754-2019 section 5.3.3: logb(x) returns the exponent e of x such that
// 1 <= scalb(abs(x), -e) < 10 for decimal formats
template <typename T>
void test_logb()
{
    BOOST_TEST_EQ(logb(T{"1E-3"}), T{-3});
    BOOST_TEST_EQ(logb(T{"1E3"}), T{3});
    BOOST_TEST_EQ(logb(T{1}), T{0});
    BOOST_TEST_EQ(logb(T{-1}), T{0});
    BOOST_TEST_EQ(logb(T{"9.999E2"}), T{2});
    BOOST_TEST_EQ(logb(T{"-1E-3"}), T{-3});

    // logb is cohort independent
    BOOST_TEST_EQ(logb(T{"1E-3"}), logb(T{"1000E-6"}));
    BOOST_TEST_EQ(logb(T{"1E3"}), logb(T{"1000"}));

    #ifndef BOOST_DECIMAL_FAST_MATH
    BOOST_TEST_EQ(logb(T{0}), -std::numeric_limits<T>::infinity());
    BOOST_TEST_EQ(logb(std::numeric_limits<T>::infinity()), std::numeric_limits<T>::infinity());
    BOOST_TEST(isnan(logb(std::numeric_limits<T>::quiet_NaN())));
    #endif
}

template <typename T>
void test_ilogb()
{
    BOOST_TEST_EQ(ilogb(T{"1E-3"}), -3);
    BOOST_TEST_EQ(ilogb(T{"1E3"}), 3);
    BOOST_TEST_EQ(ilogb(T{1}), 0);
    BOOST_TEST_EQ(ilogb(T{-1}), 0);
    BOOST_TEST_EQ(ilogb(T{"9.999E2"}), 2);
    BOOST_TEST_EQ(ilogb(T{"-1E-3"}), -3);

    BOOST_TEST_EQ(ilogb(T{"1E-3"}), ilogb(T{"1000E-6"}));
    BOOST_TEST_EQ(ilogb(T{"1E3"}), ilogb(T{"1000"}));

    BOOST_TEST_EQ(ilogb(T{0}), FP_ILOGB0);

    #ifndef BOOST_DECIMAL_FAST_MATH
    BOOST_TEST_EQ(ilogb(std::numeric_limits<T>::infinity()), INT_MAX);
    BOOST_TEST_EQ(ilogb(std::numeric_limits<T>::quiet_NaN()), FP_ILOGBNAN);
    #endif
}

int main()
{
    test_logb<decimal32_t>();
    test_logb<decimal64_t>();
    test_logb<decimal128_t>();

    test_logb<decimal_fast32_t>();
    test_logb<decimal_fast64_t>();
    test_logb<decimal_fast128_t>();

    test_ilogb<decimal32_t>();
    test_ilogb<decimal64_t>();
    test_ilogb<decimal128_t>();

    test_ilogb<decimal_fast32_t>();
    test_ilogb<decimal_fast64_t>();
    test_ilogb<decimal_fast128_t>();

    return boost::report_errors();
}
