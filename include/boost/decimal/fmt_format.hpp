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
#include <boost/decimal/detail/locale_conversion.hpp>
#include <boost/decimal/charconv.hpp>

#ifndef BOOST_DECIMAL_BUILD_MODULE

#include <algorithm>
#include <format>
#include <iostream>
#include <string>
#include <tuple>
#include <cctype>

#endif // BOOST_DECIMAL_BUILD_MODULE

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
    using CharType = typename ParseContext::char_type;

    auto sign_character = sign_option::minus;
    int ctx_precision = -1;
    boost::decimal::chars_format fmt = boost::decimal::chars_format::general;
    bool is_upper = false;
    int padding_digits = 0;
    bool use_locale = false;
    auto it {ctx.begin()};

    if (it == nullptr)
    {
        return std::make_tuple(ctx_precision, fmt, is_upper, padding_digits, sign_character, use_locale, it);
    }

    // Check for a sign character
    if (it != ctx.end())
    {
        switch (*it)
        {
            case static_cast<CharType>('-'):
                sign_character = sign_option::minus;
                ++it;
                break;
            case static_cast<CharType>('+'):
                sign_character = sign_option::plus;
                ++it;
                break;
            case static_cast<CharType>(' '):
                sign_character = sign_option::space;
                ++it;
                break;
            default:
                break;
        }
    }

    // Check for a padding character
    while (it != ctx.end() && *it >= static_cast<CharType>('0') && *it <= static_cast<CharType>('9'))
    {
        padding_digits = padding_digits * 10 + (*it - static_cast<CharType>('0'));
        ++it;
    }

    // If there is a . then we need to capture the precision argument
    if (it != ctx.end() && *it == static_cast<CharType>('.'))
    {
        ++it;
        ctx_precision = 0;
        while (it != ctx.end() && *it >= static_cast<CharType>('0') && *it <= static_cast<CharType>('9'))
        {
            ctx_precision = ctx_precision * 10 + (*it - static_cast<CharType>('0'));
            ++it;
        }
    }

    // Lastly we capture the format to include if it's upper case
    if (it != ctx.end() && *it != '}')
    {
        switch (*it)
        {
            case static_cast<CharType>('G'):
                is_upper = true;
                fmt = chars_format::general;
                break;
            case static_cast<CharType>('g'):
                fmt = chars_format::general;
                break;

            case static_cast<CharType>('F'):
                is_upper = true;
                fmt = chars_format::fixed;
                break;
            case static_cast<CharType>('f'):
                fmt = chars_format::fixed;
                break;

            case static_cast<CharType>('E'):
                is_upper = true;
                fmt = chars_format::scientific;
                break;
            case static_cast<CharType>('e'):
                fmt = chars_format::scientific;
                break;

            case static_cast<CharType>('X'):
                is_upper = true;
                fmt = chars_format::hex;
                break;
            case static_cast<CharType>('x'):
                fmt = chars_format::hex;
                break;

            case static_cast<CharType>('A'):
                if (ctx_precision != -1)
                {
                    BOOST_DECIMAL_THROW_EXCEPTION(std::logic_error("Cohort preservation is mutually exclusive with precision"));
                }

                is_upper = true;
                fmt = chars_format::cohort_preserving_scientific;
                break;
            case static_cast<CharType>('a'):
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

    // Check for the locale character
    if (it != ctx.end() && *it == static_cast<CharType>('L'))
    {
        use_locale = true;
        ++it;
    }

    // Verify we're at the closing brace
    if (it != ctx.end() && *it != static_cast<CharType>('}'))
    {
        BOOST_DECIMAL_THROW_EXCEPTION(std::logic_error("Expected '}' in format string")); // LCOV_EXCL_LINE
    }

    return std::make_tuple(ctx_precision, fmt, is_upper, padding_digits, sign_character, use_locale, it);
}

template <typename T>
struct formatter
{
    sign_option sign;
    chars_format fmt;
    int padding_digits;
    int ctx_precision;
    bool is_upper;
    bool use_locale;

    constexpr formatter() : sign{sign_option::minus},
                            fmt{chars_format::general},
                            padding_digits{0},
                            ctx_precision{6},
                            is_upper{false},
                            use_locale{false} {}

    template <typename CharType>
    constexpr auto parse(fmt::parse_context<CharType> &ctx)
    {
        const auto res {boost::decimal::detail::fmt_detail::parse_impl(ctx)};

        ctx_precision = std::get<0>(res);
        fmt = std::get<1>(res);
        is_upper = std::get<2>(res);
        padding_digits = std::get<3>(res);
        sign = std::get<4>(res);
        use_locale = std::get<5>(res);

        return std::get<6>(res);
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

        if (use_locale)
        {
            // We need approximately 1/3 more space in order to insert the thousands separators,
            // but after we have done our processing we need to shrink the string back down
            const auto initial_length {s.length()};
            s.resize(s.length() * 4 / 3 + 1);
            const auto offset {static_cast<std::size_t>(convert_pointer_pair_to_local_locale(const_cast<char*>(s.data()), s.data() + s.length()))};
            s.resize(initial_length + offset);
        }

        #ifdef BOOST_DECIMAL_NO_CXX17_IF_CONSTEXPR

        return fmt::format_to(ctx.out(), "{}", s);

        #else

        using CharType = typename FormatContext::char_type;

        if constexpr (std::is_same_v<CharType, char>)
        {
            return fmt::format_to(ctx.out(), "{}", s);
        }
        else if constexpr (std::is_same_v<CharType, wchar_t>)
        {
            std::wstring result;
            result.reserve(s.size());
            for (const char c : s)
            {
                result.push_back(static_cast<CharType>(static_cast<unsigned char>(c)));
            }

            return fmt::format_to(ctx.out(), L"{}", result);
        }
        else
        {
            // For other character types (char16_t, char32_t, etc.)
            
            std::basic_string<CharType> result;
            result.reserve(s.size());
            for (const char c : s)
            {
                result.push_back(static_cast<CharType>(static_cast<unsigned char>(c)));
            }

            if constexpr (std::is_same_v<CharType, char16_t>)
            {
                return fmt::format_to(ctx.out(), u"{}", result);
            }
            else if constexpr (std::is_same_v<CharType, char32_t>)
            {
                return fmt::format_to(ctx.out(), U"{}", result);
            }
            #ifdef BOOST_DECIMAL_HAS_CHAR8_T
            else
            {
                static_assert(std::is_same_v<CharType, char8_t>, "Unsupported wide character type");
                return fmt::format_to(ctx.out(), u8"{}", result);
            }
            #else
            else
            {
                static_assert(std::is_same_v<CharType, char>, "Unsupported wide character type");
                return fmt::format_to(ctx.out(), u8"{}", result);
            }
            #endif
        }

        #endif // BOOST_DECIMAL_NO_CXX17_IF_CONSTEXPR
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
    : public boost::decimal::detail::fmt_detail::formatter<boost::decimal::decimal32_t> {};

template <>
struct formatter<boost::decimal::decimal32_t, wchar_t>
    : public boost::decimal::detail::fmt_detail::formatter<boost::decimal::decimal32_t> {};

template <>
struct formatter<boost::decimal::decimal32_t, char16_t>
    : public boost::decimal::detail::fmt_detail::formatter<boost::decimal::decimal32_t> {};

template <>
struct formatter<boost::decimal::decimal32_t, char32_t>
    : public boost::decimal::detail::fmt_detail::formatter<boost::decimal::decimal32_t> {};

template <>
struct formatter<boost::decimal::decimal_fast32_t, char>
    : public boost::decimal::detail::fmt_detail::formatter<boost::decimal::decimal_fast32_t> {};

template <>
struct formatter<boost::decimal::decimal_fast32_t, wchar_t>
    : public boost::decimal::detail::fmt_detail::formatter<boost::decimal::decimal_fast32_t> {};

template <>
struct formatter<boost::decimal::decimal_fast32_t, char16_t>
    : public boost::decimal::detail::fmt_detail::formatter<boost::decimal::decimal_fast32_t> {};

template <>
struct formatter<boost::decimal::decimal_fast32_t, char32_t>
    : public boost::decimal::detail::fmt_detail::formatter<boost::decimal::decimal_fast32_t> {};

template <>
struct formatter<boost::decimal::decimal64_t, char>
    : public boost::decimal::detail::fmt_detail::formatter<boost::decimal::decimal64_t> {};

template <>
struct formatter<boost::decimal::decimal64_t, wchar_t>
    : public boost::decimal::detail::fmt_detail::formatter<boost::decimal::decimal64_t> {};

template <>
struct formatter<boost::decimal::decimal64_t, char16_t>
    : public boost::decimal::detail::fmt_detail::formatter<boost::decimal::decimal64_t> {};

template <>
struct formatter<boost::decimal::decimal64_t, char32_t>
    : public boost::decimal::detail::fmt_detail::formatter<boost::decimal::decimal64_t> {};

template <>
struct formatter<boost::decimal::decimal_fast64_t, char>
    : public boost::decimal::detail::fmt_detail::formatter<boost::decimal::decimal_fast64_t> {};

template <>
struct formatter<boost::decimal::decimal_fast64_t, wchar_t>
    : public boost::decimal::detail::fmt_detail::formatter<boost::decimal::decimal_fast64_t> {};

template <>
struct formatter<boost::decimal::decimal_fast64_t, char16_t>
    : public boost::decimal::detail::fmt_detail::formatter<boost::decimal::decimal_fast64_t> {};

template <>
struct formatter<boost::decimal::decimal_fast64_t, char32_t>
    : public boost::decimal::detail::fmt_detail::formatter<boost::decimal::decimal_fast64_t> {};

template <>
struct formatter<boost::decimal::decimal128_t, char>
    : public boost::decimal::detail::fmt_detail::formatter<boost::decimal::decimal128_t> {};

template <>
struct formatter<boost::decimal::decimal128_t, wchar_t>
    : public boost::decimal::detail::fmt_detail::formatter<boost::decimal::decimal128_t> {};

template <>
struct formatter<boost::decimal::decimal128_t, char16_t>
    : public boost::decimal::detail::fmt_detail::formatter<boost::decimal::decimal128_t> {};

template <>
struct formatter<boost::decimal::decimal128_t, char32_t>
    : public boost::decimal::detail::fmt_detail::formatter<boost::decimal::decimal128_t> {};

template <>
struct formatter<boost::decimal::decimal_fast128_t, char>
    : public boost::decimal::detail::fmt_detail::formatter<boost::decimal::decimal_fast128_t> {};

template <>
struct formatter<boost::decimal::decimal_fast128_t, wchar_t>
    : public boost::decimal::detail::fmt_detail::formatter<boost::decimal::decimal_fast128_t> {};

template <>
struct formatter<boost::decimal::decimal_fast128_t, char16_t>
    : public boost::decimal::detail::fmt_detail::formatter<boost::decimal::decimal_fast128_t> {};

template <>
struct formatter<boost::decimal::decimal_fast128_t, char32_t>
    : public boost::decimal::detail::fmt_detail::formatter<boost::decimal::decimal_fast128_t> {};

#ifdef BOOST_DECIMAL_HAS_CHAR8_T

template <>
struct formatter<boost::decimal::decimal32_t, char8_t>
    : public boost::decimal::detail::fmt_detail::formatter<boost::decimal::decimal32_t> {};

template <>
struct formatter<boost::decimal::decimal_fast32_t, char8_t>
    : public boost::decimal::detail::fmt_detail::formatter<boost::decimal::decimal_fast32_t> {};

template <>
struct formatter<boost::decimal::decimal64_t, char8_t>
    : public boost::decimal::detail::fmt_detail::formatter<boost::decimal::decimal64_t> {};

template <>
struct formatter<boost::decimal::decimal_fast64_t, char8_t>
    : public boost::decimal::detail::fmt_detail::formatter<boost::decimal::decimal_fast64_t> {};

template <>
struct formatter<boost::decimal::decimal128_t, char8_t>
    : public boost::decimal::detail::fmt_detail::formatter<boost::decimal::decimal128_t> {};

template <>
struct formatter<boost::decimal::decimal_fast128_t, char8_t>
    : public boost::decimal::detail::fmt_detail::formatter<boost::decimal::decimal_fast128_t> {};

#endif // BOOST_DECIMAL_HAS_CHAR8_T

#endif

} // namespace fmt

#endif // __has_include(<fmt/format.h>)

#endif // BOOST_DECIMAL_FMT_FORMAT_HPP
