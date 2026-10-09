// Copyright 2026 Shen-Ta Hsieh
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//
// See: https://github.com/boostorg/decimal/issues/1561

#include <boost/decimal.hpp>
#include <boost/core/lightweight_test.hpp>
#include "quiet_nan.hpp"
#include <limits>

using namespace boost::decimal;
using namespace boost::decimal::literals;

// The expected values are 2^x rounded once, from Python's decimal module
void test_nearest()
{
    // 2^n for an integer n is exact before the rounding, which can see a tie
    BOOST_TEST_EQ(exp2(3_DF), 8_DF);
    BOOST_TEST_EQ(exp2(-4_DF), 0.0625_DF);
    BOOST_TEST_EQ(exp2(53_DD), 9007199254740992_DD);
    BOOST_TEST_EQ(exp2(-23_DD), 1.192092895507812e-7_DD);
    BOOST_TEST_EQ(exp2(127_DL), "1.701411834604692317316873037158841e38"_DL);
    BOOST_TEST_EQ(exp2(-55_DL), "2.775557561562891351059079170227051e-17"_DL);
    BOOST_TEST_EQ(exp2(128_DL), "3.402823669209384634633746074317682e38"_DL);
    BOOST_TEST_EQ(exp2(-56_DLF), "1.387778780781445675529539585113525e-17"_DLF);

    BOOST_TEST_EQ(exp2(0.5_DD), 1.414213562373095_DD);
    BOOST_TEST_EQ(exp2(-0.5_DDF), 0.7071067811865475_DDF);
    BOOST_TEST_EQ(exp2(2.5_DF), 5.656854_DF);
    BOOST_TEST_EQ(exp2(10.3_DFF), 1260.692_DFF);
    BOOST_TEST_EQ(exp2(0.012_DD), 1.008352454894595_DD);
    BOOST_TEST_EQ(exp2("100.25"_DL), "1507499113128880389969770996485.783"_DL);
    BOOST_TEST_EQ(exp2(1000_DD), 1.071508607186267e301_DD);
    BOOST_TEST_EQ(exp2(-1300_DD), 4.581478e-392_DD);
    BOOST_TEST_EQ(exp2(1e-30_DD), 1_DD);
    BOOST_TEST_EQ(exp2(-1e-30_DD), 1_DD);

    #ifndef BOOST_DECIMAL_FAST_MATH
    BOOST_TEST_EQ(exp2(2000_DD), std::numeric_limits<decimal64_t>::infinity());
    BOOST_TEST_EQ(exp2(-2000_DD), 0_DD);
    BOOST_TEST_EQ(exp2(std::numeric_limits<decimal64_t>::infinity()), std::numeric_limits<decimal64_t>::infinity());
    BOOST_TEST_EQ(exp2(-std::numeric_limits<decimal64_t>::infinity()), 0_DD);
    BOOST_TEST(isnan(exp2(std::numeric_limits<decimal64_t>::quiet_NaN())));
    #endif
    BOOST_TEST_EQ(exp2(0_DD), 1_DD);
}

void test_directed()
{
    #ifndef BOOST_DECIMAL_NO_CONSTEVAL_DETECTION
    // fesetround has an effect only with the detection of constant evaluation
    fesetround(rounding_mode::fe_dec_upward);
    BOOST_TEST_EQ(exp2(3_DF), 8_DF);
    BOOST_TEST_EQ(exp2(53_DD), 9007199254740992_DD);
    BOOST_TEST_EQ(exp2(1e-30_DD), 1.000000000000001_DD);
    BOOST_TEST_EQ(exp2(1e-390_DD), 1.000000000000001_DD);
    BOOST_TEST_EQ(exp2(0.5_DF), 1.414214_DF);
    BOOST_TEST_EQ(exp2(-2000_DD), std::numeric_limits<decimal64_t>::denorm_min());

    fesetround(rounding_mode::fe_dec_downward);
    BOOST_TEST_EQ(exp2(-4_DD), 0.0625_DD);
    BOOST_TEST_EQ(exp2(-1e-30_DD), 0.9999999999999999_DD);
    BOOST_TEST_EQ(exp2(-1e-390_DD), 0.9999999999999999_DD);
    BOOST_TEST_EQ(exp2(0.5_DF), 1.414213_DF);
    BOOST_TEST_EQ(exp2(2000_DD), (std::numeric_limits<decimal64_t>::max)());

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
        BOOST_TEST(is_quiet_of(exp2(s), s));
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
