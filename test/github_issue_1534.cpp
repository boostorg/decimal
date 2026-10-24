// Copyright 2026 Shen-Ta Hsieh
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//
// See: https://github.com/boostorg/decimal/issues/1534

#define FMT_HEADER_ONLY
#include <boost/decimal.hpp>
#include <boost/decimal/fmt_format.hpp>
#include <boost/core/lightweight_test.hpp>
#include <iostream>
#include <limits>
#include <locale>
#include <string>

#if defined(BOOST_DECIMAL_HAS_FMTLIB_SUPPORT)
#include <fmt/format.h>
#endif

using namespace boost::decimal;
using namespace boost::decimal::literals;

// A numpunct with the Indian grouping and a separator that has no char form, so that the test
// needs no installed locale
template <typename charT>
struct indian_numpunct : std::numpunct<charT>
{
    std::string do_grouping() const override { return "\3\2"; }
    charT do_decimal_point() const override { return static_cast<charT>(','); }
    charT do_thousands_sep() const override { return static_cast<charT>(sizeof(charT) == 1 ? '.' : 0x202F); }
};

// The expected strings are what std::format gives for the same double
template <typename T, typename Format>
void test_zero_flag(Format format)
{
    const T x {3.14_DD};
    BOOST_TEST_EQ(format("{:08.2f}", x), "00003.14");
    BOOST_TEST_EQ(format("{:08.2f}", -x), "-0003.14");
    BOOST_TEST_EQ(format("{:+08.2f}", x), "+0003.14");
    BOOST_TEST_EQ(format("{: 08.2f}", x), " 0003.14");
    BOOST_TEST_EQ(format("{:08.1e}", x), "03.1e+00");
    BOOST_TEST_EQ(format("{:<08.2f}", x), "3.14    ");
    BOOST_TEST_EQ(format("{:08}", -std::numeric_limits<T>::infinity()), "    -inf");
}

// The output is longer than the old 128 character buffer
template <typename T, typename Format>
void test_long_output(Format format)
{
    BOOST_TEST_EQ(format("{:.200f}", T {1}), "1." + std::string(200, '0'));
    BOOST_TEST_EQ(format("{:+.150e}", T {1}), "+1." + std::string(150, '0') + "e+00");

    const auto max {std::numeric_limits<T>::max()};
    std::string expected(8000, '\0');
    const auto r {to_chars(&expected[0], &expected[0] + expected.size(), max, chars_format::fixed, 2)};
    expected.resize(static_cast<std::size_t>(r.ptr - expected.data()));
    BOOST_TEST_EQ(format("{:.2f}", max), expected);
}

template <typename T, typename Format>
void test_locale(Format format)
{
    const T x {1234567};
    BOOST_TEST_EQ(format("{:13.1Lf}", x), "  1,234,567.0");
    BOOST_TEST_EQ(format("{:013.1Lf}", x), "001,234,567.0");
    BOOST_TEST_EQ(format("{:13.1fL}", x), "  1,234,567.0");
    BOOST_TEST_EQ(format("{:+L}", x), "+1,234,567");
    BOOST_TEST_EQ(format("{:.0Le}", x), "1e+06");
    BOOST_TEST_EQ(format("{: L}", std::numeric_limits<T>::infinity()), " inf");
}

// L uses the locale that the caller gives
template <typename T>
void test_explicit_locale()
{
    #if defined(BOOST_DECIMAL_HAS_FORMAT_SUPPORT) || defined(BOOST_DECIMAL_HAS_FMTLIB_SUPPORT)
    const std::locale loc {std::locale::classic(), new indian_numpunct<char>};
    const T x {1234567};
    #endif

    #if defined(BOOST_DECIMAL_HAS_FORMAT_SUPPORT) || (defined(BOOST_DECIMAL_HAS_FMTLIB_SUPPORT) && !defined(BOOST_DECIMAL_NO_CXX17_IF_CONSTEXPR))
    const std::locale wide_loc {std::locale::classic(), new indian_numpunct<wchar_t>};
    #endif

    #if defined(BOOST_DECIMAL_HAS_FORMAT_SUPPORT)
    BOOST_TEST_EQ(std::format(loc, "{:+.1Lf}", x), "+12.34.567,0");
    BOOST_TEST_EQ(std::format(loc, "{:.1f}", x), "1234567.0");
    BOOST_TEST(std::format(wide_loc, L"{:L}", x) == L"12\u202F34\u202F567");
    BOOST_TEST(std::format(L"{:\u2022>6}", T {15, -1}) == L"\u2022\u2022\u20221.5");
    #endif

    #if defined(BOOST_DECIMAL_HAS_FMTLIB_SUPPORT)
    BOOST_TEST_EQ(fmt::format(loc, "{:+.1Lf}", x), "+12.34.567,0");
    #if !defined(BOOST_DECIMAL_NO_CXX17_IF_CONSTEXPR)
    BOOST_TEST(fmt::format(wide_loc, L"{:L}", x) == L"12\u202F34\u202F567");
    BOOST_TEST(fmt::format(L"{:\u2022>6}", T {15, -1}) == L"\u2022\u2022\u20221.5");
    BOOST_TEST(fmt::format(u"{:\u2022>6}", T {15, -1}) == u"\u2022\u2022\u20221.5");
    BOOST_TEST(fmt::format(u"{:L}", x) == u"1234567");
    #endif
    #endif
}

