// Copyright 2023 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_DECIMAL_DETAIL_IO_HPP
#define BOOST_DECIMAL_DETAIL_IO_HPP

#include <boost/decimal/fwd.hpp>
#include <boost/decimal/detail/config.hpp>
#include <boost/decimal/detail/type_traits.hpp>
#include <boost/decimal/detail/parser.hpp>
#include <boost/decimal/detail/attributes.hpp>
#include <boost/decimal/detail/fenv_rounding.hpp>
#include <boost/decimal/detail/concepts.hpp>
#include <boost/decimal/detail/locale_conversion.hpp>
#include <boost/decimal/charconv.hpp>

#if !defined(BOOST_DECIMAL_DISABLE_CLIB)

#ifndef BOOST_DECIMAL_BUILD_MODULE
#include <cerrno>
#include <cstring>
#include <limits>
#include <ios>
#include <iostream>
#include <locale>
#include <new>
#include <system_error>
#include <type_traits>
#include <string>
#include <memory>
#endif

namespace boost {
namespace decimal {

// 3.2.10 Formatted input:
BOOST_DECIMAL_EXPORT template <typename charT, typename traits, BOOST_DECIMAL_DECIMAL_FLOATING_TYPE DecimalType>
auto operator>>(std::basic_istream<charT, traits>& is, DecimalType& d)
    -> std::enable_if_t<detail::is_decimal_floating_point_v<DecimalType>, std::basic_istream<charT, traits>&>
{
    constexpr std::size_t static_buffer_size {1024U};

    std::basic_string<charT, traits> t_buffer;
    is >> std::ws >> t_buffer;

    const auto t_buffer_len {t_buffer.length()};

    char static_buffer[static_buffer_size] {};
    std::unique_ptr<char[]> longer_char_buffer {nullptr};
    char* buffer {static_buffer};

    if (BOOST_DECIMAL_UNLIKELY(t_buffer_len > static_buffer_size))
    {
        longer_char_buffer = std::unique_ptr<char[]>(new(std::nothrow) char[t_buffer_len]);
        if (longer_char_buffer.get() == nullptr)
        {
            errno = ENOMEM;
            return is;
        }

        buffer = longer_char_buffer.get();
    }

    BOOST_DECIMAL_IF_CONSTEXPR (!std::is_same<charT, char>::value)
    {
        auto first {buffer};
        auto t_first {t_buffer.begin()};
        auto t_buffer_end {t_buffer.end()};

        while (t_first != t_buffer_end)
        {
            *first++ = static_cast<char>(*t_first++);
        }
    }
    else
    {
        std::memcpy(buffer, t_buffer.c_str(), t_buffer.size());
    }

    detail::convert_string_to_c_locale(buffer, is.getloc());

    auto fmt {chars_format::general};
    const auto flags {is.flags()};
    if (flags & std::ios_base::scientific)
    {
        fmt = chars_format::scientific;
    }
    else if (flags & std::ios_base::hex)
    {
        fmt = chars_format::hex;
    }
    else if (flags & std::ios_base::fixed)
    {
        fmt = chars_format::fixed;
    }

    auto first {buffer};
    if (*first == '+')
    {
        // Having a leading + sign is legal in iostream, but not allowed with charconv
        // Pre-processing this case away helps support for both
        ++first;
    }

    auto r = from_chars(first, buffer + std::strlen(buffer), d, fmt);

    if (BOOST_DECIMAL_UNLIKELY(r.ec == std::errc::not_supported))
    {
        d = std::numeric_limits<DecimalType>::signaling_NaN();
    }
    else if (static_cast<int>(r.ec) == EINVAL)
    {
        errno = EINVAL;
    }

    // Put back unconsumed characters
    const auto consumed {static_cast<std::size_t>(r.ptr - buffer)};
    BOOST_DECIMAL_ASSERT(t_buffer_len >= consumed);
    const auto return_chars {static_cast<std::size_t>(t_buffer_len - consumed)};

    for (std::size_t i {}; i < return_chars; ++i)
    {
        is.putback(t_buffer[t_buffer_len - i - 1]);
    }

    return is;
}

namespace detail {

// showpoint: the general format keeps P significant digits with the trailing zeros, as printf %#g,
// and every format keeps the decimal point. hexfloat ignores the precision
template <BOOST_DECIMAL_DECIMAL_FLOATING_TYPE DecimalType>
auto to_chars_showpoint(char* first, char* last, const DecimalType& d, const chars_format fmt, const int local_precision) -> to_chars_result
{
    to_chars_result r {};
    if (fmt == chars_format::general && isfinite(d))
    {
        // The exponent after the rounding to P digits decides between the fixed and the scientific format
        const auto digits {local_precision == 0 ? 1 : local_precision};
        r = to_chars(first, last, d, chars_format::scientific, digits - 1);
        if (!r)
        {
            return r;
        }

        // The exponent is e+XX or e-XX at the end
        const auto exp_mark {static_cast<const char*>(std::memchr(first, 'e', static_cast<std::size_t>(r.ptr - first)))};
        int exp {};
        for (auto c {exp_mark + 2}; c != r.ptr; ++c)
        {
            exp = exp * 10 + (*c - '0');
        }
        exp = exp_mark[1] == '-' ? -exp : exp;
        if (exp >= -4 && exp < digits)
        {
            r = to_chars(first, last, d, chars_format::fixed, digits - 1 - exp);
        }
    }
    else if (fmt == chars_format::hex)
    {
        r = to_chars(first, last, d, fmt);
    }
    else
    {
        r = to_chars(first, last, d, fmt, local_precision);
    }

    if (r && isfinite(d) && std::memchr(first, '.', static_cast<std::size_t>(r.ptr - first)) == nullptr)
    {
        if (r.ptr == last)
        {
            return {last, std::errc::value_too_large};
        }
        auto point {static_cast<char*>(std::memchr(first, fmt == chars_format::hex ? 'p' : 'e', static_cast<std::size_t>(r.ptr - first)))};
        point = point == nullptr ? r.ptr : point;
        std::memmove(point + 1, point, static_cast<std::size_t>(r.ptr - point));
        *point = '.';
        ++r.ptr;
    }

    return r;
}

// Writes the C locale characters [first, last) as num_put does, for all the decimal types
template <typename charT, typename traits>
void put_chars(std::basic_ostream<charT, traits>& os, char* first, char* const last)
{
    const auto flags {os.flags()};
    if (flags & std::ios_base::uppercase)
    {
        detail::make_uppercase(first, last);
    }

    const char* str {first};
    const bool minus {str != last && *str == '-'};
    const bool plus {(flags & std::ios_base::showpos) && !minus};
    str += static_cast<int>(minus);

    // The numpunct of the stream groups the digits before the point as num_put does:
    // the last group size repeats, and a size of 0 or CHAR_MAX stops the grouping
    const auto loc {os.getloc()};
    const auto& np {std::use_facet<std::numpunct<charT>>(loc)};
    const auto& ct {std::use_facet<std::ctype<charT>>(loc)};
    const auto grouping {np.grouping()};
    std::size_t digits {};
    while (str + digits != last && str[digits] >= '0' && str[digits] <= '9')
    {
        ++digits;
    }
    std::size_t first_group {};
    auto separators {detail::count_separators(grouping, digits, first_group)};

    // Pad to the width as num_put does: internal puts the fill after the sign
    const auto total {static_cast<std::size_t>(last - str) + static_cast<std::size_t>(minus || plus) + separators};
    // As num_put does, the width resets before the output, so it also resets when the stream buffer throws
    const auto width {static_cast<std::size_t>(os.width() > 0 ? os.width() : 0)};
    os.width(0);
    auto padding {width > total ? width - total : 0U};
    const auto adjust {flags & std::ios_base::adjustfield};

    auto& sb {*os.rdbuf()};
    bool failed {false};
    const auto put = [&](const charT c) { failed = failed || traits::eq_int_type(sb.sputc(c), traits::eof()); };
    const auto put_padding = [&]
    {
        for (; padding > 0U && !failed; --padding)
        {
            put(os.fill());
        }
    };

    // A run is widened and written in blocks, and the output stops at the first failed write
    const auto put_run = [&](std::size_t n)
    {
        constexpr std::size_t block_size {64U};
        charT block[block_size];
        while (n > 0U && !failed)
        {
            const auto k {n < block_size ? n : block_size};
            ct.widen(str, str + k, block);
            failed = failed || sb.sputn(block, static_cast<std::streamsize>(k)) != static_cast<std::streamsize>(k);
            str += k;
            n -= k;
        }
    };

    if (adjust != std::ios_base::left && adjust != std::ios_base::internal)
    {
        put_padding();
    }
    if ((minus || plus) && !failed)
    {
        put(ct.widen(minus ? '-' : '+'));
    }
    if (adjust == std::ios_base::internal)
    {
        put_padding();
    }
    put_run(first_group);
    while (separators > 0U && !failed)
    {
        put(np.thousands_sep());
        put_run(detail::group_size(grouping, --separators, digits));
    }
    const auto point {failed ? nullptr : static_cast<const char*>(std::memchr(str, '.', static_cast<std::size_t>(last - str)))};
    if (point != nullptr)
    {
        put_run(static_cast<std::size_t>(point - str));
        if (!failed)
        {
            put(np.decimal_point());
        }
        ++str;
    }
    put_run(static_cast<std::size_t>(last - str));
    put_padding();

    if (failed)
    {
        os.setstate(std::ios_base::badbit);
    }
}

// Writes d as num_put writes a double
template <typename charT, typename traits, BOOST_DECIMAL_DECIMAL_FLOATING_TYPE DecimalType>
void put_decimal(std::basic_ostream<charT, traits>& os, const DecimalType& d)
{
    // The float field picks the format as for a double, so hexfloat is fixed | scientific
    chars_format fmt = chars_format::general;
    const auto flags {os.flags()};
    const auto float_field {flags & std::ios_base::floatfield};
    if (float_field == (std::ios_base::fixed | std::ios_base::scientific))
    {
        fmt = chars_format::hex;
    }
    else if (float_field == std::ios_base::scientific)
    {
        fmt = chars_format::scientific;
    }
    else if (float_field == std::ios_base::fixed)
    {
        fmt = chars_format::fixed;
    }

    // A negative precision means 6, as for printf. hexfloat, inf and NaN do not use the precision,
    // so it is 0 for them and their short text needs no large buffer
    constexpr std::streamsize max_precision {std::numeric_limits<int>::max() / 2};
    const auto stream_precision {os.precision() < 0 ? 6 : os.precision() < max_precision ? os.precision() : max_precision};
    const auto local_precision {fmt == chars_format::hex || !isfinite(d) ? 0 : static_cast<int>(stream_precision)};

    const auto print = [&](char* first, char* last)
    {
        if (flags & std::ios_base::showpoint)
        {
            return detail::to_chars_showpoint(first, last, d, fmt, local_precision);
        }

        // hexfloat ignores the precision
        return fmt == chars_format::hex ? to_chars(first, last, d, fmt) : to_chars(first, last, d, fmt, local_precision);
    };

    // Most values fit the local buffer. A longer one gets the length that to_chars needs at most,
    // where showpoint keeps the trailing zeros of the general format, as the scientific format does
    char local_buffer[512];
    std::unique_ptr<char[]> heap_buffer;
    char* buffer {local_buffer};
    auto r {print(local_buffer, local_buffer + sizeof(local_buffer))};
    if (r.ec == std::errc::value_too_large)
    {
        const auto sizing_fmt {fmt == chars_format::general && (flags & std::ios_base::showpoint) ? chars_format::scientific : fmt};
        const auto length {detail::to_chars_length(d, sizing_fmt, local_precision)};
        heap_buffer.reset(new char[length]);
        buffer = heap_buffer.get();
        r = print(buffer, buffer + length);
    }

    // The buffer sizes make this fail never, and a failure writes nothing and sets badbit
    if (BOOST_DECIMAL_UNLIKELY(!r))
    {
        os.setstate(std::ios_base::badbit);
        return;
    }

    detail::put_chars(os, buffer, r.ptr);
}

} // namespace detail

// 3.2.11 Formatted output
BOOST_DECIMAL_EXPORT template <typename charT, typename traits, BOOST_DECIMAL_DECIMAL_FLOATING_TYPE DecimalType>
auto operator<<(std::basic_ostream<charT, traits>& os, const DecimalType& d)
    -> std::enable_if_t<detail::is_decimal_floating_point_v<DecimalType>, std::basic_ostream<charT, traits>&>
{
    const typename std::basic_ostream<charT, traits>::sentry guard(os);
    if (guard)
    {
        #ifndef BOOST_DECIMAL_DISABLE_EXCEPTIONS
        try
        {
            detail::put_decimal(os, d);
        }
        catch (...)
        {
            // An exception during the output sets badbit, and goes to the caller only when badbit is in the
            // exception mask. setstate throws ios_base::failure for that mask, so the original exception is rethrown
            try
            {
                os.setstate(std::ios_base::badbit);
            }
            catch (const std::ios_base::failure&)
            {
            }
            if (os.exceptions() & std::ios_base::badbit)
            {
                throw;
            }
        }
        #else
        detail::put_decimal(os, d);
        #endif
    }

    return os;
}

} //namespace decimal
} //namespace boost

#endif // BOOST_DECIMAL_DISABLE_CLIB

#endif //BOOST_DECIMAL_DETAIL_IO_HPP
