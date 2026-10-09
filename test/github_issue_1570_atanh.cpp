// Copyright 2026 Shen-Ta Hsieh
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//
// See: https://github.com/boostorg/decimal/issues/1570

#include <boost/decimal.hpp>
#include <boost/core/lightweight_test.hpp>
#include "quiet_nan.hpp"

using namespace boost::decimal;
using namespace boost::decimal::literals;

// The expected values are atanh(x) rounded once, from Python's decimal module
void test_nearest()
{
    BOOST_TEST_EQ(atanh(0.05_DF), 0.05004173_DF);
    BOOST_TEST_EQ(atanh(0.05_DD), 0.05004172927849127_DD);
    BOOST_TEST_EQ(atanh(0.05_DL), "0.05004172927849126824578527423892595"_DL);
    BOOST_TEST_EQ(atanh(0.01_DL), "0.01000033335333476201588210755140422"_DL);
    BOOST_TEST_EQ(atanh(-0.02799547_DF), -0.02800279_DF);
    BOOST_TEST_EQ(atanh(0.4180510_DF), 0.4453279_DF);
    BOOST_TEST_EQ(atanh(0.003466968059984195_DD), 0.003466981950916645_DD);
    BOOST_TEST_EQ(atanh(0.2808509509025428_DD), 0.2886056523155067_DD);
    // Where two words leave the result to three, and one word to two
    BOOST_TEST_EQ(atanh(0.115_DL), "0.1155110194431448083124047995404583"_DL);
    BOOST_TEST_EQ(atanh(0.144_DD), 0.1450078978990006_DD);
    BOOST_TEST_EQ(atanh("-0.003018523174517859271429957865475320"_DL), "-0.003018532342307991743812064044714129"_DL);
    BOOST_TEST_EQ(atanh("0.5288865824484114951689689421832047"_DL), "0.5885980794764472077885930880608001"_DL);
    BOOST_TEST_EQ(atanh(0.4180510_DFF), 0.4453279_DFF);
    BOOST_TEST_EQ(atanh(0.2808509509025428_DDF), 0.2886056523155067_DDF);
    BOOST_TEST_EQ(atanh("0.5288865824484114951689689421832047"_DLF), "0.5885980794764472077885930880608001"_DLF);

    // Arguments next to 0 and 1
    BOOST_TEST_EQ(atanh(0.001_DD), 0.001000000333333533_DD);
    BOOST_TEST_EQ(atanh(-0.003_DF), -0.003000009_DF);
    BOOST_TEST_EQ(atanh(0.5_DD), 0.5493061443340548_DD);
    BOOST_TEST_EQ(atanh(-0.3_DL), "-0.3095196042031117154740673490610694"_DL);
    BOOST_TEST_EQ(atanh(0.9999999_DF), 8.405621_DF);
    BOOST_TEST_EQ(atanh("0.9999999999999999999999999999999999"_DL), "39.49052017117874928301447079036328"_DL);
}

void test_directed()
{
    #ifndef BOOST_DECIMAL_NO_CONSTEVAL_DETECTION
    // fesetround has an effect only with the detection of constant evaluation
    fesetround(rounding_mode::fe_dec_downward);
    BOOST_TEST_EQ(atanh(1.2e-15_DL), "1.200000000000000000000000000000576e-15"_DL);
    BOOST_TEST_EQ(atanh(0.8733017_DF), 1.346824_DF);
    BOOST_TEST_EQ(atanh(-1E-398_DD), -2E-398_DD);

    fesetround(rounding_mode::fe_dec_upward);
    BOOST_TEST_EQ(atanh(-0.01731244_DF), -0.01731416_DF);
    BOOST_TEST_EQ(atanh(-0.6398902041436529_DD), -0.7579877982218560_DD);
    BOOST_TEST_EQ(atanh("0.5840820523483254216492627246707359"_DL), "0.6686361860864968556693637807000492"_DL);
    BOOST_TEST_EQ(atanh(1E-398_DD), 2E-398_DD);

    fesetround(rounding_mode::fe_dec_toward_zero);
    BOOST_TEST_EQ(atanh(0.006146170873574939_DD), 0.006146248266717101_DD);
    BOOST_TEST_EQ(atanh(-0.6240562_DF), -0.7316212_DF);
    BOOST_TEST_EQ(atanh(1E-398_DD), 1E-398_DD);

    fesetround(rounding_mode::fe_dec_to_nearest_from_zero);
    BOOST_TEST_EQ(atanh(-0.2686822_DF), -0.2754429_DF);
    BOOST_TEST_EQ(atanh(0.4608214115888064_DD), 0.4983536591419257_DD);

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
        BOOST_TEST(is_quiet_of(atanh(s), s));
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
