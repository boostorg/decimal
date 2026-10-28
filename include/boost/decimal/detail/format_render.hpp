// Copyright 2026 Shen-Ta Hsieh
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_DECIMAL_DETAIL_FORMAT_RENDER_HPP
#define BOOST_DECIMAL_DETAIL_FORMAT_RENDER_HPP

#include <boost/decimal/detail/config.hpp>
#include <boost/decimal/charconv.hpp>

#ifndef BOOST_DECIMAL_BUILD_MODULE
#include <algorithm>
#include <climits>
#include <cstddef>
#include <iterator>
#include <memory>
#include <string>
#endif

namespace boost {
namespace decimal {
namespace detail {

enum class format_sign_option
{
    plus,
    minus,
    space
};

enum class format_align_option
{
    none,
    left,    // '<'
    right,   // '>'
    center   // '^'
};

// The parsed specification of std::format and {fmt}
template <typename CharT>
struct format_spec
{
    chars_format fmt {chars_format::general};
    int precision {-1};
    int width {};
    format_sign_option sign {format_sign_option::minus};
    format_align_option alignment {format_align_option::none};
    CharT fill {static_cast<CharT>(' ')};
    bool is_upper {};
    bool use_locale {};
};

// Reads a width or a precision. A number above INT_MAX sets overflow
template <typename Iterator>
constexpr auto parse_format_number(Iterator& it, const Iterator end, bool& overflow) -> int
{
    using char_type = typename std::iterator_traits<Iterator>::value_type;

    int value {};
    while (it != end && *it >= static_cast<char_type>('0') && *it <= static_cast<char_type>('9'))
    {
        const auto digit {static_cast<int>(*it - static_cast<char_type>('0'))};
        overflow = overflow || value > (INT_MAX - digit) / 10;
        value = overflow ? INT_MAX : value * 10 + digit;
        ++it;
    }

    return value;
}

template <typename char_type>
constexpr auto format_alignment(const char_type c) -> format_align_option
{
    return c == static_cast<char_type>('<') ? format_align_option::left :
           c == static_cast<char_type>('>') ? format_align_option::right :
           c == static_cast<char_type>('^') ? format_align_option::center : format_align_option::none;
}

// Parses [[fill]align][sign][0][width][.precision][L][type][L] of std::format and {fmt} into spec, and
// returns the iterator at the closing brace. An error throws Error, or without exceptions returns the
// iterator at the bad character, so that the library reports the format string as invalid. Only a
// type that keeps the cohort of a value accepts a and A
template <typename Error, typename CharT, typename Iterator>
constexpr auto parse_format_spec(Iterator it, const Iterator end, format_spec<CharT>& spec, const bool keeps_cohort) -> Iterator
{
    using char_type = typename std::iterator_traits<Iterator>::value_type;
    #ifdef BOOST_DECIMAL_DISABLE_EXCEPTIONS
    const auto first {it};
    #endif

    // A fill is any character in front of an alignment
    if (it != end)
    {
        auto next {it};
        ++next;
        if (next != end && format_alignment(*next) != format_align_option::none)
        {
            spec.fill = static_cast<CharT>(*it);
            spec.alignment = format_alignment(*next);
            it = ++next;
        }
        else if (format_alignment(*it) != format_align_option::none)
        {
            spec.alignment = format_alignment(*it);
            ++it;
        }
    }

    if (it != end && (*it == static_cast<char_type>('-') || *it == static_cast<char_type>('+') || *it == static_cast<char_type>(' ')))
    {
        spec.sign = *it == static_cast<char_type>('+') ? format_sign_option::plus :
                    *it == static_cast<char_type>(' ') ? format_sign_option::space : format_sign_option::minus;
        ++it;
    }

    // A 0 before the width pads with zeros after the sign, but an alignment overrides it
    if (it != end && *it == static_cast<char_type>('0'))
    {
        if (spec.alignment == format_align_option::none)
        {
            spec.fill = static_cast<CharT>('0');
        }
        ++it;
    }

    bool overflow {false};
    spec.width = parse_format_number(it, end, overflow);
    if (it != end && *it == static_cast<char_type>('.'))
    {
        ++it;
        spec.precision = parse_format_number(it, end, overflow);
    }
    if (overflow)
    {
        #ifndef BOOST_DECIMAL_DISABLE_EXCEPTIONS
        throw Error("The width or the precision is above INT_MAX");
        #else
        return first;
        #endif
    }

    // The standard puts L before the type, and the older syntax puts it after the type
    if (it != end && *it == static_cast<char_type>('L'))
    {
        spec.use_locale = true;
        ++it;
    }

    // A capital type gives uppercase output
    if (it != end && *it != static_cast<char_type>('}'))
    {
        const auto type {*it};
        if (type == static_cast<char_type>('g') || type == static_cast<char_type>('G'))
        {
            spec.fmt = chars_format::general;
        }
        else if (type == static_cast<char_type>('f') || type == static_cast<char_type>('F'))
        {
            spec.fmt = chars_format::fixed;
        }
        else if (type == static_cast<char_type>('e') || type == static_cast<char_type>('E'))
        {
            spec.fmt = chars_format::scientific;
        }
        else if (type == static_cast<char_type>('x') || type == static_cast<char_type>('X'))
        {
            spec.fmt = chars_format::hex;
        }
        else if ((type == static_cast<char_type>('a') || type == static_cast<char_type>('A')) && spec.precision == -1 && keeps_cohort)
        {
            spec.fmt = chars_format::cohort_preserving_scientific;
        }
        else
        {
            // An unknown type, or a cohort preservation with a precision or for a fast type
            #ifndef BOOST_DECIMAL_DISABLE_EXCEPTIONS
            throw Error("Invalid format specifier");
            #else
            return it;
            #endif
        }
        spec.is_upper = type >= static_cast<char_type>('A') && type <= static_cast<char_type>('Z');
        ++it;
    }

    if (it != end && *it == static_cast<char_type>('L'))
    {
        spec.use_locale = true;
        ++it;
    }

    if (it != end && *it != static_cast<char_type>('}'))
    {
        #ifndef BOOST_DECIMAL_DISABLE_EXCEPTIONS
        throw Error("Expected '}' in format string");
        #else
        return it;
        #endif
    }

    return it;
}

// Writes v for std::format and {fmt} with the parsed specification, and reports a failed conversion
// with the Error of the library. localize gives the characters of [first, last) with the locale of the
// format context, in the output character type
template <typename Error, typename CharT, BOOST_DECIMAL_DECIMAL_FLOATING_TYPE T, typename OutputIt, typename Localize>
auto format_decimal(const T& v, const format_spec<CharT>& spec, OutputIt out, Localize localize) -> OutputIt
{
    // The buffer holds the sign and the longest to_chars result, on the stack when it is short
    const auto length {to_chars_length(v, spec.fmt, spec.precision) + 1U};
    char local_buffer[128];
    std::unique_ptr<char[]> heap_buffer {length > sizeof(local_buffer) ? new char[length] : nullptr};
    char* const buffer_first {heap_buffer ? heap_buffer.get() : local_buffer};
    char* const buffer_last {buffer_first + (heap_buffer ? length : sizeof(local_buffer))};

    // to_chars writes a minus sign, and a + or space sign goes in front of it
    auto buffer_front = buffer_first;
    const bool has_sign {spec.sign != format_sign_option::minus || signbit(v)};
    if (spec.sign != format_sign_option::minus && !signbit(v))
    {
        *buffer_front++ = spec.sign == format_sign_option::plus ? '+' : ' ';
    }

    const auto r {spec.precision != -1 ? to_chars(buffer_front, buffer_last, v, spec.fmt, spec.precision) : to_chars(buffer_front, buffer_last, v, spec.fmt)};
    // Without exceptions, a failed conversion writes nothing
    if (!r)
    {
        #ifndef BOOST_DECIMAL_DISABLE_EXCEPTIONS
        throw Error("The value cannot be formatted with this specification");
        #else
        return out;
        #endif
    }

    if (spec.is_upper)
    {
        make_uppercase(buffer_first, r.ptr);
    }

    // The output takes the character type of the context, which can differ from CharT: {fmt} gives
    // FMT_COMPILE(L"...") with format_to_n a char context (#1537)
    using string_type = decltype(localize(buffer_front, r.ptr));
    using output_char_type = typename string_type::value_type;

    // L takes the grouping, the separator and the decimal point from the locale of the context,
    // as operator<< does, and a + or space sign stays in front
    string_type localized {};
    if (spec.use_locale)
    {
        localized = localize(buffer_front, r.ptr);
    }
    const auto prefix_size {static_cast<std::size_t>(buffer_front - buffer_first)};
    const auto text_size {spec.use_locale ? localized.size() : static_cast<std::size_t>(r.ptr - buffer_front)};
    const auto size {prefix_size + text_size};
    const auto padding {spec.width > 0 && size < static_cast<std::size_t>(spec.width) ? static_cast<std::size_t>(spec.width) - size : 0U};

    // A number aligns right, and the zeros of the 0 flag go after its sign
    std::size_t left_pad {};
    std::size_t zeros {};
    std::size_t right_pad {};
    auto left_fill {spec.fill};
    switch (spec.alignment)
    {
        case format_align_option::left:
            right_pad = padding;
            break;
        case format_align_option::right:
            left_pad = padding;
            break;
        case format_align_option::center:
            left_pad = padding / 2U;
            right_pad = padding - left_pad;
            break;
        case format_align_option::none:
        default:
            if (spec.fill == static_cast<CharT>('0') && isfinite(v))
            {
                zeros = padding;
            }
            else
            {
                left_pad = padding;
                left_fill = static_cast<CharT>(' ');
            }
            break;
    }

    // The padding and the characters go straight to the output. The minus sign of to_chars
    // starts the text, and the zeros go after it
    const auto minus_size {static_cast<std::size_t>(zeros != 0U && prefix_size == 0U && has_sign)};
    out = std::fill_n(out, left_pad, static_cast<output_char_type>(left_fill));
    out = std::copy(buffer_first, buffer_front, out);
    if (spec.use_locale)
    {
        out = std::copy(localized.begin(), localized.begin() + static_cast<std::ptrdiff_t>(minus_size), out);
        out = std::fill_n(out, zeros, static_cast<output_char_type>('0'));
        out = std::copy(localized.begin() + static_cast<std::ptrdiff_t>(minus_size), localized.end(), out);
    }
    else
    {
        out = std::copy(buffer_front, buffer_front + minus_size, out);
        out = std::fill_n(out, zeros, static_cast<output_char_type>('0'));
        out = std::copy(buffer_front + minus_size, r.ptr, out);
    }

    return std::fill_n(out, right_pad, static_cast<output_char_type>(spec.fill));
}

} // namespace detail
} // namespace decimal
} // namespace boost

#endif // BOOST_DECIMAL_DETAIL_FORMAT_RENDER_HPP
