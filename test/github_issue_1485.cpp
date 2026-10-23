// Copyright 2026 Shen-Ta Hsieh
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//
// See: https://github.com/boostorg/decimal/issues/1485

#include <boost/decimal.hpp>
#include <boost/core/lightweight_test.hpp>
#include <climits>

using namespace boost::decimal;
using namespace boost::decimal::literals;

template <typename T>
bool is_pos_zero(const T x)
{
    return x == 0 && !signbit(x);
}

template <typename T>
bool is_neg_zero(const T x)
{
    return x == 0 && signbit(x);
}

// A zero result keeps the sign of x, also when |x| is below the last digit of the type,
// and modf, which uses ceil and floor, keeps it too
template <typename T>
void test_signed_zero()
{
    constexpr int tiny {-std::numeric_limits<T>::digits10 - 5};

    BOOST_TEST(is_neg_zero(ceil(T(5U, -1, true))));
    BOOST_TEST(is_neg_zero(ceil(T(9U, -1, true))));
    BOOST_TEST(is_neg_zero(ceil(T(1U, tiny, true))));
    BOOST_TEST(is_pos_zero(floor(T(5U, -1))));
    BOOST_TEST(is_pos_zero(floor(T(1U, tiny))));
    BOOST_TEST(is_neg_zero(round(T(4U, -1, true))));
    BOOST_TEST(is_neg_zero(round(T(1U, tiny, true))));
    BOOST_TEST(is_pos_zero(round(T(4U, -1))));
    BOOST_TEST(is_neg_zero(roundeven(T(5U, -1, true))));
    BOOST_TEST(is_neg_zero(roundeven(T(1U, tiny, true))));
    BOOST_TEST(is_pos_zero(roundeven(T(5U, -1))));
    BOOST_TEST(is_neg_zero(trunc(T(5U, -1, true))));
    BOOST_TEST(is_pos_zero(trunc(T(5U, -1))));

    T ip {};
    static_cast<void>(modf(T(4U, -1, true), &ip));
    BOOST_TEST(is_neg_zero(ip));

    // A value below the last digit and away from zero still goes to one
    BOOST_TEST_EQ(ceil(T(1U, tiny)), T(1));
    BOOST_TEST_EQ(floor(T(1U, tiny, true)), T(-1));
}

// rint, nearbyint, lrint and llrint round in the current mode, also when |x| is below 0.1
template <typename T>
void test_rint()
{
    constexpr int tiny {-std::numeric_limits<T>::digits10 - 5};

    BOOST_TEST_EQ(rint(T(5U, -1)), T(0));
    BOOST_TEST_EQ(rint(T(15U, -1)), T(2));
    BOOST_TEST(is_neg_zero(rint(T(4U, -1, true))));
    BOOST_TEST(is_neg_zero(nearbyint(T(1U, tiny, true))));

    #ifndef BOOST_DECIMAL_NO_CONSTEVAL_DETECTION
    // fesetround has an effect only with the detection of constant evaluation
    fesetround(rounding_mode::fe_dec_upward);
    BOOST_TEST_EQ(rint(T(1U, tiny)), T(1));
    BOOST_TEST_EQ(rint(T(4U, -2)), T(1));
    BOOST_TEST_EQ(nearbyint(T(11U, -1)), T(2));
    BOOST_TEST(is_neg_zero(rint(T(1U, tiny, true))));
    BOOST_TEST_EQ(lrint(T(1U, tiny)), 1L);
    BOOST_TEST_EQ(llrint(T(4U, -2)), 1LL);

    fesetround(rounding_mode::fe_dec_downward);
    BOOST_TEST_EQ(rint(T(1U, tiny, true)), T(-1));
    BOOST_TEST_EQ(rint(T(4U, -2, true)), T(-1));
    BOOST_TEST_EQ(nearbyint(T(19U, -1)), T(1));
    BOOST_TEST(is_pos_zero(rint(T(1U, tiny))));
    BOOST_TEST_EQ(lrint(T(1U, tiny, true)), -1L);
    BOOST_TEST_EQ(llrint(T(4U, -2, true)), -1LL);

    fesetround(rounding_mode::fe_dec_toward_zero);
    BOOST_TEST(is_neg_zero(rint(T(9U, -1, true))));
    BOOST_TEST_EQ(rint(T(19U, -1, true)), T(-1));

    fesetround(rounding_mode::fe_dec_to_nearest_from_zero);
    BOOST_TEST_EQ(rint(T(5U, -1, true)), T(-1));
    BOOST_TEST_EQ(rint(T(25U, -1)), T(3));
    BOOST_TEST(is_pos_zero(rint(T(4U, -2))));

    fesetround(rounding_mode::fe_dec_to_nearest);
    #endif
}

