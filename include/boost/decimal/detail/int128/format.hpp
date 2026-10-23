// Copyright 2025 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_DECIMAL_DETAIL_INT128_FORMAT_HPP
#define BOOST_DECIMAL_DETAIL_INT128_FORMAT_HPP

// The feature-test macros below are only visible once <version> has been seen.
// The module build gets them from the global module fragment instead.
#ifndef BOOST_DECIMAL_DETAIL_INT128_BUILD_MODULE
#  if __has_include(<version>)
#    include <version>
#  endif
#endif

#if __has_include(<format>) && defined(__cpp_lib_format) && __cpp_lib_format >= 201907L

#include <boost/decimal/detail/int128/detail/mini_to_chars.hpp>
#include <boost/decimal/detail/int128/detail/config.hpp>
#include <boost/decimal/detail/int128/int128.hpp>

#ifndef BOOST_DECIMAL_DETAIL_INT128_BUILD_MODULE
#include <string>
#include <format>
#include <tuple>
#endif

#define BOOST_DECIMAL_DETAIL_INT128_HAS_FORMAT

#if defined(__cpp_lib_constexpr_format) && __cpp_lib_constexpr_format >= 202511L
#  define BOOST_DECIMAL_DETAIL_INT128_HAS_CONSTEXPR_FORMAT
#  define BOOST_DECIMAL_DETAIL_INT128_CONSTEXPR_FORMAT constexpr
#else
#  define BOOST_DECIMAL_DETAIL_INT128_CONSTEXPR_FORMAT
#endif

namespace boost::int128::detail {

enum class sign_option
{
    plus,
    negative,
    space
};

enum class alignment
{
    none,
    left,   // <
    right,  // >
    center  // ^
};

template <typename ParseContext>
constexpr auto parse_impl(ParseContext& ctx)
{
    auto it {ctx.begin()};
    int base = 10;
    bool is_upper = false;
    int padding_digits = 0;
    auto sign = sign_option::negative;
    bool prefix = false;
    bool zero_pad = false;
    char fill_char = ' ';
    auto align = alignment::none;

    // Parse fill and alignment: [[fill]align]
    // Alignment characters are: < (left), > (right), ^ (center)
    if (it != ctx.end())
    {
        // Check if we have [fill]align (fill char followed by alignment)
        auto next = it;
        ++next;
        if (next != ctx.end() && (*next == '<' || *next == '>' || *next == '^'))
        {
            fill_char = *it;
            it = next;
            switch (*it)
            {
                case '<':
                    align = alignment::left;
                    break;
                case '>':
                    align = alignment::right;
                    break;
                case '^':
                    align = alignment::center;
                    break;
                default:                        // LCOV_EXCL_LINE
                    BOOST_DECIMAL_DETAIL_INT128_UNREACHABLE;   // LCOV_EXCL_LINE
            }
            ++it;
        }
        // Check if we just have align (no fill char)
        else if (*it == '<' || *it == '>' || *it == '^')
        {
            switch (*it)
            {
                case '<':
                    align = alignment::left;
                    break;
                case '>':
                    align = alignment::right;
                    break;
                case '^':
                    align = alignment::center;
                    break;
                default:                        // LCOV_EXCL_LINE
                    BOOST_DECIMAL_DETAIL_INT128_UNREACHABLE;   // LCOV_EXCL_LINE
            }
            ++it;
        }
    }

    // Handle sign or space
    if (it != ctx.end())
    {
        switch (*it) {
            case ' ':
                sign = sign_option::space;
                ++it;
                break;
            case '+':
                sign = sign_option::plus;
                ++it;
                break;
            case '-':
                sign = sign_option::negative;
                ++it;
                break;
            default:
                break;
        }
    }

    // Alternate form option
    if (it != ctx.end() && *it == '#')
    {
        prefix = true;
        ++it;
    }

    // Zero-pad flag (std::format places '0' before the width)
    if (it != ctx.end() && *it == '0')
    {
        zero_pad = true;
        ++it;
    }

    // Parse the width
    while (it != ctx.end() && *it >= '0' && *it <= '9')
    {
        padding_digits = padding_digits * 10 + (*it - '0');
        ++it;
    }

    // Integer presentation
    if (it != ctx.end() && *it != '}')
    {
        switch (*it++)
        {
            case 'b':
                base = 2;
                break;
            case 'B':
                base = 2;
                is_upper = true;
                break;

            case 'o':
                base = 8;
                break;

            case 'd':
                base = 10;
                break;

            case 'x':
                base = 16;
                break;
            case 'X':
                base = 16;
                is_upper = true;
                break;
            default:                                                                                // LCOV_EXCL_LINE
                BOOST_DECIMAL_DETAIL_INT128_THROW_EXCEPTION(std::format_error("Unsupported format specifier"));    // LCOV_EXCL_LINE
        }
    }

    // Verify we're at the closing brace
    if (it != ctx.end() && *it != '}')
    {
        BOOST_DECIMAL_DETAIL_INT128_THROW_EXCEPTION(std::format_error("Expected '}' in format string")); // LCOV_EXCL_LINE
    }

    return std::make_tuple(base, padding_digits, sign, is_upper, prefix, zero_pad, fill_char, align, it);
}

template <typename T>
struct is_library_type_impl
{
    static constexpr bool value {std::is_same_v<T, boost::int128::uint128> || std::is_same_v<T, boost::int128::int128>};
};

template <typename T>
BOOST_DECIMAL_DETAIL_INT128_INLINE_CONSTEXPR bool is_library_type_v = is_library_type_impl<T>::value;

template <typename T>
concept is_library_type = is_library_type_v<T>;

} // namespace boost::int128::detail

