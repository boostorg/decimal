// Copyright 2026 Shen-Ta Hsieh
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//
// See: https://github.com/boostorg/decimal/issues/1539

#include <boost/decimal.hpp>
#include <boost/core/lightweight_test.hpp>
#include <cstring>
#include <limits>
#include <memory>
#include <sstream>
#include <string>

using namespace boost::decimal;

// from_chars reads the same characters as std::from_chars for double
template <typename T>
void test_from_chars()
{
    const auto check = [](const char* s, const std::ptrdiff_t len, const T expected)
    {
        T value {};
        const auto r {from_chars(s, s + std::strlen(s), value)};
        BOOST_TEST(r);
        BOOST_TEST_EQ(r.ptr - s, len);
        BOOST_TEST_EQ(value, expected);
    };

    check("1.5e", 3, T{15, -1});
    check("1e+", 1, T{1});
    check("2E-x", 1, T{2});
    check("0e", 1, T{0});
    #ifndef BOOST_DECIMAL_FAST_MATH
    check("infinity", 8, std::numeric_limits<T>::infinity());
    check("-INFINITY", 9, -std::numeric_limits<T>::infinity());
    check("infinit", 3, std::numeric_limits<T>::infinity());
    #endif

    // An exact-size copy has no NUL after it, so a sanitizer finds a read past the end
    for (const char* s : {".", ".e5", "-.", "-", "-e5", "s", "-s", "sn", "s5", "-S1e3"})
    {
        const auto size {std::strlen(s)};
        std::unique_ptr<char[]> input {new char[size]};
        std::memcpy(input.get(), s, size);
        T value {};
        const auto r {from_chars(input.get(), input.get() + size, value)};
        BOOST_TEST(r.ec == std::errc::invalid_argument);
        BOOST_TEST_EQ(r.ptr, input.get());
    }

    // A second decimal point ends a number with more digits than the parser keeps,
    // so each number is 40 digits, the first point and one digit long
    for (const char* s : {"1234567890123456789012345678901234567890.1.2", "1.2345678901234567890123456789012345678901.5"})
    {
        T value {};
        T expected {};
        const auto r {from_chars(s, s + std::strlen(s), value)};
        from_chars(s, s + 42, expected);
        BOOST_TEST_EQ(r.ptr - s, 42);
        BOOST_TEST_EQ(value, expected);
    }

    T value {};
    BOOST_TEST(!from_chars("1.5e", value, chars_format::scientific));
}

// strtod keeps the sign of an infinity or a NaN, and a NaN payload makes a quiet NaN
template <typename T>
void test_strtod()
{
    char* end {};

    #ifndef BOOST_DECIMAL_FAST_MATH
    const char* s {"-infinity"};

    BOOST_TEST_EQ(strtod<T>(s, &end), -std::numeric_limits<T>::infinity());
    BOOST_TEST_EQ(end, s + 9);

    const auto n {strtod<T>("-nan(123)", nullptr)};
    BOOST_TEST(isnan(n) && !issignaling(n) && signbit(n));
    BOOST_TEST(issignaling(strtod<T>("snan", nullptr)));
    BOOST_TEST(!signbit(strtod<T>("+inf", nullptr)));
    #endif

    // Without a conversion, endptr is the start of the string
    for (const char* bad : {"   xyz", "+-5", ".e5"})
    {
        static_cast<void>(strtod<T>(bad, &end));
        BOOST_TEST_EQ(end, bad);
    }
}

// operator>> sets failbit when it can not read a value, and reads a long token in full
template <typename T>
void test_stream()
{
    for (const char* bad : {"abc", "+-5", "."})
    {
        std::istringstream is {bad};
        T value {};
        is >> value;
        BOOST_TEST(is.fail());
    }

    for (const std::size_t len : {std::size_t{1023}, std::size_t{1024}, std::size_t{1500}})
    {
        std::string text(len, '0');
        text[0] = '1';
        std::istringstream is {text};
        T value {};
        is >> value;
        BOOST_TEST(!is.fail());
        BOOST_TEST_EQ(value, T(1, static_cast<int>(len - 1U)));
    }
}

template <typename T>
void test()
{
    test_from_chars<T>();
    test_strtod<T>();
    test_stream<T>();
}

int main()
{
    test<decimal32_t>();
    test<decimal64_t>();
    test<decimal128_t>();
    test<decimal_fast32_t>();
    test<decimal_fast64_t>();
    test<decimal_fast128_t>();

    return boost::report_errors();
}
