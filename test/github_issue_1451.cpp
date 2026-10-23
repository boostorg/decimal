// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//
// https://github.com/boostorg/decimal/issues/1451
//
// The far apart branch of add_impl rounded the magnitude of the result and not its value,
// thus a directed mode went the wrong way when the result was negative.

#include <boost/decimal.hpp>
#include <boost/core/lightweight_test.hpp>

using namespace boost::decimal;
using namespace boost::decimal::literals;

// fesetround changes the mode only when the library can find a constant evaluation.
#ifndef BOOST_DECIMAL_NO_CONSTEVAL_DETECTION

// The two orders of the operands go through two copies of the branch, and operator-
// flips the sign of the right operand on its own way into add_impl.
template <typename T>
void check(const rounding_mode mode, const T lhs, const T rhs, const T expected)
{
    fesetround(mode);
    BOOST_TEST_EQ(lhs + rhs, expected);
    BOOST_TEST_EQ(rhs + lhs, expected);
    BOOST_TEST_EQ(lhs - (-rhs), expected);
    BOOST_TEST_EQ(rhs - (-lhs), expected);
}

// tiny is too far below big to change its digits, thus it only selects the direction.
// above and below are the neighbours of big, and every value here is exact.
template <typename T>
void test_directed_sums(const T big, const T tiny, const T above, const T below)
{
    check(rounding_mode::fe_dec_upward, big, tiny, above);
    check(rounding_mode::fe_dec_upward, big, -tiny, big);
    check(rounding_mode::fe_dec_upward, -big, -tiny, -big);
    check(rounding_mode::fe_dec_upward, -big, tiny, -below);

    check(rounding_mode::fe_dec_downward, big, tiny, big);
    check(rounding_mode::fe_dec_downward, big, -tiny, below);
    check(rounding_mode::fe_dec_downward, -big, -tiny, -above);
    check(rounding_mode::fe_dec_downward, -big, tiny, -big);

    check(rounding_mode::fe_dec_toward_zero, big, tiny, big);
    check(rounding_mode::fe_dec_toward_zero, big, -tiny, below);
    check(rounding_mode::fe_dec_toward_zero, -big, -tiny, -big);
    check(rounding_mode::fe_dec_toward_zero, -big, tiny, -below);
}

// The exact x*y+z is -5693498000000000000000000000000000.000...000189, thus the downward
// mode must give the value below -5.693498e+33, and the upward mode that value itself.
void test_fma()
{
    fesetround(rounding_mode::fe_dec_downward);
    BOOST_TEST_EQ(fma(-4.872580e-23_DF, 3.879349e-03_DF, -5.693498e+33_DF), -5.693499e+33_DF);
    BOOST_TEST_EQ(fma(-4.872580e-23_DFF, 3.879349e-03_DFF, -5.693498e+33_DFF), -5.693499e+33_DFF);
    BOOST_TEST_EQ(fma(-4.872580e-23_DD, 3.879349e-03_DD, -5.693498e+33_DD), -5.693498000000001e+33_DD);
    BOOST_TEST_EQ(fma(-4.872580e-23_DDF, 3.879349e-03_DDF, -5.693498e+33_DDF), -5.693498000000001e+33_DDF);

    fesetround(rounding_mode::fe_dec_upward);
    BOOST_TEST_EQ(fma(-4.872580e-23_DF, 3.879349e-03_DF, -5.693498e+33_DF), -5.693498e+33_DF);
    BOOST_TEST_EQ(fma(-4.872580e-23_DFF, 3.879349e-03_DFF, -5.693498e+33_DFF), -5.693498e+33_DFF);
    BOOST_TEST_EQ(fma(-4.872580e-23_DD, 3.879349e-03_DD, -5.693498e+33_DD), -5.693498e+33_DD);
    BOOST_TEST_EQ(fma(-4.872580e-23_DDF, 3.879349e-03_DDF, -5.693498e+33_DDF), -5.693498e+33_DDF);
}

#endif

int main()
{
    #ifndef BOOST_DECIMAL_NO_CONSTEVAL_DETECTION

    // 1 is a power of ten and 5 is not, thus the two go through the two paths of a decrement.
    test_directed_sums(1_DF, 1e-50_DF, 1.000001_DF, 9.999999e-01_DF);
    test_directed_sums(5_DF, 1e-50_DF, 5.000001_DF, 4.999999_DF);
    test_directed_sums(1_DD, 1e-50_DD, 1.000000000000001_DD, 9.999999999999999e-01_DD);
    test_directed_sums(5_DD, 1e-50_DD, 5.000000000000001_DD, 4.999999999999999_DD);
    test_directed_sums(1_DL, 1e-50_DL, 1.000000000000000000000000000000001_DL, 9.999999999999999999999999999999999e-01_DL);
    test_directed_sums(5_DL, 1e-50_DL, 5.000000000000000000000000000000001_DL, 4.999999999999999999999999999999999_DL);
    test_directed_sums(1_DFF, 1e-50_DFF, 1.000001_DFF, 9.999999e-01_DFF);
    test_directed_sums(5_DFF, 1e-50_DFF, 5.000001_DFF, 4.999999_DFF);
    test_directed_sums(1_DDF, 1e-50_DDF, 1.000000000000001_DDF, 9.999999999999999e-01_DDF);
    test_directed_sums(5_DDF, 1e-50_DDF, 5.000000000000001_DDF, 4.999999999999999_DDF);
    test_directed_sums(1_DLF, 1e-50_DLF, 1.000000000000000000000000000000001_DLF, 9.999999999999999999999999999999999e-01_DLF);
    test_directed_sums(5_DLF, 1e-50_DLF, 5.000000000000000000000000000000001_DLF, 4.999999999999999999999999999999999_DLF);

    test_fma();

    fesetround(rounding_mode::fe_dec_to_nearest);

    #endif

    return boost::report_errors();
}
