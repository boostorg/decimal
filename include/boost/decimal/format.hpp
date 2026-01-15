// Copyright 2023 - 2024 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_DECIMAL_FORMAT_HPP
#define BOOST_DECIMAL_FORMAT_HPP

#if __has_include(<format>) && defined(__cpp_lib_format) && __cpp_lib_format >= 201907L && !defined(BOOST_DECIMAL_DISABLE_CLIB)

#define BOOST_DECIMAL_HAS_FORMAT_SUPPORT

#include <boost/decimal/detail/config.hpp>
#include <boost/decimal/detail/locale_conversion.hpp>
#include <boost/decimal/charconv.hpp>
#include <algorithm>
#include <format>
#include <iostream>
#include <iomanip>
#include <string>
#include <tuple>
#include <cctype>

// Default :g
// Fixed :f
// Scientific :e
// Hex :x
// Cohort Preserving :a
//
// Capital letter for any of the above leads to all characters being uppercase

namespace boost::decimal::detail {

enum class format_sign_option
{
    plus,
    minus,
    space
};

template <typename ParseContext>
constexpr auto parse_impl(ParseContext &ctx)
{
    using CharType = typename ParseContext::char_type;

    auto sign_character = format_sign_option::minus;
    auto it {ctx.begin()};
    int ctx_precision = -1;
    boost::decimal::chars_format fmt = boost::decimal::chars_format::general;
    bool is_upper = false;
    int padding_digits = 0;
    bool use_locale = false;

    // Check for a sign character
    if (it != ctx.end())
    {
        switch (*it)
        {
            case static_cast<CharType>('-'):
                sign_character = format_sign_option::minus;
                ++it;
                break;
            case static_cast<CharType>('+'):
                sign_character = format_sign_option::plus;
                ++it;
                break;
            case static_cast<CharType>(' '):
                sign_character = format_sign_option::space;
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
    if (it != ctx.end() && *it != static_cast<CharType>('}'))
    {
        switch (*it)
        {
            case 'G':
                is_upper = true;
                [[fallthrough]];
            case 'g':
                fmt = chars_format::general;
                break;

            case 'F':
                is_upper = true;
                [[fallthrough]];
            case 'f':
                fmt = chars_format::fixed;
                break;

            case 'E':
                is_upper = true;
                [[fallthrough]];
            case 'e':
                fmt = chars_format::scientific;
                break;

            case 'X':
                is_upper = true;
                [[fallthrough]];
            case 'x':
                fmt = chars_format::hex;
                break;

            case 'A':
                is_upper = true;
                [[fallthrough]];
            case 'a':
                if (ctx_precision != -1)
                {
                    BOOST_DECIMAL_THROW_EXCEPTION(std::format_error("Cohort preservation is mutually exclusive with precision"));
                }

                fmt = chars_format::cohort_preserving_scientific;
                break;

            // LCOV_EXCL_START
            default:
                BOOST_DECIMAL_THROW_EXCEPTION(std::format_error("Invalid format specifier"));
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
        BOOST_DECIMAL_THROW_EXCEPTION(std::format_error("Expected '}' in format string")); // LCOV_EXCL_LINE
    }

    return std::make_tuple(ctx_precision, fmt, is_upper, padding_digits, sign_character, use_locale, it);
}

template <typename>
struct formattable_character_type : std::false_type {};

template <>
struct formattable_character_type<char> : std::true_type {};

template <>
struct formattable_character_type<wchar_t> : std::true_type {};

template <typename CharT>
inline constexpr bool is_formattable_character_type_v = formattable_character_type<CharT>::value;

} // Namespace boost::decimal::detail

namespace std {

template <boost::decimal::detail::concepts::decimal_floating_point_type T, typename CharT>
    requires boost::decimal::detail::is_formattable_character_type_v<CharT>
struct formatter<T, CharT>
{
    boost::decimal::chars_format fmt;
    boost::decimal::detail::format_sign_option sign;
    int ctx_precision;
    int padding_digits;
    bool is_upper;
    bool use_locale;

    constexpr formatter() : fmt(boost::decimal::chars_format::general),
                            sign(boost::decimal::detail::format_sign_option::minus),
                            ctx_precision(6),
                            padding_digits(0),
                            is_upper(false),
                            use_locale(false)
    {}

    constexpr auto parse(basic_format_parse_context<CharT>& ctx)
    {
        const auto res {boost::decimal::detail::parse_impl(ctx)};

        ctx_precision = std::get<0>(res);
        fmt = std::get<1>(res);
        is_upper = std::get<2>(res);
        padding_digits = std::get<3>(res);
        sign = std::get<4>(res);
        use_locale = std::get<5>(res);

        return std::get<6>(res);
    }

    template <typename FormatContext>
    auto format(const T &v, FormatContext &ctx) const
    {
        using namespace boost::decimal;
        using namespace boost::decimal::detail;

        using CharType = FormatContext::char_type;
        static_assert(is_formattable_character_type_v<CharType>, "This is an unsupported character type. Only char and wchar_t can be used with std::format");

        std::array<char, 128> buffer {};
        auto buffer_front = buffer.data();
        bool has_sign {false};
        switch (sign)
        {
            case format_sign_option::minus:
                if (signbit(v))
                {
                    has_sign = true;
                }
                break;
            case format_sign_option::plus:
                if (!signbit(v))
                {
                    *buffer_front++ = '+';
                }
                has_sign = true;
                break;
            case format_sign_option::space:
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
            r = to_chars(buffer_front, buffer.data() + buffer.size(), v, fmt, ctx_precision);
        }
        else
        {
            r = to_chars(buffer_front, buffer.data() + buffer.size(), v, fmt);
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

        // std <format> only supports char and wchar_t
        if constexpr (std::is_same_v<CharType, char>)
        {
            return std::format_to(ctx.out(), "{}", s);
        }
        else
        {
            std::wstring result;
            result.reserve(s.size());
            for (const char c : s)
            {
                result.push_back(static_cast<CharType>(static_cast<unsigned char>(c)));
            }

            return std::format_to(ctx.out(), L"{}", result);
        }
    }
};

} // Namespace std

#endif // <format> compatibility

#endif //BOOST_DECIMAL_FORMAT_HPP
