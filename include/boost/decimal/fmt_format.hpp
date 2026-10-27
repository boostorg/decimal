// Copyright 2023 - 2025 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_DECIMAL_FMT_FORMAT_HPP
#define BOOST_DECIMAL_FMT_FORMAT_HPP

#if __has_include(<fmt/format.h>) && __has_include(<fmt/base.h>)

#define BOOST_DECIMAL_HAS_FMTLIB_SUPPORT

#ifdef __GNUC__
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wfloat-equal"
#  pragma GCC diagnostic ignored "-Wconversion"
#endif

#include <fmt/format.h>
#include <fmt/base.h>
#include <fmt/xchar.h>

#ifdef __GNUC__
#  pragma GCC diagnostic pop
#endif

#include <boost/decimal/detail/config.hpp>
#include <boost/decimal/detail/io.hpp>
#include <boost/decimal/detail/format_render.hpp>
#include <boost/decimal/charconv.hpp>

#ifndef BOOST_DECIMAL_BUILD_MODULE

#include <locale>
#include <sstream>
#include <string>
#include <type_traits>

#endif // BOOST_DECIMAL_BUILD_MODULE

namespace boost {
namespace decimal {
namespace detail {
namespace fmt_detail {

// operator<< writes the number with the locale, but std::locale has the facets only for char and
// wchar_t, so another character type takes the char result
template <typename charT>
auto localize(char* first, char* last, const std::locale& loc, std::true_type) -> std::basic_string<charT>
{
    std::basic_ostringstream<charT> os;
    os.imbue(loc);
    put_chars(os, first, last);
    return os.str();
}

template <typename charT>
auto localize(char* first, char* last, const std::locale& loc, std::false_type) -> std::basic_string<charT>
{
    const auto s {localize<char>(first, last, loc, std::true_type {})};
    return std::basic_string<charT>(s.begin(), s.end());
}

template <typename T, typename CharType = char>
struct formatter
{
    format_spec<CharType> spec;

    template <typename ParseCharType>
    constexpr auto parse(fmt::parse_context<ParseCharType> &ctx)
    {
        return parse_format_spec<fmt::format_error>(ctx.begin(), ctx.end(), spec, !is_fast_type_v<T>);
    }

    template <typename FormatContext>
    auto format(const T& v, FormatContext& ctx) const
    {
        using OutputCharType = typename FormatContext::char_type;
        const auto localize = [&ctx](char* first, char* last)
        {
            return fmt_detail::localize<OutputCharType>(first, last, ctx.locale().template get<std::locale>(),
                                                        std::integral_constant<bool, std::is_same<OutputCharType, char>::value || std::is_same<OutputCharType, wchar_t>::value> {});
        };

        return format_decimal<fmt::format_error>(v, spec, ctx.out(), localize);
    }
};

} // namespace fmt_detail
} // namespace detail
} // namespace decimal
} // Namespace boost

