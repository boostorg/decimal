// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//
// https://github.com/boostorg/decimal/issues/1453
//
// The divide dropped the remainder of its wide quotient before the constructor rounded it,
// thus a directed mode could not move away from the truncated quotient.

#include <boost/decimal.hpp>
#include <boost/core/lightweight_test.hpp>

using namespace boost::decimal;
using namespace boost::decimal::literals;

// fesetround changes the mode only when the library can find a constant evaluation.
#ifndef BOOST_DECIMAL_NO_CONSTEVAL_DETECTION

template <typename T>
void check(const rounding_mode mode, const T lhs, const T rhs, const T expected)
{
    fesetround(mode);
    BOOST_TEST_EQ(lhs / rhs, expected);
    BOOST_TEST_EQ(-lhs / -rhs, expected);
}

// below and above are the neighbours of the exact quotient, and nearest is the one of the
// two which is nearer to it. An exact quotient is its own neighbour in every mode.
template <typename T>
void test_quotients(const T lhs, const T rhs, const T below, const T above, const T nearest)
{
    check(rounding_mode::fe_dec_upward, lhs, rhs, above);
    check(rounding_mode::fe_dec_upward, -lhs, rhs, -below);

    check(rounding_mode::fe_dec_downward, lhs, rhs, below);
    check(rounding_mode::fe_dec_downward, -lhs, rhs, -above);

    check(rounding_mode::fe_dec_toward_zero, lhs, rhs, below);
    check(rounding_mode::fe_dec_toward_zero, -lhs, rhs, -below);

    check(rounding_mode::fe_dec_to_nearest_from_zero, lhs, rhs, nearest);
    check(rounding_mode::fe_dec_to_nearest_from_zero, -lhs, rhs, -nearest);
}

#endif

int main()
{
    #ifndef BOOST_DECIMAL_NO_CONSTEVAL_DETECTION

    // The last pair of each type has a quotient whose digits past the precision are zero for
    // more digits than the wide quotient holds, thus only the remainder can select the direction.
    test_quotients(1_DF, 4_DF, 2.5e-01_DF, 2.5e-01_DF, 2.5e-01_DF);
    test_quotients(1_DF, 3_DF, 3.333333e-01_DF, 3.333334e-01_DF, 3.333333e-01_DF);
    test_quotients(2_DF, 3_DF, 6.666666e-01_DF, 6.666667e-01_DF, 6.666667e-01_DF);
    test_quotients(1_DF, 1.000001_DF, 9.99999e-01_DF, 9.999991e-01_DF, 9.99999e-01_DF);

    test_quotients(1_DFF, 4_DFF, 2.5e-01_DFF, 2.5e-01_DFF, 2.5e-01_DFF);
    test_quotients(1_DFF, 3_DFF, 3.333333e-01_DFF, 3.333334e-01_DFF, 3.333333e-01_DFF);
    test_quotients(2_DFF, 3_DFF, 6.666666e-01_DFF, 6.666667e-01_DFF, 6.666667e-01_DFF);
    test_quotients(1_DFF, 1.000001_DFF, 9.99999e-01_DFF, 9.999991e-01_DFF, 9.99999e-01_DFF);

    test_quotients(1_DD, 4_DD, 2.5e-01_DD, 2.5e-01_DD, 2.5e-01_DD);
    test_quotients(1_DD, 3_DD, 3.333333333333333e-01_DD, 3.333333333333334e-01_DD, 3.333333333333333e-01_DD);
    test_quotients(2_DD, 3_DD, 6.666666666666666e-01_DD, 6.666666666666667e-01_DD, 6.666666666666667e-01_DD);
    test_quotients(1_DD, 1.000000000000001_DD, 9.99999999999999e-01_DD, 9.999999999999991e-01_DD, 9.99999999999999e-01_DD);

    test_quotients(1_DDF, 4_DDF, 2.5e-01_DDF, 2.5e-01_DDF, 2.5e-01_DDF);
    test_quotients(1_DDF, 3_DDF, 3.333333333333333e-01_DDF, 3.333333333333334e-01_DDF, 3.333333333333333e-01_DDF);
    test_quotients(2_DDF, 3_DDF, 6.666666666666666e-01_DDF, 6.666666666666667e-01_DDF, 6.666666666666667e-01_DDF);
    test_quotients(1_DDF, 1.000000000000001_DDF, 9.99999999999999e-01_DDF, 9.999999999999991e-01_DDF, 9.99999999999999e-01_DDF);

    test_quotients(1_DL, 4_DL, 2.5e-01_DL, 2.5e-01_DL, 2.5e-01_DL);
    test_quotients(1_DL, 3_DL, 3.333333333333333333333333333333333e-01_DL, 3.333333333333333333333333333333334e-01_DL, 3.333333333333333333333333333333333e-01_DL);
    test_quotients(2_DL, 3_DL, 6.666666666666666666666666666666666e-01_DL, 6.666666666666666666666666666666667e-01_DL, 6.666666666666666666666666666666667e-01_DL);
    test_quotients(1_DL, 1.000000000000000000000000000000001_DL, 9.99999999999999999999999999999999e-01_DL, 9.999999999999999999999999999999991e-01_DL, 9.99999999999999999999999999999999e-01_DL);

    test_quotients(1_DLF, 4_DLF, 2.5e-01_DLF, 2.5e-01_DLF, 2.5e-01_DLF);
    test_quotients(1_DLF, 3_DLF, 3.333333333333333333333333333333333e-01_DLF, 3.333333333333333333333333333333334e-01_DLF, 3.333333333333333333333333333333333e-01_DLF);
    test_quotients(2_DLF, 3_DLF, 6.666666666666666666666666666666666e-01_DLF, 6.666666666666666666666666666666667e-01_DLF, 6.666666666666666666666666666666667e-01_DLF);
    test_quotients(1_DLF, 1.000000000000000000000000000000001_DLF, 9.99999999999999999999999999999999e-01_DLF, 9.999999999999999999999999999999991e-01_DLF, 9.99999999999999999999999999999999e-01_DLF);

    fesetround(rounding_mode::fe_dec_to_nearest);

    #endif

    return boost::report_errors();
}
