// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//
// See: https://github.com/boostorg/decimal/issues/1537

#define FMT_HEADER_ONLY
#include <boost/decimal.hpp>
#include <boost/decimal/fmt_format.hpp>
#include <boost/core/lightweight_test.hpp>
#include <cstddef>
#include <string>

// The wide formatters of {fmt} need C++17
#if defined(BOOST_DECIMAL_HAS_FMTLIB_SUPPORT) && !defined(BOOST_DECIMAL_NO_CXX17_IF_CONSTEXPR)

#include <fmt/compile.h>
#include <fmt/xchar.h>

using namespace boost::decimal;
using namespace boost::decimal::literals;

// The function from the issue: {fmt} formats FMT_COMPILE(L"...") with format_to_n in a char context
template <typename T>
auto decimal_to_string(const T x, wchar_t* s, const std::size_t size) -> std::size_t
{
    std::size_t count {};
    if (s != nullptr && size > 0)
    {
        const auto r {fmt::format_to_n(s, size - 1, FMT_COMPILE(L"{}"), x)};
        *r.out = L'\0';
        count = r.size;
    }

    return count;
}

template <typename T>
void test()
{
    const T x {3.14_DD};

    wchar_t buffer[64] {};
    BOOST_TEST_EQ(decimal_to_string(x, buffer, 64U), 4U);
    BOOST_TEST(std::wstring(buffer) == L"3.14");

    // A short buffer takes what fits and reports the full length
    wchar_t short_buffer[3] {};
    BOOST_TEST_EQ(decimal_to_string(-x, short_buffer, 3U), 5U);
    BOOST_TEST(std::wstring(short_buffer) == L"-3");

    // The fill, the zeros of the 0 flag and the locale path of the char context
    wchar_t spec_buffer[64] {};
    const auto r {fmt::format_to_n(spec_buffer, 63U, FMT_COMPILE(L"{:*^10.2f}|{:+08.2f}|{:.2Lf}"), x, x, -x)};
    *r.out = L'\0';
    BOOST_TEST(std::wstring(spec_buffer) == L"***3.14***|+0003.14|-3.14");

    char narrow_buffer[64] {};
    const auto narrow_r {fmt::format_to_n(narrow_buffer, 63U, FMT_COMPILE("{:*^10.2f}|{:+08.2f}"), x, x)};
    *narrow_r.out = '\0';
    BOOST_TEST_EQ(std::string(narrow_buffer), "***3.14***|+0003.14");

    BOOST_TEST(fmt::format(FMT_COMPILE(L"{}"), x) == L"3.14");
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

#else

int main()
{
    return 0;
}

#endif
