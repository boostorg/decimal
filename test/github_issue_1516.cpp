// Copyright 2026 Shen-Ta Hsieh
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//
// https://github.com/boostorg/decimal/issues/1516
//
// fmod and remquo used x - n * y, which is not exact. remainder halved a
// subnormal y, which can round, and remquo took a halfway quotient to zero.

#include <boost/decimal.hpp>
#include <boost/core/lightweight_test.hpp>

using namespace boost::decimal;
using namespace boost::decimal::literals;

// The quotient has more digits than the type, and 1e90 / 1e-90 overflows.
void test_far_exponents()
{
    BOOST_TEST_EQ(fmod(1e20_DF, 3_DF), 1_DF);
    BOOST_TEST_EQ(fmod(1e90_DF, 1e-90_DF), 0_DF);
    BOOST_TEST_EQ(fmod(9999999e80_DF, 7_DF), 4_DF);
    BOOST_TEST_EQ(remainder(9999999e80_DF, 7_DF), -3_DF);
    BOOST_TEST_EQ(fmod(1e90_DF, 7_DF), 1_DF);
    BOOST_TEST_EQ(fmod(1e20_DFF, 3_DFF), 1_DFF);
    BOOST_TEST_EQ(fmod(1e90_DFF, 1e-90_DFF), 0_DFF);
    BOOST_TEST_EQ(remainder(9999999e80_DFF, 7_DFF), -3_DFF);

    BOOST_TEST_EQ(fmod(1e40_DD, 3_DD), 1_DD);
    BOOST_TEST_EQ(fmod(1e300_DD, 7_DD), 1_DD);
    BOOST_TEST_EQ(remainder(1e300_DD, 7_DD), 1_DD);
    BOOST_TEST_EQ(fmod(1e40_DDF, 3_DDF), 1_DDF);
    BOOST_TEST_EQ(fmod(1e300_DDF, 7_DDF), 1_DDF);

    BOOST_TEST_EQ(fmod(1e56_DL, 7.000000000000000000000000000000001_DL), 2.285714285716326530612244897959184_DL);
    BOOST_TEST_EQ(fmod(1e56_DLF, 7.000000000000000000000000000000001_DLF), 2.285714285716326530612244897959184_DLF);
    BOOST_TEST_EQ(fmod(-1e40_DL, 3_DL), -1_DL);
    BOOST_TEST_EQ(fmod(1e6000_DL, 7_DL), 1_DL);
    BOOST_TEST_EQ(fmod(1e6000_DL, 7.000000000000000000000000000000001_DL), 5.377632392990291431618479088598623_DL);
    BOOST_TEST_EQ(fmod(1e6000_DLF, 7.000000000000000000000000000000001_DLF), 5.377632392990291431618479088598623_DLF);
}

// The exponents are 44 apart, and 34 digits times 10^44 is above 2^256.
void test_u256_range()
{
    BOOST_TEST_EQ(fmod(9999999999999999999999999999999999e10_DL, 7e-34_DL), 6e-34_DL);
    BOOST_TEST_EQ(remainder(9999999999999999999999999999999999e10_DL, 7e-34_DL), -1e-34_DL);
    BOOST_TEST_EQ(fmod(9999999999999999999999999999999999e10_DLF, 7e-34_DLF), 6e-34_DLF);

    // y divides x, so the remainder becomes zero and the reduction stops
    BOOST_TEST_EQ(fmod(1234567890123456789012345678901234e66_DL, 1234567890123456789012345678901234_DL), 0_DL);
    BOOST_TEST_EQ(fmod(1234567890123456789012345678901234e66_DLF, 1234567890123456789012345678901234_DLF), 0_DLF);
}

// Half of 7e-101 is 3.5e-101, which rounds to 4e-101 in decimal32_t.
void test_subnormal_remainder()
{
    BOOST_TEST_EQ(fmod(-6.8e-17_DF, 7e-101_DF), -4e-101_DF);
    BOOST_TEST_EQ(remainder(-6.8e-17_DF, 7e-101_DF), 3e-101_DF);
    BOOST_TEST_EQ(remainder(-9937.527725213_DD, 3e-398_DD), 1e-398_DD);

    // fmod is exact here, so only the halved y moves the result
    BOOST_TEST_EQ(remainder(4e-101_DF, 7e-101_DF), -3e-101_DF);
    BOOST_TEST_EQ(remainder(2e-398_DD, 3e-398_DD), -1e-398_DD);
}

// Platforms keep 3 or 4 quotient bits, so compare got % 8 (it keeps the sign)
template <typename T>
void check_remquo(const T x, const T y, const T expected, const int quo)
{
    int got {};
    const T result {remquo(x, y, &got)};
    BOOST_TEST_EQ(result, expected);
    BOOST_TEST_EQ(signbit(result), signbit(expected));
    BOOST_TEST_EQ(got % 8, quo);
}

