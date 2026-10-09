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

// The expected values are sinh(x) and cosh(x) rounded once, from Python's decimal module
void test_nearest()
{
    // The Taylor series, for |x| < 1/16
    BOOST_TEST_EQ(sinh(0.05_DD), 0.05002083593765502_DD);
    BOOST_TEST_EQ(cosh(0.05_DD), 1.001250260438369_DD);
    BOOST_TEST_EQ(sinh(-0.0123_DF), -0.01230031_DF);
    BOOST_TEST_EQ(cosh(0.0123_DF), 1.000076_DF);
    BOOST_TEST_EQ(sinh("0.0624"_DL), "0.06244050298864175318903997310479085"_DL);
    BOOST_TEST_EQ(sinh("0.03"_DLF), "0.03000450020250433933995580093600190"_DLF);
    BOOST_TEST_EQ(sinh(3e-16_DL), "3.000000000000000000000000000000045e-16"_DL);
    BOOST_TEST_EQ(cosh(1e-16_DL), "1.000000000000000000000000000000005"_DL);
    BOOST_TEST_EQ(sinh(1e-20_DL), 1e-20_DL);
    BOOST_TEST_EQ(sinh(1e-30_DD), 1e-30_DD);
    BOOST_TEST_EQ(cosh(1e-30_DD), 1_DD);
    BOOST_TEST_EQ(sinh(-1e-200_DD), -1e-200_DD);
    BOOST_TEST_EQ(sinh(-3e-390_DD), -3e-390_DD);
    BOOST_TEST_EQ(cosh(3e-390_DD), 1_DD);

    // exp(|x|) and exp(-|x|)
    BOOST_TEST_EQ(sinh("0.0625"_DL), "0.06254069805219182172183988501029229"_DL);
    BOOST_TEST_EQ(cosh("0.0625"_DL), "1.001953760865667607841550709632597"_DL);
    BOOST_TEST_EQ(sinh(-0.25_DD), -0.2526123168081683_DD);
    BOOST_TEST_EQ(sinh(0.9_DD), 1.026516725708175_DD);
    BOOST_TEST_EQ(sinh(1_DD), 1.175201193643801_DD);
    BOOST_TEST_EQ(cosh(2.5_DDF), 6.132289479663686_DDF);
    BOOST_TEST_EQ(sinh(-0.5_DFF), -0.5210953_DFF);
    BOOST_TEST_EQ(sinh(10_DL), "11013.23287470339337723652455484636"_DL);
    BOOST_TEST_EQ(sinh(50_DL), "2592352764293536232043.726661466743"_DL);
    BOOST_TEST_EQ(cosh(-50_DL), "2592352764293536232043.726661466743"_DL);

    // exp(|x|) only
    BOOST_TEST_EQ(sinh(30_DF), 5.343237e12_DF);
    BOOST_TEST_EQ(sinh(100_DD), 1.344058570908068e43_DD);
    BOOST_TEST_EQ(cosh("14141.4"_DL), "1.701987597323254228514447325009893e6141"_DL);

    // exp(887) overflows, but its half does not
    BOOST_TEST_EQ(sinh(887_DD), 8.282767152491226e384_DD);
    BOOST_TEST_EQ(sinh(-887_DD), -8.282767152491226e384_DD);
    BOOST_TEST_EQ(cosh(-887_DD), 8.282767152491226e384_DD);

    // Next to a rounding boundary, where the first try in one word cannot round
    BOOST_TEST_EQ(sinh(0.258_DD), 0.2608717932586549_DD);
    BOOST_TEST_EQ(sinh(0.503_DD), 0.5244805333943190_DD);
    BOOST_TEST_EQ(cosh(0.003_DD), 1.000004500003375_DD);
    BOOST_TEST_EQ(sinh(5.83_DD), 170.1778704973843_DD);
    BOOST_TEST_EQ(cosh(3.45_DD), 15.76606897256300_DD);

    // Next to a rounding boundary, where the first try in two words cannot round
    BOOST_TEST_EQ(sinh(0.67_DL), "0.7212643714246985089290357657301327"_DL);
    BOOST_TEST_EQ(sinh(2.1_DL), "4.021856742157334081615540017203002"_DL);
    BOOST_TEST_EQ(cosh(2.5_DL), "6.132289479663686116619852312817563"_DL);

    #ifndef BOOST_DECIMAL_FAST_MATH
    BOOST_TEST_EQ(sinh(1000_DD), std::numeric_limits<decimal64_t>::infinity());
    BOOST_TEST_EQ(sinh(-1000_DD), -std::numeric_limits<decimal64_t>::infinity());
    BOOST_TEST_EQ(cosh(-1000_DD), std::numeric_limits<decimal64_t>::infinity());
    #endif
    BOOST_TEST_EQ(sinh(1.5e-15_DL), "1.500000000000000000000000000000563e-15"_DL);
}

void test_directed()
{
    #ifndef BOOST_DECIMAL_NO_CONSTEVAL_DETECTION
    // fesetround has an effect only with the detection of constant evaluation
    fesetround(rounding_mode::fe_dec_upward);
    BOOST_TEST_EQ(sinh(1e-30_DD), 1.000000000000001e-30_DD);
    BOOST_TEST_EQ(cosh(1e-30_DD), 1.000000000000001_DD);
    BOOST_TEST_EQ(sinh(3e-390_DD), 3.00000001e-390_DD);
    BOOST_TEST_EQ(cosh(3e-390_DD), 1.000000000000001_DD);
    BOOST_TEST_EQ(cosh(0.5_DF), 1.127626_DF);

    fesetround(rounding_mode::fe_dec_downward);
    BOOST_TEST_EQ(cosh(1e-14_DL), "1.000000000000000000000000000050000"_DL);
    BOOST_TEST_EQ(cosh(1e-30_DD), 1_DD);
    BOOST_TEST_EQ(sinh(-1e-30_DD), -1.000000000000001e-30_DD);
    BOOST_TEST_EQ(cosh(0.5_DF), 1.127625_DF);
    BOOST_TEST_EQ(sinh(1000_DD), (std::numeric_limits<decimal64_t>::max)());
    #ifndef BOOST_DECIMAL_FAST_MATH
    BOOST_TEST_EQ(sinh(-1000_DD), -std::numeric_limits<decimal64_t>::infinity());
    #endif

    fesetround(rounding_mode::fe_dec_toward_zero);
    BOOST_TEST_EQ(sinh(-1e-30_DD), -1e-30_DD);
    BOOST_TEST_EQ(cosh(-1000_DD), (std::numeric_limits<decimal64_t>::max)());

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
        BOOST_TEST(is_quiet_of(sinh(s), s));
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
