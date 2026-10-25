// Copyright 2026 Shen-Ta Hsieh
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//
// See: https://github.com/boostorg/decimal/issues/1542

#include <boost/decimal.hpp>
#include <boost/core/lightweight_test.hpp>
#include <cerrno>
#include <limits>
#include <string>

using namespace boost::decimal;

// A directed rounding can give the largest finite value in place of infinity
template <typename T>
bool is_overflow(const T value, const bool negative)
{
    return signbit(value) == negative && (isinf(value) || abs(value) == std::numeric_limits<T>::max());
}

template <typename T>
void test()
{
    // A wide character that can not be converted ends the number
    const wchar_t* wide {L"1.5\u20ac"};
    wchar_t* wide_end {};
    BOOST_TEST_EQ(wcstod<T>(wide, &wide_end), T(15, -1));
    BOOST_TEST_EQ(wide_end, wide + 3);

    const wchar_t* wide_first {L"\u20ac5"};
    wide_end = nullptr;
    static_cast<void>(wcstod<T>(wide_first, &wide_end));
    BOOST_TEST_EQ(wide_end, wide_first);

    // Only the whitespace of isspace in the C locale is skipped
    char* end {};
    const char* spaces {" \t\n\v\f\r5"};
    BOOST_TEST_EQ(strtod<T>(spaces, &end), T{5});
    BOOST_TEST_EQ(end, spaces + 7);

    for (const char* s : {"\x80" "5", "\x01" "5", "\xc2\xa0" "5"})
    {
        static_cast<void>(strtod<T>(s, &end));
        BOOST_TEST_EQ(end, s);
    }

    // A finite number too large for the type sets ERANGE
    #ifndef BOOST_DECIMAL_FAST_MATH
    errno = 0;
    BOOST_TEST(is_overflow(strtod<T>("-1e99999", nullptr), true));
    BOOST_TEST_EQ(errno, ERANGE);

    errno = 0;
    BOOST_TEST(is_overflow(wcstod<T>(L"1e99999", nullptr), false));
    BOOST_TEST_EQ(errno, ERANGE);

    // A number can end at its point, as in "1000.", and still overflow
    const std::string point {"1" + std::string(7000U, '0') + "."};
    errno = 0;
    BOOST_TEST(is_overflow(strtod<T>(point.c_str(), nullptr), false));
    BOOST_TEST_EQ(errno, ERANGE);

    errno = 0;
    BOOST_TEST_EQ(strtod<T>("infinity", nullptr), std::numeric_limits<T>::infinity());
    BOOST_TEST_EQ(errno, 0);
    #endif

    errno = 0;
    BOOST_TEST_EQ(strtod<T>("1e5", nullptr), T(1, 5));
    BOOST_TEST_EQ(errno, 0);
}

// A directed rounding gives the largest finite value for an overflow, and ERANGE too.
// fesetround has an effect only with the detection of constant evaluation
#if !defined(BOOST_DECIMAL_FAST_MATH) && !defined(BOOST_DECIMAL_NO_CONSTEVAL_DETECTION)
template <typename T>
void test_directed_rounding()
{
    const auto max_exp {std::numeric_limits<T>::max_exponent10};
    const std::string over {"1e" + std::to_string(max_exp + 1)};
    const std::string under_limit {"9." + std::string(static_cast<std::size_t>(std::numeric_limits<T>::digits10) + 2U, '9') + "e" + std::to_string(max_exp)};
    const auto max {std::numeric_limits<T>::max()};

    for (const auto mode : {rounding_mode::fe_dec_toward_zero, rounding_mode::fe_dec_downward, rounding_mode::fe_dec_upward})
    {
        fesetround(mode);
        for (const std::string& s : {over, "-" + over, std::string("1e1000000"), std::string("-1e1000000")})
        {
            errno = 0;
            BOOST_TEST(is_overflow(strtod<T>(s.c_str(), nullptr), s[0] == '-'));
            BOOST_TEST_EQ(errno, ERANGE);
        }

        errno = 0;
        static_cast<void>(wcstod<T>(L"-1e1000000", nullptr));
        BOOST_TEST_EQ(errno, ERANGE);
    }

    // Toward zero, a value below 10^(emax + 1) gives the largest finite value without an overflow
    fesetround(rounding_mode::fe_dec_toward_zero);
    for (const std::string& s : {under_limit, "-" + under_limit})
    {
        errno = 0;
        BOOST_TEST_EQ(abs(strtod<T>(s.c_str(), nullptr)), max);
        BOOST_TEST_EQ(errno, 0);
    }

    fesetround(rounding_mode::fe_dec_to_nearest);
}
#else
template <typename T>
void test_directed_rounding() {}
#endif

int main()
{
    test<decimal32_t>();
    test<decimal64_t>();
    test<decimal128_t>();
    test<decimal_fast32_t>();
    test<decimal_fast64_t>();
    test<decimal_fast128_t>();

    test_directed_rounding<decimal32_t>();
    test_directed_rounding<decimal64_t>();
    test_directed_rounding<decimal128_t>();
    test_directed_rounding<decimal_fast32_t>();
    test_directed_rounding<decimal_fast64_t>();
    test_directed_rounding<decimal_fast128_t>();

    return boost::report_errors();
}