namespace fmt {

#ifdef BOOST_DECIMAL_NO_CXX17_IF_CONSTEXPR

template <>
struct formatter<boost::decimal::decimal32_t>
    : public boost::decimal::detail::fmt_detail::formatter<boost::decimal::decimal32_t> {};

template <>
struct formatter<boost::decimal::decimal_fast32_t>
    : public boost::decimal::detail::fmt_detail::formatter<boost::decimal::decimal_fast32_t> {};

template <>
struct formatter<boost::decimal::decimal64_t>
    : public boost::decimal::detail::fmt_detail::formatter<boost::decimal::decimal64_t> {};

template <>
struct formatter<boost::decimal::decimal_fast64_t>
    : public boost::decimal::detail::fmt_detail::formatter<boost::decimal::decimal_fast64_t> {};

template <>
struct formatter<boost::decimal::decimal128_t>
    : public boost::decimal::detail::fmt_detail::formatter<boost::decimal::decimal128_t> {};

template <>
struct formatter<boost::decimal::decimal_fast128_t>
    : public boost::decimal::detail::fmt_detail::formatter<boost::decimal::decimal_fast128_t> {};

#else

template <>
struct formatter<boost::decimal::decimal32_t, char>
    : public boost::decimal::detail::fmt_detail::formatter<boost::decimal::decimal32_t, char> {};

template <>
struct formatter<boost::decimal::decimal32_t, wchar_t>
    : public boost::decimal::detail::fmt_detail::formatter<boost::decimal::decimal32_t, wchar_t> {};

template <>
struct formatter<boost::decimal::decimal32_t, char16_t>
    : public boost::decimal::detail::fmt_detail::formatter<boost::decimal::decimal32_t, char16_t> {};

template <>
struct formatter<boost::decimal::decimal32_t, char32_t>
    : public boost::decimal::detail::fmt_detail::formatter<boost::decimal::decimal32_t, char32_t> {};

template <>
struct formatter<boost::decimal::decimal_fast32_t, char>
    : public boost::decimal::detail::fmt_detail::formatter<boost::decimal::decimal_fast32_t, char> {};

template <>
struct formatter<boost::decimal::decimal_fast32_t, wchar_t>
    : public boost::decimal::detail::fmt_detail::formatter<boost::decimal::decimal_fast32_t, wchar_t> {};

template <>
struct formatter<boost::decimal::decimal_fast32_t, char16_t>
    : public boost::decimal::detail::fmt_detail::formatter<boost::decimal::decimal_fast32_t, char16_t> {};

template <>
struct formatter<boost::decimal::decimal_fast32_t, char32_t>
    : public boost::decimal::detail::fmt_detail::formatter<boost::decimal::decimal_fast32_t, char32_t> {};

template <>
struct formatter<boost::decimal::decimal64_t, char>
    : public boost::decimal::detail::fmt_detail::formatter<boost::decimal::decimal64_t, char> {};

template <>
struct formatter<boost::decimal::decimal64_t, wchar_t>
    : public boost::decimal::detail::fmt_detail::formatter<boost::decimal::decimal64_t, wchar_t> {};

template <>
struct formatter<boost::decimal::decimal64_t, char16_t>
    : public boost::decimal::detail::fmt_detail::formatter<boost::decimal::decimal64_t, char16_t> {};

template <>
struct formatter<boost::decimal::decimal64_t, char32_t>
    : public boost::decimal::detail::fmt_detail::formatter<boost::decimal::decimal64_t, char32_t> {};

template <>
struct formatter<boost::decimal::decimal_fast64_t, char>
    : public boost::decimal::detail::fmt_detail::formatter<boost::decimal::decimal_fast64_t, char> {};

template <>
struct formatter<boost::decimal::decimal_fast64_t, wchar_t>
    : public boost::decimal::detail::fmt_detail::formatter<boost::decimal::decimal_fast64_t, wchar_t> {};

template <>
struct formatter<boost::decimal::decimal_fast64_t, char16_t>
    : public boost::decimal::detail::fmt_detail::formatter<boost::decimal::decimal_fast64_t, char16_t> {};

template <>
struct formatter<boost::decimal::decimal_fast64_t, char32_t>
    : public boost::decimal::detail::fmt_detail::formatter<boost::decimal::decimal_fast64_t, char32_t> {};

template <>
struct formatter<boost::decimal::decimal128_t, char>
    : public boost::decimal::detail::fmt_detail::formatter<boost::decimal::decimal128_t, char> {};

template <>
struct formatter<boost::decimal::decimal128_t, wchar_t>
    : public boost::decimal::detail::fmt_detail::formatter<boost::decimal::decimal128_t, wchar_t> {};

template <>
struct formatter<boost::decimal::decimal128_t, char16_t>
    : public boost::decimal::detail::fmt_detail::formatter<boost::decimal::decimal128_t, char16_t> {};

template <>
struct formatter<boost::decimal::decimal128_t, char32_t>
    : public boost::decimal::detail::fmt_detail::formatter<boost::decimal::decimal128_t, char32_t> {};

template <>
struct formatter<boost::decimal::decimal_fast128_t, char>
    : public boost::decimal::detail::fmt_detail::formatter<boost::decimal::decimal_fast128_t, char> {};

template <>
struct formatter<boost::decimal::decimal_fast128_t, wchar_t>
    : public boost::decimal::detail::fmt_detail::formatter<boost::decimal::decimal_fast128_t, wchar_t> {};

template <>
struct formatter<boost::decimal::decimal_fast128_t, char16_t>
    : public boost::decimal::detail::fmt_detail::formatter<boost::decimal::decimal_fast128_t, char16_t> {};

template <>
struct formatter<boost::decimal::decimal_fast128_t, char32_t>
    : public boost::decimal::detail::fmt_detail::formatter<boost::decimal::decimal_fast128_t, char32_t> {};

#ifdef BOOST_DECIMAL_HAS_CHAR8_T

template <>
struct formatter<boost::decimal::decimal32_t, char8_t>
    : public boost::decimal::detail::fmt_detail::formatter<boost::decimal::decimal32_t, char8_t> {};

template <>
struct formatter<boost::decimal::decimal_fast32_t, char8_t>
    : public boost::decimal::detail::fmt_detail::formatter<boost::decimal::decimal_fast32_t, char8_t> {};

template <>
struct formatter<boost::decimal::decimal64_t, char8_t>
    : public boost::decimal::detail::fmt_detail::formatter<boost::decimal::decimal64_t, char8_t> {};

template <>
struct formatter<boost::decimal::decimal_fast64_t, char8_t>
    : public boost::decimal::detail::fmt_detail::formatter<boost::decimal::decimal_fast64_t, char8_t> {};

template <>
struct formatter<boost::decimal::decimal128_t, char8_t>
    : public boost::decimal::detail::fmt_detail::formatter<boost::decimal::decimal128_t, char8_t> {};

template <>
struct formatter<boost::decimal::decimal_fast128_t, char8_t>
    : public boost::decimal::detail::fmt_detail::formatter<boost::decimal::decimal_fast128_t, char8_t> {};

#endif // BOOST_DECIMAL_HAS_CHAR8_T

#endif

} // namespace fmt

#endif // __has_include(<fmt/format.h>)

#endif // BOOST_DECIMAL_FMT_FORMAT_HPP
