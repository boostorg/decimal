// Copyright 2026 Shen-Ta Hsieh
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//
// See: https://github.com/boostorg/decimal/issues/1541

#include <boost/decimal.hpp>
#include <boost/core/lightweight_test.hpp>
#include <cstring>

using namespace boost::decimal;

// from_chars reads the same characters as std::from_chars for double
template <typename T>
void check(const char* s, const chars_format fmt, const std::ptrdiff_t len, const T expected)
{
    T value {};
    const auto r {from_chars(s, s + std::strlen(s), value, fmt)};
    BOOST_TEST(r);
    BOOST_TEST_EQ(r.ptr - s, len);
    BOOST_TEST_EQ(value, expected);
    BOOST_TEST_EQ(signbit(value), signbit(expected));
}

template <typename T>
void test()
{
    // Zeros before a character that ends the number
    check("0,", chars_format::general, 1, T{0});
    check("-0.00 ", chars_format::general, 5, -T{0});
    check("0x10", chars_format::general, 1, T{0});
    check("000;", chars_format::fixed, 3, T{0});
    check("0.0z", chars_format::hex, 3, T{0});
    check("0e5,", chars_format::scientific, 3, T{0});

    // The fixed format has no exponent, so the number ends before the "e"
    check("1.5e5", chars_format::fixed, 3, T{15, -1});
    check("0E5", chars_format::fixed, 1, T{0});

    // The scientific format needs an exponent, also for a zero
    for (const char* s : {"0", "-0.0", "00,", "1"})
    {
        T value {};
        const auto r {from_chars(s, s + std::strlen(s), value, chars_format::scientific)};
        BOOST_TEST(r.ec == std::errc::invalid_argument);
        BOOST_TEST_EQ(r.ptr, s);
    }
}

// The fractional zeros give the cohort of a zero
template <typename T>
void test_cohort()
{
    T value {};
    const char* s {"0.000,"};
    BOOST_TEST(from_chars(s, s + std::strlen(s), value, chars_format::cohort_preserving_fixed));

    char buffer[32] {};
    *to_chars(buffer, buffer + sizeof(buffer), value, chars_format::cohort_preserving_fixed).ptr = '\0';
    BOOST_TEST_CSTR_EQ(buffer, "0.000");
}

int main()
{
    test<decimal32_t>();
    test<decimal64_t>();
    test<decimal128_t>();
    test<decimal_fast32_t>();
    test<decimal_fast64_t>();
    test<decimal_fast128_t>();

    test_cohort<decimal32_t>();
    test_cohort<decimal64_t>();
    test_cohort<decimal128_t>();

    return boost::report_errors();
}