// A bad specification throws the format_error of the library: a or A for a fast type, which keeps no
// cohort, an unknown type, and a width or a precision above INT_MAX. Without exceptions, the library
// rejects the format string as it does for a double
template <typename T>
void test_errors()
{
    #if !defined(BOOST_DECIMAL_DISABLE_EXCEPTIONS) && (defined(BOOST_DECIMAL_HAS_FORMAT_SUPPORT) || defined(BOOST_DECIMAL_HAS_FMTLIB_SUPPORT))
    const T one {1};
    const char* const bad_specs[] {"{:q}", "{:.2a}", "{:5.2fx}", "{:99999999999}", "{:.3000000000f}"};

    #if defined(BOOST_DECIMAL_HAS_FORMAT_SUPPORT)
    for (const auto spec : bad_specs)
    {
        BOOST_TEST_THROWS(static_cast<void>(std::vformat(spec, std::make_format_args(one))), std::format_error);
    }
    BOOST_DECIMAL_IF_CONSTEXPR (detail::is_fast_type_v<T>)
    {
        BOOST_TEST_THROWS(static_cast<void>(std::vformat("{:a}", std::make_format_args(one))), std::format_error);
        BOOST_TEST_THROWS(static_cast<void>(std::vformat("{:>40A}", std::make_format_args(one))), std::format_error);
    }
    #endif

    #if defined(BOOST_DECIMAL_HAS_FMTLIB_SUPPORT)
    for (const auto spec : bad_specs)
    {
        BOOST_TEST_THROWS(static_cast<void>(fmt::format(fmt::runtime(spec), one)), fmt::format_error);
    }
    BOOST_DECIMAL_IF_CONSTEXPR (detail::is_fast_type_v<T>)
    {
        BOOST_TEST_THROWS(static_cast<void>(fmt::format(fmt::runtime("{:a}"), one)), fmt::format_error);
        BOOST_TEST_THROWS(static_cast<void>(fmt::format(fmt::runtime("{:>40LA}"), one)), fmt::format_error);
    }
    #endif
    #endif
}

template <typename T>
void test()
{
    test_explicit_locale<T>();
    test_errors<T>();

    #if defined(BOOST_DECIMAL_HAS_FORMAT_SUPPORT)
    const auto std_format = [](const char* spec, const T v) { return std::vformat(spec, std::make_format_args(v)); };
    test_zero_flag<T>(std_format);
    test_long_output<T>(std_format);
    #endif

    #if defined(BOOST_DECIMAL_HAS_FMTLIB_SUPPORT)
    const auto fmt_format = [](const char* spec, const T v) { return fmt::format(fmt::runtime(spec), v); };
    test_zero_flag<T>(fmt_format);
    test_long_output<T>(fmt_format);
    #endif

    // An installed locale is needed, and a missing one throws
    #if !defined(BOOST_DECIMAL_DISABLE_EXCEPTIONS) && !defined(BOOST_DECIMAL_QEMU_TEST)
    try
    {
        std::locale::global(std::locale("en_US.UTF-8"));
    }
    catch (...)
    {
        std::cerr << "Test not run" << std::endl;
        return;
    }

    #if defined(BOOST_DECIMAL_HAS_FORMAT_SUPPORT)
    test_locale<T>(std_format);
    #endif

    #if defined(BOOST_DECIMAL_HAS_FMTLIB_SUPPORT)
    test_locale<T>(fmt_format);
    #endif

    std::locale::global(std::locale::classic());
    #endif
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
