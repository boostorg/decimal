// Copyright 2026 Shen-Ta Hsieh
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//
// See: https://github.com/boostorg/decimal/issues/1504

#include <boost/decimal.hpp>
#include <boost/core/lightweight_test.hpp>
#include <climits>

using namespace boost::decimal;
using namespace boost::decimal::literals;

// lround and llround round half away from zero in each rounding mode, also when |x| is below 0.1
template <typename T>
void test_lround()
{
    constexpr int tiny {-std::numeric_limits<T>::digits10 - 5};

    BOOST_TEST_EQ(lround(T(5U, -1)), 1L);
    BOOST_TEST_EQ(lround(T(25U, -1)), 3L);
    BOOST_TEST_EQ(lround(T(25U, -1, true)), -3L);
    BOOST_TEST_EQ(lround(T(4U, -1)), 0L);
    BOOST_TEST_EQ(llround(T(15U, -1, true)), -2LL);
    BOOST_TEST_EQ(llround(T(1U, tiny)), 0LL);

    #ifndef BOOST_DECIMAL_NO_CONSTEVAL_DETECTION
    // fesetround has an effect only with the detection of constant evaluation
    fesetround(rounding_mode::fe_dec_upward);
    BOOST_TEST_EQ(lround(T(1U, tiny)), 0L);
    BOOST_TEST_EQ(llround(T(21U, -1)), 2LL);

    fesetround(rounding_mode::fe_dec_downward);
    BOOST_TEST_EQ(lround(T(1U, tiny, true)), 0L);
    BOOST_TEST_EQ(llround(T(21U, -1, true)), -2LL);

    fesetround(rounding_mode::fe_dec_to_nearest);
    BOOST_TEST_EQ(lround(T(25U, -1)), 3L);
    BOOST_TEST_EQ(llround(T(5U, -1, true)), -1LL);
    #endif
}

// llround saturates out of range and does not wrap to the other sign
void test_llround_range()
{
    BOOST_TEST_EQ(llround("1e20"_DD), LLONG_MAX);
    BOOST_TEST_EQ(llround("-1e20"_DD), LLONG_MIN);
    BOOST_TEST_EQ(llround("9.223372036854776e18"_DD), LLONG_MAX);
    BOOST_TEST_EQ(llround("-9.223372036854776e18"_DD), LLONG_MIN);
    BOOST_TEST_EQ(llround("9.223372036854776e18"_DDF), LLONG_MAX);
    BOOST_TEST_EQ(llround("-9.223372036854776e18"_DDF), LLONG_MIN);
    BOOST_TEST_EQ(llround("9223372036854775808"_DL), LLONG_MAX);
    BOOST_TEST_EQ(llround("-9223372036854775808"_DL), LLONG_MIN);
    BOOST_TEST_EQ(llround("9223372036854775807.5"_DL), LLONG_MAX);
    BOOST_TEST_EQ(llround("-9223372036854775807.5"_DL), LLONG_MIN);
    BOOST_TEST_EQ(llround("-9223372036854775808.4"_DL), LLONG_MIN);
    BOOST_TEST_EQ(llround("-9223372036854775808.6"_DL), LLONG_MIN);
    BOOST_TEST_EQ(llround("100000000000000000000.5"_DL), LLONG_MAX);
    BOOST_TEST_EQ(llround("9223372036854775806.5"_DL), LLONG_MAX);
    BOOST_TEST_EQ(llround("9223372036854775805.5"_DL), 9223372036854775806LL);
}

// The same values are out of range for a long of 32 or 64 bits
void test_lround_range()
{
    BOOST_TEST_EQ(lround("9.223372036854776e18"_DD), LONG_MAX);
    BOOST_TEST_EQ(lround("-9.223372036854776e18"_DD), LONG_MIN);
    BOOST_TEST_EQ(lround("9.223372036854776e18"_DDF), LONG_MAX);
    BOOST_TEST_EQ(lround("-9.223372036854776e18"_DDF), LONG_MIN);
    BOOST_TEST_EQ(lround("1e19"_DF), LONG_MAX);
    BOOST_TEST_EQ(lround("-1e19"_DL), LONG_MIN);
}

int main()
{
    test_lround<decimal32_t>();
    test_lround<decimal64_t>();
    test_lround<decimal128_t>();
    test_lround<decimal_fast32_t>();
    test_lround<decimal_fast64_t>();
    test_lround<decimal_fast128_t>();

    test_llround_range();
    test_lround_range();

    return boost::report_errors();
}