// The quotient has more digits than the type, and 2.586e55 / 4.002e-50 overflows.
void test_remquo_far_exponents()
{
    check_remquo(1e20_DF, 3_DF, 1_DF, 5);
    check_remquo(2.586e55_DF, 4.002e-50_DF, -1.854e-50_DF, 7);
    check_remquo(-0.65911_DF, -4.658e-53_DF, -3e-54_DF, 2);
    check_remquo(1e20_DFF, 3_DFF, 1_DFF, 5);

    check_remquo(5.359702e22_DD, 7.797906e12_DD, -4.93556e11_DD, 2);
    check_remquo(1e300_DD, 7_DD, 1_DD, 1);
    check_remquo(1e300_DDF, 7_DDF, 1_DDF, 1);

    check_remquo(7.26613176911193852227549e327_DL, 7.321343798770885600491038e-499_DL, 1.54707777430105082307737e-499_DL, 5);
    check_remquo(7.26613176911193852227549e327_DLF, 7.321343798770885600491038e-499_DLF, 1.54707777430105082307737e-499_DLF, 5);
}

// y divides x, so the remainder becomes zero before the last step
void test_remquo_exact_division()
{
    check_remquo(1e90_DF, 1_DF, 0_DF, 0);
    check_remquo(1e300_DD, 1_DD, 0_DD, 0);
    check_remquo(1e100_DL, 1_DL, 0_DL, 0);
    check_remquo(1e100_DLF, 1_DLF, 0_DLF, 0);
    check_remquo(1234567890123456789012345678901234e66_DL, 1234567890123456789012345678901234_DL, 0_DL, 0);
    check_remquo(-1e100_DL, 1_DL, -0_DL, 0);
}

// A halfway quotient goes to the even integer
void test_remquo_halfway()
{
    check_remquo(7_DF, 2_DF, -1_DF, 4);
    check_remquo(-7_DF, 2_DF, 1_DF, -4);
    check_remquo(5_DF, 2_DF, 1_DF, 2);
    check_remquo(7_DD, 2_DD, -1_DD, 4);
    check_remquo(7_DL, 2_DL, -1_DL, 4);
    check_remquo(7_DLF, 2_DLF, -1_DLF, 4);
}

// An infinite y leaves x, and the quotient is zero
void test_remquo_infinite_divisor()
{
    #ifndef BOOST_DECIMAL_FAST_MATH
    check_remquo(5_DF, std::numeric_limits<decimal32_t>::infinity(), 5_DF, 0);
    check_remquo(-5_DD, std::numeric_limits<decimal64_t>::infinity(), -5_DD, 0);
    check_remquo(5_DLF, -std::numeric_limits<decimal_fast128_t>::infinity(), 5_DLF, 0);

    int quo {};
    BOOST_TEST(isnan(remquo(std::numeric_limits<decimal32_t>::infinity(), std::numeric_limits<decimal32_t>::infinity(), &quo)));
    #endif
}

// A zero x is its own remainder, even for a 34-digit y (3 in decimal_fast128_t)
void test_zero()
{
    BOOST_TEST_EQ(fmod(0_DLF, 3_DLF), 0_DLF);
    BOOST_TEST(signbit(fmod(-0_DLF, 3_DLF)));
    BOOST_TEST_EQ(remainder(0_DLF, 3_DLF), 0_DLF);
    BOOST_TEST_EQ(fmod(0_DL, 1.234567890123456789012_DL), 0_DL);
}

// The result has the exponent min(ex, ey), as IEEE 754 prefers. The
// constructors keep the cohort, which a literal would not.
void test_cohort()
{
    int quo {};
    BOOST_TEST_EQ(to_bits(fmod(decimal32_t {5000U, -3}, 3_DF)), to_bits(decimal32_t {2000U, -3}));
    BOOST_TEST_EQ(to_bits(remainder(decimal32_t {5000U, -3}, 3_DF)), to_bits(decimal32_t {1000U, -3, true}));
    BOOST_TEST_EQ(to_bits(remquo(5_DF, 3_DF, &quo)), to_bits(decimal32_t {1U, 0, true}));
    BOOST_TEST_EQ(to_bits(remquo(5_DD, decimal64_t {300U, -2}, &quo)), to_bits(decimal64_t {100U, -2, true}));
}

int main()
{
    test_far_exponents();
    test_u256_range();
    test_subnormal_remainder();
    test_remquo_far_exponents();
    test_remquo_exact_division();
    test_remquo_halfway();
    test_remquo_infinite_divisor();
    test_zero();
    test_cohort();

    return boost::report_errors();
}
