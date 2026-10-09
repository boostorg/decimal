// Copyright 2026 Shen-Ta Hsieh
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//
// See: https://github.com/boostorg/decimal/issues/1557

#include <boost/decimal.hpp>
#include <boost/core/lightweight_test.hpp>
#include "quiet_nan.hpp"
#include <limits>

using namespace boost::decimal;
using namespace boost::decimal::literals;

// The expected values are exp(x) rounded once, from Python's decimal module
void test_nearest()
{
    BOOST_TEST_EQ(exp(-886.6121476392893_DD), 8.896858673609e-386_DD);
    BOOST_TEST_EQ(exp(69.11742129600862_DD), 1.040673923013077e30_DD);
    BOOST_TEST_EQ(exp(-69.11742129600862_DD), 9.609157853256158e-31_DD);
    BOOST_TEST_EQ(exp(2.384187630914485_DD), 10.85024468032785_DD);
    BOOST_TEST_EQ(exp(-914.2_DD), 9e-398_DD);
    BOOST_TEST_EQ(exp(1_DD), 2.718281828459045_DD);
    BOOST_TEST_EQ(exp(1e-30_DD), 1_DD);
    BOOST_TEST_EQ(exp(-1e-390_DD), 1_DD);
    BOOST_TEST_EQ(exp(-225.8488_DF), 8.22e-99_DF);
    BOOST_TEST_EQ(exp(5.884449_DF), 359.4047_DF);
    BOOST_TEST_EQ(exp(-1_DF), 0.3678794_DF);
    BOOST_TEST_EQ(exp("912.4682777163705"_DL), "1.905188379808810662536163793455628e396"_DL);
    BOOST_TEST_EQ(exp("0.6931471805599453094172321214581766"_DL), 2_DL);
    BOOST_TEST_EQ(exp(-14000_DL), "7.537951771629720907593006284141136e-6081"_DL);
    BOOST_TEST_EQ(exp(14100.5_DL), "5.879522772387939140565400738265113e6123"_DL);
    BOOST_TEST_EQ(exp(-886.6121476392893_DDF), 8.896858673609e-386_DDF);
    BOOST_TEST_EQ(exp(69.11742129600862_DDF), 1.040673923013077e30_DDF);
    BOOST_TEST_EQ(exp(5.884449_DFF), 359.4047_DFF);
    BOOST_TEST_EQ(exp(14100.5_DLF), "5.879522772387939140565400738265113e6123"_DLF);

    #ifndef BOOST_DECIMAL_FAST_MATH
    BOOST_TEST_EQ(exp(1000_DD), std::numeric_limits<decimal64_t>::infinity());
    BOOST_TEST_EQ(exp(-1000_DD), 0_DD);
    #endif
}

void test_directed()
{
    #ifndef BOOST_DECIMAL_NO_CONSTEVAL_DETECTION
    // fesetround has an effect only with the detection of constant evaluation
    fesetround(rounding_mode::fe_dec_downward);
    BOOST_TEST_EQ(exp(-1e-20_DF), 0.9999999_DF);
    BOOST_TEST_EQ(exp(-1e-390_DD), 0.9999999999999999_DD);
    BOOST_TEST_EQ(exp(1000_DD), (std::numeric_limits<decimal64_t>::max)());
    BOOST_TEST_EQ(exp(-230_DF), 1.2e-100_DF);

    fesetround(rounding_mode::fe_dec_upward);
    BOOST_TEST_EQ(exp(-1e-20_DF), 1_DF);
    BOOST_TEST_EQ(exp(1e-20_DF), 1.000001_DF);
    BOOST_TEST_EQ(exp(1e-390_DD), 1.000000000000001_DD);
    BOOST_TEST_EQ(exp(-1000_DD), std::numeric_limits<decimal64_t>::denorm_min());
    BOOST_TEST_EQ(exp(-230_DF), 1.3e-100_DF);

    fesetround(rounding_mode::fe_dec_toward_zero);
    BOOST_TEST_EQ(exp(1000_DD), (std::numeric_limits<decimal64_t>::max)());

    fesetround(rounding_mode::fe_dec_to_nearest_from_zero);
    BOOST_TEST_EQ(exp(-912.4682777163705_DD), 5.2e-397_DD);

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
        BOOST_TEST(is_quiet_of(exp(s), s));
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
