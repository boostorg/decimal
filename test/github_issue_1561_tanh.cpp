// Copyright 2026 Shen-Ta Hsieh
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//
// See: https://github.com/boostorg/decimal/issues/1561

#include <boost/decimal.hpp>
#include <boost/core/lightweight_test.hpp>
#include "quiet_nan.hpp"

using namespace boost::decimal;
using namespace boost::decimal::literals;

// The expected values are tanh(x) rounded once, from Python's decimal module
void test_nearest()
{
    // The Taylor series of sinh and cosh, for |x| < 1/16
    BOOST_TEST_EQ(tanh(0.05_DD), 0.04995837495787997_DD);
    BOOST_TEST_EQ(tanh(-0.0123_DF), -0.01229938_DF);
    BOOST_TEST_EQ(tanh("0.0624"_DL), "0.06231913573607965461598295019095125"_DL);
    BOOST_TEST_EQ(tanh(1e-30_DD), 1e-30_DD);
    BOOST_TEST_EQ(tanh(0.01_DL), "0.009999666679999460339328919708839498"_DL);
    BOOST_TEST_EQ(tanh(3e-16_DL), "2.999999999999999999999999999999910e-16"_DL);
    BOOST_TEST_EQ(tanh(1e-20_DL), 1e-20_DL);

    // (1 - t) / (1 + t) with t = exp(-2|x|)
    BOOST_TEST_EQ(tanh("0.0625"_DL), "0.06241874674751251449014289119421133"_DL);
    BOOST_TEST_EQ(tanh(-0.25_DD), -0.2449186624037091_DD);
    BOOST_TEST_EQ(tanh(1_DD), 0.7615941559557649_DD);
    BOOST_TEST_EQ(tanh(0.5_DF), 0.4621172_DF);
    BOOST_TEST_EQ(tanh(5_DFF), 0.9999092_DFF);
    BOOST_TEST_EQ(tanh(2.5_DDF), 0.9866142981514303_DDF);
    BOOST_TEST_EQ(tanh("0.7"_DLF), "0.6043677771171634963086871831038265"_DLF);
    BOOST_TEST_EQ(tanh(3_DL), "0.9950547536867304513318801852554885"_DL);
    BOOST_TEST_EQ(tanh(15_DL), "0.9999999999998128475406232140209232"_DL);
    BOOST_TEST_EQ(tanh(20_DD), 1_DD);
    BOOST_TEST_EQ(tanh(50_DD), 1_DD);
    BOOST_TEST_EQ(tanh(-1000_DD), -1_DD);

    // Next to a rounding boundary, where the first try in one word cannot round
    BOOST_TEST_EQ(tanh(0.482_DD), 0.4478439131457828_DD);
    BOOST_TEST_EQ(tanh(1.06_DD), 0.7856638590269437_DD);

    // Next to a rounding boundary, where the first try in two words cannot round
    BOOST_TEST_EQ(tanh(0.8_DL), "0.6640367702678489636848446564002429"_DL);

    // Next to a rounding boundary and to 1, where the values of x next to each other give results that three words
    // of the quotient do not tell apart
    BOOST_TEST_EQ(tanh("-39.03237480524167175042270718447927"_DL), "-0.9999999999999999999999999999999997"_DL);
}

void test_directed()
{
    #ifndef BOOST_DECIMAL_NO_CONSTEVAL_DETECTION
    // fesetround has an effect only with the detection of constant evaluation
    fesetround(rounding_mode::fe_dec_downward);
    BOOST_TEST_EQ(tanh(1.2e-15_DL), "1.199999999999999999999999999999424e-15"_DL);
    BOOST_TEST_EQ(tanh(1e-30_DD), 9.999999999999999e-31_DD);
    BOOST_TEST_EQ(tanh(2e-390_DD), 1.99999999e-390_DD);
    BOOST_TEST_EQ(tanh(20_DD), 0.9999999999999999_DD);
    BOOST_TEST_EQ(tanh(9_DF), 0.9999999_DF);
    BOOST_TEST_EQ(tanh(50_DL), "0.9999999999999999999999999999999999"_DL);
    BOOST_TEST_EQ(tanh(1000_DD), 0.9999999999999999_DD);
    BOOST_TEST_EQ(tanh(-1000_DD), -1_DD);

    fesetround(rounding_mode::fe_dec_upward);
    BOOST_TEST_EQ(tanh(-1e-30_DD), -9.999999999999999e-31_DD);
    BOOST_TEST_EQ(tanh(20_DD), 1_DD);
    BOOST_TEST_EQ(tanh(0.5_DF), 0.4621172_DF);
    BOOST_TEST_EQ(tanh("38.59464043656472178260823211117293"_DL), "0.9999999999999999999999999999999995"_DL);

    fesetround(rounding_mode::fe_dec_toward_zero);
    BOOST_TEST_EQ(tanh(1e-30_DD), 9.999999999999999e-31_DD);
    BOOST_TEST_EQ(tanh(-40_DD), -0.9999999999999999_DD);

    fesetround(rounding_mode::fe_dec_to_nearest);
    #endif
}

// A signaling NaN operand gives a quiet NaN (IEEE 754 6.2)
template <typename T>
void test_snan()
{
    #ifndef BOOST_DECIMAL_FAST_MATH
    for (const auto s : {snan<T>("7"), -snan<T>("7")})
    {
        BOOST_TEST(is_quiet_of(tanh(s), s));
    }
    #endif
}

int main()
{
    test_nearest();
    test_directed();
    test_snan<decimal32_t>();
    test_snan<decimal64_t>();
    test_snan<decimal128_t>();
    test_snan<decimal_fast32_t>();
    test_snan<decimal_fast64_t>();
    test_snan<decimal_fast128_t>();

    return boost::report_errors();
}
