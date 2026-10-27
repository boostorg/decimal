// Copyright 2026 Shen-Ta Hsieh
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//
// See: https://github.com/boostorg/decimal/issues/1530

#include <boost/decimal.hpp>
#include <boost/core/lightweight_test.hpp>
#include <climits>

using namespace boost::decimal;
using namespace boost::decimal::literals;

template <typename T>
void check(const T value, const chars_format fmt, const int precision, const char* expected)
{
    char buffer[128] {};
    const auto r {to_chars(buffer, buffer + sizeof(buffer) - 1, value, fmt, precision)};
    BOOST_TEST(r.ec == std::errc());
    *r.ptr = '\0';
    BOOST_TEST_CSTR_EQ(buffer, expected);
}

// A dropped 5 with more digits after it is above the tie
void test_not_a_tie()
{
    check("9.30585e-20"_DF, chars_format::scientific, 2, "9.31e-20");
    check("2.5178205e-4"_DD, chars_format::scientific, 0, "3e-04");
    check("6.0369653291e4"_DD, chars_format::scientific, 5, "6.03697e+04");
    check("-8.01251e31"_DL, chars_format::scientific, 3, "-8.013e+31");
    check("0.059814"_DF, chars_format::fixed, 1, "0.1");
    check("-0.2505"_DF, chars_format::fixed, 1, "-0.3");
    check("5.15e-5"_DF, chars_format::fixed, 4, "0.0001");
    check("42.38996745999424606413895"_DL, chars_format::fixed, 7, "42.3899675");
    check("2.5"_DD, chars_format::fixed, 0, "2");
    check("3.5"_DD, chars_format::fixed, 0, "4");
}

// A carry gives one more digit
void test_carry()
{
    check("-9.8"_DF, chars_format::scientific, 0, "-1e+01");
    check("9.97e15"_DF, chars_format::scientific, 0, "1e+16");
    check("9.996"_DD, chars_format::scientific, 2, "1.00e+01");
    check("9.996"_DD, chars_format::fixed, 2, "10.00");
    check("0.0996"_DD, chars_format::fixed, 1, "0.1");
}

// The value can be far below the last digit, or far above it
void test_range()
{
    check("6.5155e-15"_DD, chars_format::fixed, 14, "0.00000000000001");
    check("5.25e-13"_DD, chars_format::fixed, 12, "0.000000000001");
    check("1e-30"_DD, chars_format::fixed, 3, "0.000");
    check("-1e-30"_DD, chars_format::fixed, 3, "-0.000");
    check("1.5e20"_DF, chars_format::fixed, 2, "150000000000000000000.00");
    check("123.456"_DD, chars_format::fixed, 0, "123");

    char small[8] {};
    BOOST_TEST(to_chars(small, small + sizeof(small), "1.5e20"_DF, chars_format::fixed, 2).ec == std::errc::value_too_large);

    // The length of a precision near INT_MAX does not fit in an int, and does not fit the buffer
    BOOST_TEST(to_chars(small, small + sizeof(small), "1e9"_DD, chars_format::fixed, INT_MAX).ec == std::errc::value_too_large);
    BOOST_TEST(to_chars(small, small + sizeof(small), "-1e9"_DD, chars_format::fixed, INT_MAX).ec == std::errc::value_too_large);
    BOOST_TEST(to_chars(small, small + sizeof(small), "1e-30"_DD, chars_format::fixed, INT_MAX).ec == std::errc::value_too_large);
}

// The rounding follows the current mode and the sign of the value
void test_modes()
{
    #ifndef BOOST_DECIMAL_NO_CONSTEVAL_DETECTION
    // fesetround has an effect only with the detection of constant evaluation
    fesetround(rounding_mode::fe_dec_downward);
    check("-1.21"_DD, chars_format::fixed, 1, "-1.3");
    check("1.29"_DD, chars_format::fixed, 1, "1.2");
    check("-1e-30"_DD, chars_format::fixed, 3, "-0.001");
    check("-1.21"_DD, chars_format::scientific, 1, "-1.3e+00");

    fesetround(rounding_mode::fe_dec_upward);
    check("1.21"_DD, chars_format::fixed, 1, "1.3");
    check("-1.29"_DD, chars_format::fixed, 1, "-1.2");
    check("1e-30"_DD, chars_format::scientific, 0, "1e-30");

    fesetround(rounding_mode::fe_dec_to_nearest);
    #endif
}

int main()
{
    test_not_a_tie();
    test_carry();
    test_range();
    test_modes();

    return boost::report_errors();
}
