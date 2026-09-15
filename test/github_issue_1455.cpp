// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//
// https://github.com/boostorg/decimal/issues/1455
//
// IEEE 754-2019 6.3: an exact cancel is +0 in every mode but downward, where it is -0.
// The add kernel took the sign of its left operand, and no path read the mode.

#include <boost/decimal.hpp>
#include <boost/core/lightweight_test.hpp>

using namespace boost::decimal;
using namespace boost::decimal::literals;

template <typename T>
void check_zero(const T value, const bool neg)
{
    BOOST_TEST(value == 0);
    BOOST_TEST_EQ(signbit(value), neg);
}

// one_shift and one_wide are one with a smaller exponent, thus the sum aligns them first.
template <typename T>
void check(const rounding_mode mode, const T one, const T one_shift, const T one_wide, const T zero)
{
    fesetround(mode);
    const bool neg {mode == rounding_mode::fe_dec_downward};

    check_zero(one - one, neg);
    check_zero(-one + one, neg);
    check_zero(one + (-one), neg);
    check_zero(-one - (-one), neg);
    check_zero(one_shift - one, neg);
    check_zero(-one_shift + one, neg);
    check_zero(one_wide - one, neg);
    check_zero(-one_wide + one, neg);

    check_zero(zero + (-zero), neg);
    check_zero(-zero + zero, neg);
    check_zero(zero - zero, neg);
    check_zero(-zero - (-zero), neg);

    // A sum of like signs keeps its sign in every mode
    check_zero(zero + zero, false);
    check_zero(zero - (-zero), false);
    check_zero(-zero + (-zero), true);
    check_zero(-zero - zero, true);

    check_zero(one * one + (-one), neg);
    check_zero(fma(one, one, -one), neg);
    check_zero(fma(-one, one, one), neg);
    check_zero(fma(one_shift, one, -one), neg);
    check_zero(fma(zero, one, -zero), neg);
}

template <typename T>
void test(const T one, const T one_shift, const T one_wide, const T zero)
{
    #ifdef BOOST_DECIMAL_NO_CONSTEVAL_DETECTION
    // fesetround has no effect here, thus only the compile-time mode is tested
    check(_boost_decimal_global_rounding_mode, one, one_shift, one_wide, zero);
    #else
    check(rounding_mode::fe_dec_to_nearest, one, one_shift, one_wide, zero);
    check(rounding_mode::fe_dec_downward, one, one_shift, one_wide, zero);
    check(rounding_mode::fe_dec_upward, one, one_shift, one_wide, zero);
    check(rounding_mode::fe_dec_toward_zero, one, one_shift, one_wide, zero);
    check(rounding_mode::fe_dec_to_nearest_from_zero, one, one_shift, one_wide, zero);
    #endif
}

int main()
{
    static_assert(signbit(-1_DF + 1_DF) == (_boost_decimal_global_rounding_mode == rounding_mode::fe_dec_downward), "");
    static_assert(signbit(-1_DL + 1_DL) == (_boost_decimal_global_rounding_mode == rounding_mode::fe_dec_downward), "");

    test(1_DF, 1.0_DF, 1.000000_DF, 0_DF);
    test(1_DFF, 1.0_DFF, 1.000000_DFF, 0_DFF);
    test(1_DD, 1.0_DD, 1.000000_DD, 0_DD);
    test(1_DDF, 1.0_DDF, 1.000000_DDF, 0_DDF);
    test(1_DL, 1.0_DL, 1.000000_DL, 0_DL);
    test(1_DLF, 1.0_DLF, 1.000000_DLF, 0_DLF);

    fesetround(rounding_mode::fe_dec_to_nearest);

    return boost::report_errors();
}