namespace std {

template <boost::int128::detail::is_library_type T>
struct formatter<T>
{
    int base;
    int padding_digits;
    boost::int128::detail::sign_option sign;
    bool is_upper;
    bool prefix;
    bool zero_pad;
    char fill_char;
    boost::int128::detail::alignment align;

    constexpr formatter() : base {10},
                            padding_digits {0},
                            sign {boost::int128::detail::sign_option::negative},
                            is_upper {false},
                            prefix {false},
                            zero_pad {false},
                            fill_char {' '},
                            align {boost::int128::detail::alignment::none}
    {}

    constexpr auto parse(format_parse_context& ctx)
    {
        const auto res {boost::int128::detail::parse_impl(ctx)};

        base = std::get<0>(res);
        padding_digits = std::get<1>(res);
        sign = std::get<2>(res);
        is_upper = std::get<3>(res);
        prefix = std::get<4>(res);
        zero_pad = std::get<5>(res);
        fill_char = std::get<6>(res);
        align = std::get<7>(res);

        return std::get<8>(res);
    }

    template <typename FormatContext>
    BOOST_DECIMAL_DETAIL_INT128_CONSTEXPR_FORMAT auto format(T v, FormatContext& ctx) const
    {
        char buffer[boost::int128::detail::mini_to_chars_buffer_size];
        bool isneg {false};
        boost::int128::uint128 abs_v {};

        if constexpr (std::is_same_v<T, boost::int128::int128>)
        {
            if (v < 0)
            {
                isneg = true;
                // Can't negate int128::min(), handle specially
                if (v == (std::numeric_limits<T>::min)())
                {
                    abs_v = boost::int128::uint128{UINT64_C(0x8000000000000000), 0};
                }
                else
                {
                    abs_v = static_cast<boost::int128::uint128>(-v);
                }
            }
            else
            {
                abs_v = static_cast<boost::int128::uint128>(v);
            }
        }
        else
        {
            abs_v = static_cast<boost::int128::uint128>(v);
        }

        const auto end = boost::int128::detail::mini_to_chars(buffer, abs_v, base, is_upper);
        std::string s(end, buffer + sizeof(buffer));

        // Calculate prefix length that will be added later
        std::size_t prefix_len {0};
        if (prefix)
        {
            switch (base)
            {
                case 2:
                case 16:
                    prefix_len = 2;  // "0b", "0B", "0x", or "0X"
                    break;
                case 8:
                    prefix_len = 1;  // "0"
                    break;
                default:
                    break;
            }
        }

        // Calculate sign length that will be added later
        std::size_t sign_len {0};
        if (sign == boost::int128::detail::sign_option::plus || sign == boost::int128::detail::sign_option::space || isneg)
        {
            sign_len = 1;
        }

        // Zero-padding applies only with the '0' flag and no explicit alignment.
        // Account for prefix and sign in the padding calculation.
        if (zero_pad && align == boost::int128::detail::alignment::none && padding_digits > 0)
        {
            auto target_digit_width {static_cast<std::size_t>(padding_digits)};
            if (target_digit_width > prefix_len + sign_len)
            {
                target_digit_width -= prefix_len + sign_len;
            }
            else
            {
                target_digit_width = 0;
            }

            if (s.size() - 1u < target_digit_width)
            {
                s.insert(s.begin(), target_digit_width - s.size() + 1u, '0');
            }
        }

        if (prefix)
        {
            switch (base)
            {
                case 2:
                    if (is_upper)
                    {
                        s.insert(s.begin(), 'B');
                    }
                    else
                    {
                        s.insert(s.begin(), 'b');
                    }
                    s.insert(s.begin(), '0');
                    break;
                case 8:
                    s.insert(s.begin(), '0');
                    break;
                case 16:
                    if (is_upper)
                    {
                        s.insert(s.begin(), 'X');
                    }
                    else
                    {
                        s.insert(s.begin(), 'x');
                    }
                    s.insert(s.begin(), '0');
                    break;
                default:
                    // Nothing to do
                    break;
            }
        }

        // Insert our sign
        switch (sign)
        {
            case boost::int128::detail::sign_option::plus:
                if (isneg)
                {
                    s.insert(s.begin(), '-');
                }
                else
                {
                    s.insert(s.begin(), '+');
                }
                break;
            case boost::int128::detail::sign_option::space:
                if (!isneg)
                {
                    s.insert(s.begin(), ' ');
                }
                if constexpr (std::is_same_v<T, boost::int128::int128>)
                {
                    if (isneg)
                    {
                        s.insert(s.begin(), '-');
                    }
                }
                break;
            case boost::int128::detail::sign_option::negative:
                if constexpr (std::is_same_v<T, boost::int128::int128>)
                {
                    if (isneg)
                    {
                        s.insert(s.begin(), '-');
                    }
                }
                break;
            default:                        // LCOV_EXCL_LINE
                BOOST_DECIMAL_DETAIL_INT128_UNREACHABLE;   // LCOV_EXCL_LINE
        }

        s.erase(0, s.find_first_not_of('\0'));
        s.erase(s.find_last_not_of('\0') + 1);

        // Apply alignment. An explicit alignment uses fill_char; with no explicit
        // alignment and no zero-padding, integer types default to right alignment
        // with the fill character (matching std::format).
        if (s.size() < static_cast<std::size_t>(padding_digits) &&
            (align != boost::int128::detail::alignment::none || !zero_pad))
        {
            auto fill_count = static_cast<std::size_t>(padding_digits) - s.size();
            const auto effective_align = (align == boost::int128::detail::alignment::none)
                                             ? boost::int128::detail::alignment::right
                                             : align;
            switch (effective_align)
            {
                case boost::int128::detail::alignment::left:
                    s.append(fill_count, fill_char);
                    break;
                case boost::int128::detail::alignment::right:
                    s.insert(s.begin(), fill_count, fill_char);
                    break;
                case boost::int128::detail::alignment::center:
                {
                    auto left_fill = fill_count / 2;
                    auto right_fill = fill_count - left_fill;
                    s.insert(s.begin(), left_fill, fill_char);
                    s.append(right_fill, fill_char);
                    break;
                }
                default:                        // LCOV_EXCL_LINE
                    break;                      // LCOV_EXCL_LINE
            }
        }

        return std::format_to(ctx.out(), "{}", s);
    }
};

} // namespace std

#endif

#endif // BOOST_DECIMAL_DETAIL_INT128_FORMAT_HPP