// llrint saturates out of range and does not wrap to the other sign
void test_llrint_range()
{
    BOOST_TEST_EQ(llrint("1e20"_DD), LLONG_MAX);
    BOOST_TEST_EQ(llrint("-1e20"_DD), LLONG_MIN);
    BOOST_TEST_EQ(llrint("9.223372036854776e18"_DD), LLONG_MAX);
    BOOST_TEST_EQ(llrint("-9.223372036854776e18"_DD), LLONG_MIN);
    BOOST_TEST_EQ(llrint("9.223372036854776e18"_DDF), LLONG_MAX);
    BOOST_TEST_EQ(llrint("-9.223372036854776e18"_DDF), LLONG_MIN);
    BOOST_TEST_EQ(llrint("9223372036854775808"_DL), LLONG_MAX);
    BOOST_TEST_EQ(llrint("-9223372036854775808"_DL), LLONG_MIN);
    BOOST_TEST_EQ(llrint("9223372036854775807.5"_DL), LLONG_MAX);
    BOOST_TEST_EQ(llrint("-9223372036854775808.4"_DL), LLONG_MIN);
    BOOST_TEST_EQ(llrint("-9223372036854775808.6"_DL), LLONG_MIN);
    BOOST_TEST_EQ(llrint("100000000000000000000.5"_DL), LLONG_MAX);
    BOOST_TEST_EQ(llrint("9223372036854775806.5"_DL), 9223372036854775806LL);
}

// IEEE 754 gives the result the exponent max(q(x), 0) for the types with cohorts
void test_quantum()
{
    BOOST_TEST_EQ(quantexp(ceil("1.5"_DF)), 0);
    BOOST_TEST_EQ(quantexp(ceil("-1.5"_DF)), 0);
    BOOST_TEST_EQ(quantexp(floor("-1.5"_DF)), 0);
    BOOST_TEST_EQ(quantexp(round("2.5"_DF)), 0);
    BOOST_TEST_EQ(quantexp(roundeven("2.5"_DF)), 0);
    BOOST_TEST_EQ(quantexp(ceil("2.0"_DF)), 0);
    BOOST_TEST_EQ(quantexp(ceil("2e3"_DF)), 3);
    BOOST_TEST_EQ(ceil("999999.9"_DF), "1000000"_DF);
    BOOST_TEST_EQ(quantexp(ceil("999999.9"_DF)), 0);

    BOOST_TEST_EQ(quantexp(ceil("1.25"_DD)), 0);
    BOOST_TEST_EQ(quantexp(floor("-1.25"_DD)), 0);
    BOOST_TEST_EQ(quantexp(round("-2.50"_DD)), 0);
    BOOST_TEST_EQ(quantexp(roundeven("3.50"_DD)), 0);
    BOOST_TEST_EQ(quantexp(floor("7.000"_DD)), 0);
    BOOST_TEST_EQ(ceil("999999999999999.9"_DD), "1000000000000000"_DD);
    BOOST_TEST_EQ(quantexp(ceil("999999999999999.9"_DD)), 0);

    BOOST_TEST_EQ(quantexp(ceil("1.25"_DL)), 0);
    BOOST_TEST_EQ(quantexp(floor("-1.25"_DL)), 0);
    BOOST_TEST_EQ(quantexp(round("12345678901234567890.5"_DL)), 0);
    BOOST_TEST_EQ(quantexp(roundeven("-0.5"_DL)), 0);
    BOOST_TEST_EQ(quantexp(round("5e2"_DL)), 2);
    BOOST_TEST_EQ(round("12345678901234567890.5"_DL), "12345678901234567891"_DL);
    BOOST_TEST_EQ(quantexp(rint("2.0"_DF)), 0);
    BOOST_TEST_EQ(quantexp(nearbyint("-2.50"_DD)), 0);
    BOOST_TEST_EQ(quantexp(trunc("-2.5"_DL)), 0);
}

int main()
{
    test_signed_zero<decimal32_t>();
    test_signed_zero<decimal64_t>();
    test_signed_zero<decimal128_t>();
    test_signed_zero<decimal_fast32_t>();
    test_signed_zero<decimal_fast64_t>();
    test_signed_zero<decimal_fast128_t>();

    test_rint<decimal32_t>();
    test_rint<decimal64_t>();
    test_rint<decimal128_t>();
    test_rint<decimal_fast32_t>();
    test_rint<decimal_fast64_t>();
    test_rint<decimal_fast128_t>();

    test_llrint_range();
    test_quantum();

    return boost::report_errors();
}
