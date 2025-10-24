// Copyright 2023 - 2025 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_DECIMAL_FMT_FORMAT_HPP
#define BOOST_DECIMAL_FMT_FORMAT_HPP

#if __has_include(<fmt/format.h>) && __has_include(<fmt/base.h>)

#define BOOST_DECIMAL_HAS_FMTLIB_SUPPORT

#include <fmt/format.h>
#include <fmt/base.h>
#include <boost/decimal/detail/config.hpp>
#include <boost/decimal/charconv.hpp>
#include <algorithm>
#include <format>
#include <iostream>
#include <string>
#include <tuple>
#include <cctype>

namespace boost {
namespace decimal {
namespace detail {
namespace fmt_detail {

enum class sign_option
{
    plus,
    minus,
    space
};

template <typename ParseContext>
constexpr auto parse_impl(ParseContext &ctx)
{
    auto sign_character = sign_option::minus;
    int ctx_precision = -1;
    boost::decimal::chars_format fmt = boost::decimal::chars_format::general;
    bool is_upper = false;
    int padding_digits = 0;
    auto it {ctx.begin()};

    if (it == nullptr)
    {
        return std::make_tuple(ctx_precision, fmt, is_upper, padding_digits, sign_character, it);
    }

    // Check for a sign character
    if (it != ctx.end())
    {
        switch (*it)
        {
            case '-':
                sign_character = sign_option::minus;
                ++it;
                break;
            case '+':
                sign_character = sign_option::plus;
                ++it;
                break;
            case ' ':
                sign_character = sign_option::space;
                ++it;
                break;
            default:
                break;
        }
    }

    // Check for a padding character
    while (it != ctx.end() && *it >= '0' && *it <= '9')
    {
        padding_digits = padding_digits * 10 + (*it - '0');
        ++it;
    }

    // If there is a . then we need to capture the precision argument
    if (it != ctx.end() && *it == '.')
    {
        ++it;
        ctx_precision = 0;
        while (it != ctx.end() && *it >= '0' && *it <= '9')
        {
            ctx_precision = ctx_precision * 10 + (*it - '0');
            ++it;
        }
    }

    // Lastly we capture the format to include if it's upper case
    if (it != ctx.end() && *it != '}')
    {
        switch (*it)
        {
            case 'G':
                is_upper = true;
                fmt = chars_format::general;
                break;
            case 'g':
                fmt = chars_format::general;
                break;

            case 'F':
                is_upper = true;
                fmt = chars_format::fixed;
                break;
            case 'f':
                fmt = chars_format::fixed;
                break;

            case 'E':
                is_upper = true;
                fmt = chars_format::scientific;
                break;
            case 'e':
                fmt = chars_format::scientific;
                break;

            case 'X':
                is_upper = true;
                fmt = chars_format::hex;
                break;
            case 'x':
                fmt = chars_format::hex;
                break;

            case 'A':
                if (ctx_precision != -1)
                {
                    BOOST_DECIMAL_THROW_EXCEPTION(std::logic_error("Cohort preservation is mutually exclusive with precision"));
                }

                is_upper = true;
                fmt = chars_format::cohort_preserving_scientific;
                break;
            case 'a':
                if (ctx_precision != -1)
                {
                    BOOST_DECIMAL_THROW_EXCEPTION(std::logic_error("Cohort preservation is mutually exclusive with precision"));
                }

                fmt = chars_format::cohort_preserving_scientific;
                break;
            // LCOV_EXCL_START
            default:
                BOOST_DECIMAL_THROW_EXCEPTION(std::logic_error("Invalid format specifier"));
            // LCOV_EXCL_STOP
        }
        ++it;
    }

    // Verify we're at the closing brace
    if (it != ctx.end() && *it != '}')
    {
        BOOST_DECIMAL_THROW_EXCEPTION(std::logic_error("Expected '}' in format string")); // LCOV_EXCL_LINE
    }

    return std::make_tuple(ctx_precision, fmt, is_upper, padding_digits, sign_character, it);
}

template <typename T>
struct formatter
{
    sign_option sign;
    chars_format fmt;
    int padding_digits;
    int ctx_precision;
    bool is_upper;

    constexpr formatter() : sign{sign_option::minus},
                            fmt{chars_format::general},
                            padding_digits{0},
                            ctx_precision{6},
                            is_upper{false} {}

    constexpr auto parse(fmt::format_parse_context &ctx)
    {
        const auto res {boost::decimal::detail::fmt_detail::parse_impl(ctx)};

        ctx_precision = std::get<0>(res);
        fmt = std::get<1>(res);
        is_upper = std::get<2>(res);
        padding_digits = std::get<3>(res);
        sign = std::get<4>(res);

        return std::get<5>(res);
    }

    template <typename FormatContext>
    auto format(const T& v, FormatContext& ctx) const
    {
        std::array<char, 128> buffer {};
        auto buffer_front = buffer.data();
        bool has_sign {false};
        switch (sign)
        {
            case sign_option::minus:
                if (signbit(v))
                {
                    has_sign = true;
                }
                break;
            case sign_option::plus:
                if (!signbit(v))
                {
                    *buffer_front++ = '+';
                }
                has_sign = true;
                break;
            case sign_option::space:
                if (!signbit(v))
                {
                    *buffer_front++ = ' ';
                }
                has_sign = true;
                break;
            // LCOV_EXCL_START
            default:
                BOOST_DECIMAL_UNREACHABLE;
            // LCOV_EXCL_STOP
        }

        boost::decimal::to_chars_result r {};
        if (ctx_precision != -1)
        {
            r = boost::decimal::to_chars(buffer_front, buffer.data() + buffer.size(), v, fmt, ctx_precision);
        }
        else
        {
            r = boost::decimal::to_chars(buffer_front, buffer.data() + buffer.size(), v, fmt);
        }

        std::string s(buffer.data(), static_cast<std::size_t>(r.ptr - buffer.data()));

        if (is_upper)
        {
            #ifdef _MSC_VER
            #  pragma warning(push)
            #  pragma warning(disable : 4244)
            #endif

            std::transform(s.begin() + static_cast<std::size_t>(has_sign), s.end(), s.begin() + static_cast<std::size_t>(has_sign),
                           [](unsigned char c)
                           { return std::toupper(c); });

            #ifdef _MSC_VER
            #  pragma warning(pop)
            #endif
        }

        if (s.size() < static_cast<std::size_t>(padding_digits))
        {
            s.insert(s.begin() + static_cast<std::size_t>(has_sign), static_cast<std::size_t>(padding_digits) - s.size(), '0');
        }

        return fmt::format_to(ctx.out(), "{}", s);
    }
};

} // namespace fmt_detail
} // namespace detail
} // namespace decimal
} // Namespace boost

namespace fmt {

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

} // namespace fmt

#endif // __has_include(<fmt/format.h>)

#endif // BOOST_DECIMAL_FMT_FORMAT_HPP
