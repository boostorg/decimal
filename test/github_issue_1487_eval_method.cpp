// Copyright 2026 Shen-Ta Hsieh
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//
// https://github.com/boostorg/decimal/issues/1487
//
// frexp with the widest evaluation type. Each value is just below a power of two, so the fraction
// in decimal128_t is just below 1, and it rounds to 1 in the type of the value.

#define BOOST_DECIMAL_DEC_EVAL_METHOD 2

#include <boost/decimal.hpp>
#include <boost/core/lightweight_test.hpp>

using namespace boost::decimal;
using namespace boost::decimal::literals;

template <typename T>
void test(const T v, const T frac, const int expon)
{
    int e {};
    BOOST_TEST_EQ(frexp(v, &e), frac);
    BOOST_TEST_EQ(e, expon);
}

int main()
{
    // 9223372e12 = 2^63 * 0.999999996, 9536743e-13 = 2^-20 * 0.99999998,
    // and 4722366482869645e6 = 2^72 * 0.99999999999999995
    test(9223372e12_DF, 0.5_DF, 64);
    test(9536743e-13_DF, 0.5_DF, -19);
    test(static_cast<decimal_fast32_t>(9223372e12_DF), static_cast<decimal_fast32_t>(0.5_DF), 64);
    test(4722366482869645e6_DD, 0.5_DD, 73);
    test(static_cast<decimal_fast64_t>(4722366482869645e6_DD), static_cast<decimal_fast64_t>(0.5_DD), 73);

    return boost::report_errors();
}
