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

// The expected values are exp(x) - 1 rounded once, from Python's decimal module
void test_nearest()
{
    // The Taylor series of sinh and cosh, for |x| < 1/16
    BOOST_TEST_EQ(expm1(0.05_DD), 0.05127109637602404_DD);
    BOOST_TEST_EQ(expm1(-0.05_DD), -0.04877057549928599_DD);
    BOOST_TEST_EQ(expm1(-0.0123_DF), -0.01222466_DF);
    BOOST_TEST_EQ(expm1("0.0624"_DL), "0.06438801479426252690188329926006567"_DL);
    BOOST_TEST_EQ(expm1("-0.0624"_DLF), "-0.06049299118302097947619664694951604"_DLF);
    BOOST_TEST_EQ(expm1(-0.01_DL), "-0.009950166250831946426094022819963442"_DL);
    BOOST_TEST_EQ(expm1(1e-40_DL), 1e-40_DL);
    BOOST_TEST_EQ(expm1(1e-30_DD), 1e-30_DD);
    BOOST_TEST_EQ(expm1(-1e-30_DD), -1e-30_DD);

    // exp(x) - 1
    BOOST_TEST_EQ(expm1("0.0625"_DL), "0.06449445891785942956339059464288967"_DL);
    BOOST_TEST_EQ(expm1(1_DD), 1.718281828459045_DD);
    BOOST_TEST_EQ(expm1(0.5_DFF), 0.6487213_DFF);
    BOOST_TEST_EQ(expm1(2.5_DDF), 11.18249396070347_DDF);
    BOOST_TEST_EQ(expm1(10_DL), "22025.46579480671651695790064528424"_DL);
    BOOST_TEST_EQ(expm1(100_DD), 2.688117141816135e43_DD);
    BOOST_TEST_EQ(expm1(100_DL), "2.688117141816135448412625551580014e43"_DL);

    // 1 - exp(x) for x < 0
    BOOST_TEST_EQ(expm1(-0.0625_DD), -0.06058693718652421_DD);
    BOOST_TEST_EQ(expm1(-1_DD), -0.6321205588285577_DD);
    BOOST_TEST_EQ(expm1(-3_DF), -0.9502129_DF);
    BOOST_TEST_EQ(expm1(-2.5_DD), -0.9179150013761012_DD);
    BOOST_TEST_EQ(expm1(-10_DL), "-0.9999546000702375151484644084844394"_DL);
    BOOST_TEST_EQ(expm1(-100_DL), -1_DL);
    BOOST_TEST_EQ(expm1(-100_DD), -1_DD);
    BOOST_TEST_EQ(expm1(-1000_DD), -1_DD);

    // Next to a rounding boundary and to -1, where the values of x next to each other give results that three words
    // of the difference do not tell apart
    BOOST_TEST_EQ(expm1("-67.29728408730881346680779243949991"_DL), "-0.9999999999999999999999999999940685"_DL);

    #ifndef BOOST_DECIMAL_FAST_MATH
    BOOST_TEST_EQ(expm1(1000_DD), std::numeric_limits<decimal64_t>::infinity());
    #endif
}

void test_directed()
{
    #ifndef BOOST_DECIMAL_NO_CONSTEVAL_DETECTION
    // fesetround has an effect only with the detection of constant evaluation
    fesetround(rounding_mode::fe_dec_toward_zero);
    BOOST_TEST_EQ(expm1(-1e-30_DD), -9.999999999999999e-31_DD);
    BOOST_TEST_EQ(expm1(-100_DD), -0.9999999999999999_DD);
    BOOST_TEST_EQ(expm1(-100_DL), "-0.9999999999999999999999999999999999"_DL);
    BOOST_TEST_EQ(expm1(-1000_DD), -0.9999999999999999_DD);

    fesetround(rounding_mode::fe_dec_downward);
    BOOST_TEST_EQ(expm1(1e-30_DL), "1.000000000000000000000000000000500e-30"_DL);
    BOOST_TEST_EQ(expm1(-1e-30_DD), -1e-30_DD);
    BOOST_TEST_EQ(expm1(-100_DD), -1_DD);
    BOOST_TEST_EQ(expm1(0.5_DF), 0.6487212_DF);

    fesetround(rounding_mode::fe_dec_upward);
    BOOST_TEST_EQ(expm1(1e-30_DD), 1.000000000000001e-30_DD);
    BOOST_TEST_EQ(expm1(5e-390_DD), 5.00000001e-390_DD);
    BOOST_TEST_EQ(expm1(-0.5_DF), -0.3934693_DF);
    BOOST_TEST_EQ(expm1("-63.41791797246554717594688333531244"_DL), "-0.9999999999999999999999999997129561"_DL);

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
        BOOST_TEST(is_quiet_of(expm1(s), s));
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
