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

// The expected values are asinh(x) and acosh(x) rounded once, from Python's decimal module
void test_nearest()
{
    BOOST_TEST_EQ(asinh(0.05_DF), 0.04997919_DF);
    BOOST_TEST_EQ(asinh(0.05_DD), 0.04997919006934867_DD);
    BOOST_TEST_EQ(asinh(0.05_DL), "0.04997919006934866523139496213635679"_DL);
    BOOST_TEST_EQ(asinh(0.01_DL), "0.009999833340832886935141612163953796"_DL);
    BOOST_TEST_EQ(asinh(4.191165E+11_DF), 27.45456_DF);
    BOOST_TEST_EQ(asinh(-1.649219E+28_DF), -65.66583_DF);
    BOOST_TEST_EQ(asinh(-9378078.399864494_DD), -16.74703261913369_DD);
    BOOST_TEST_EQ(asinh(8.297351029947686E+25_DD), 60.37371081625513_DD);
    BOOST_TEST_EQ(asinh("35012619006179450789.73395133063323"_DL), "45.69538739399585858131132029859318"_DL);
    BOOST_TEST_EQ(asinh("-443070033664453731931823226528.2695"_DL), "-68.95667253848549350747041240065701"_DL);
    BOOST_TEST_EQ(acosh(9467.783_DF), 9.848797_DF);
    BOOST_TEST_EQ(acosh(6.738715E+18_DF), 44.04755_DF);
    BOOST_TEST_EQ(acosh(29906394420589.39_DD), 31.72224061424068_DD);
    BOOST_TEST_EQ(acosh(5.159154742956694E+24_DD), 57.59596216898775_DD);
    // Where two words leave the result to three, and one word to two
    BOOST_TEST_EQ(asinh(0.27_DL), "0.2668226669099408582463541778057620"_DL);
    BOOST_TEST_EQ(acosh(2.1_DL), "1.372859144242579840478282154314263"_DL);
    BOOST_TEST_EQ(asinh(7.7_DD), 2.738557650137144_DD);
    BOOST_TEST_EQ(asinh(0.135_DD), 0.1345932644887743_DD);
    BOOST_TEST_EQ(acosh(6_DD), 2.477888730288475_DD);
    BOOST_TEST_EQ(acosh("8909332312812341718269423.171754400"_DL), "58.14228871425822319076121565681721"_DL);
    BOOST_TEST_EQ(acosh("867907139968817986660867621.5148525"_DL), "62.72127413975170780803134328544164"_DL);
    BOOST_TEST_EQ(asinh(-9378078.399864494_DDF), -16.74703261913369_DDF);
    BOOST_TEST_EQ(acosh(9467.783_DFF), 9.848797_DFF);
    BOOST_TEST_EQ(acosh("8909332312812341718269423.171754400"_DLF), "58.14228871425822319076121565681721"_DLF);

    // Arguments next to 0 and 1, and large arguments
    BOOST_TEST_EQ(asinh(1E-60_DD), 1E-60_DD);
    BOOST_TEST_EQ(asinh(-0.003_DF), -0.002999996_DF);
    BOOST_TEST_EQ(asinh(0.5_DD), 0.4812118250596034_DD);
    BOOST_TEST_EQ(asinh(0.8_DD), 0.7326682560454109_DD);
    BOOST_TEST_EQ(asinh(-0.3_DL), "-0.2956730475634224391027105297335171"_DL);
    BOOST_TEST_EQ(asinh(1.5_DF), 1.194763_DF);
    BOOST_TEST_EQ(asinh(9.99_DD), 2.997227420191335_DD);
    BOOST_TEST_EQ(asinh(1E+40_DL), "92.79655090032177267013689030883274"_DL);
    BOOST_TEST_EQ(asinh(1E+300_DD), 691.4686750787737_DD);
    BOOST_TEST_EQ(acosh(1.000001_DF), 0.001414213_DF);
    BOOST_TEST_EQ(acosh(1.000000000000001_DD), 4.472135954999579E-8_DD);
    BOOST_TEST_EQ(acosh("1.000000000000000000000000000000001"_DL), "4.472135954999579392818347337462552E-17"_DL);
    BOOST_TEST_EQ(acosh(1.00001_DD), 0.004472132228228002_DD);
    BOOST_TEST_EQ(acosh(1.5_DF), 0.9624237_DF);
    BOOST_TEST_EQ(acosh(9.99_DD), 2.992217300372479_DD);
    BOOST_TEST_EQ(acosh(1E+40_DL), "92.79655090032177267013689030883274"_DL);
    BOOST_TEST_EQ(acosh(1E+300_DD), 691.4686750787737_DD);
    // sqrt(2w) and its w/12 term exact, which puts the 3w^2/160 term on a rounding boundary
    BOOST_TEST_EQ(acosh("1.0000000000000000000000000001125"_DL), "1.499999999999999999999999999985938E-14"_DL);
    // A small x whose x^3/6 term ends on a rounding boundary: the next term decides past three words
    BOOST_TEST_EQ(asinh("1.500000000000000000000000001E-15"_DL), "1.500000000000000000000000000999437E-15"_DL);
}

void test_directed()
{
    #ifndef BOOST_DECIMAL_NO_CONSTEVAL_DETECTION
    // fesetround has an effect only with the detection of constant evaluation
    fesetround(rounding_mode::fe_dec_downward);
    BOOST_TEST_EQ(asinh("-28698.95823368667746074638802771270"_DL), "-10.95776328347457026052451328714561"_DL);
    BOOST_TEST_EQ(acosh(1.089222_DF), 0.4193469_DF);
    BOOST_TEST_EQ(asinh(1E-60_DD), 9.999999999999999E-61_DD);
    BOOST_TEST_EQ(asinh(3E-397_DD), 2.9E-397_DD);

    fesetround(rounding_mode::fe_dec_upward);
    BOOST_TEST_EQ(asinh(-7692146.360043812_DD), -16.54885759363653_DD);
    BOOST_TEST_EQ(acosh(1.000068364881966_DD), 0.01169308368401478_DD);
    BOOST_TEST_EQ(acosh("1.0000000000000000000000000000045"_DL), "2.999999999999999999999999999998876E-15"_DL);
    BOOST_TEST_EQ(acosh("245.6943774370837142730370043548513"_DL), "6.197231434522735188329513133423292"_DL);
    BOOST_TEST_EQ(asinh(3E-397_DD), 3E-397_DD);

    fesetround(rounding_mode::fe_dec_toward_zero);
    BOOST_TEST_EQ(asinh(8.580488E+28_DF), 67.31502_DF);

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
        BOOST_TEST(is_quiet_of(asinh(s), s));
        BOOST_TEST(is_quiet_of(acosh(s), s));
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
