// Copyright 2026 Shen-Ta Hsieh
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//
// See: https://github.com/boostorg/decimal/issues/1531

#include <boost/decimal.hpp>
#include <boost/core/lightweight_test.hpp>
#include <climits>
#include <cstring>
#include <iomanip>
#include <limits>
#include <sstream>
#include <string>

using namespace boost::decimal;
using namespace boost::decimal::literals;

// The expected strings are what printf("%.*g") gives for the same double
template <typename T>
void check(const T value, const int precision, const char* expected)
{
    char buffer[64];
    const auto r {to_chars(buffer, buffer + sizeof(buffer), value, chars_format::general, precision)};
    BOOST_TEST(r);
    *r.ptr = '\0';
    BOOST_TEST_CSTR_EQ(buffer, expected);
}

template <typename T>
void test()
{
    check(T{1.5_DL}, 1, "2");
    check(T{1.5_DL}, 0, "2");
    check(T{-1.5_DL}, 3, "-1.5");
    check(T{1234567.25_DL}, 6, "1.23457e+06");
    check(T{1234567.25_DL}, 7, "1234567");
    check(T{0.0001234_DL}, 3, "0.000123");
    check(T{0.00001234_DL}, 3, "1.23e-05");
    check(T{999.96_DL}, 3, "1e+03");
    check(T{999.96_DL}, 4, "1000");
    check(T{9.96_DL}, 2, "10");
    check(T{100_DL}, 3, "100");
    check(T{100_DL}, 2, "1e+02");
    check(T{0.5_DL}, 30, "0.5");
    check(T{0_DL}, 3, "0");
    check(-T{0_DL}, 3, "-0");
    check(std::numeric_limits<T>::infinity(), 3, "inf");

    // inf and nan do not use the precision, so a large one fits a small buffer
    char buffer[8];
    auto r {to_chars(buffer, buffer + sizeof(buffer), -std::numeric_limits<T>::infinity(), chars_format::fixed, 1000000)};
    BOOST_TEST(r);
    BOOST_TEST_EQ(std::string(buffer, r.ptr), "-inf");
    r = to_chars(buffer, buffer, -std::numeric_limits<T>::infinity(), chars_format::fixed, 3);
    BOOST_TEST(!r);

    // A negative zero needs room for its sign and its digit
    r = to_chars(buffer, buffer + 2, -T{0}, chars_format::fixed, 0);
    BOOST_TEST(r);
    BOOST_TEST_EQ(std::string(buffer, r.ptr), "-0");
    for (const auto fmt : {chars_format::fixed, chars_format::general})
    {
        buffer[1] = 'x';
        r = to_chars(buffer, buffer + 1, -T{0}, fmt, 0);
        BOOST_TEST(!r);
        BOOST_TEST_EQ(buffer[1], 'x');
    }
    BOOST_TEST(!to_chars(buffer, buffer + sizeof(buffer), T{0}, chars_format::scientific, INT_MAX));

    // A zero and a nan fit a buffer of their exact length
    const auto exact = [](const T v, const chars_format fmt, const int precision, const char* expected)
    {
        char text[16];
        const auto n {std::strlen(expected)};
        const auto fits {to_chars(text, text + n, v, fmt, precision)};
        BOOST_TEST(fits);
        BOOST_TEST_EQ(std::string(text, fits.ptr), expected);
        BOOST_TEST(!to_chars(text, text + n - 1, v, fmt, precision));
    };
    exact(T{0}, chars_format::fixed, 3, "0.000");
    exact(T{0}, chars_format::scientific, 0, "0e+00");
    exact(-T{0}, chars_format::scientific, 0, "-0e+00");

    // A shorter buffer gets the shorter -nan
    char text[9];
    r = to_chars(text, text + sizeof(text), -std::numeric_limits<T>::quiet_NaN(), chars_format::fixed, 2);
    BOOST_TEST(r);
    BOOST_TEST_EQ(std::string(text, r.ptr), "-nan(ind)");

    // The ostream default precision is 6, with the general format
    std::ostringstream out;
    out << T{1234567.25_DL} << ' ' << std::setprecision(3) << T{0.0001234_DL};
    BOOST_TEST_EQ(out.str(), "1.23457e+06 0.000123");
}

// The scientific format needs room for its exponent, and a buffer of the exact length is enough
void test_scientific_room()
{
    char tiny[4] {};
    BOOST_TEST(to_chars(tiny, tiny + sizeof(tiny), "1.5"_DD, chars_format::scientific, 2).ec == std::errc::value_too_large);
    char exact[8] {};
    BOOST_TEST(to_chars(exact, exact + 7, "1.5"_DD, chars_format::scientific, 2).ec == std::errc::value_too_large);
    auto r {to_chars(exact, exact + 8, "1.5"_DD, chars_format::scientific, 2)};
    BOOST_TEST(r && std::string(exact, r.ptr) == "1.50e+00");
    BOOST_TEST(to_chars(exact, exact + 1, "-1.5"_DD, chars_format::scientific, 0).ec == std::errc::value_too_large);
    r = to_chars(exact, exact + 8, "1.5e-100"_DD, chars_format::scientific, 1);
    BOOST_TEST(r && std::string(exact, r.ptr) == "1.5e-100");
    BOOST_TEST(to_chars(exact, exact + 7, "1.5e-100"_DD, chars_format::scientific, 1).ec == std::errc::value_too_large);
}

int main()
{
    test_scientific_room();
    test<decimal32_t>();
    test<decimal64_t>();
    test<decimal128_t>();
    test<decimal_fast32_t>();
    test<decimal_fast64_t>();
    test<decimal_fast128_t>();

    return boost::report_errors();
}
