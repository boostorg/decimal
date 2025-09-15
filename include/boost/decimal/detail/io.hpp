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
#include <cinttypes>
#include <limits>
#include <ios>
#include <iostream>
#include <system_error>
#include <type_traits>
#endif

namespace boost {
namespace decimal {

// 3.2.10 Formatted input:
BOOST_DECIMAL_EXPORT template <typename charT, typename traits, BOOST_DECIMAL_DECIMAL_FLOATING_TYPE DecimalType>
auto operator>>(std::basic_istream<charT, traits>& is, DecimalType& d)
    -> std::enable_if_t<detail::is_decimal_floating_point_v<DecimalType>, std::basic_istream<charT, traits>&>
{
    charT t_buffer[1024] {}; // What should be an unreasonably high maximum
    is >> std::setw(1023) >> t_buffer;

    const auto t_buffer_len {std::char_traits<charT>::length(t_buffer)};

    char buffer[1024] {};

    BOOST_DECIMAL_IF_CONSTEXPR (!std::is_same<charT, char>::value)
    {
        auto first = buffer;
        auto t_first = t_buffer;
        auto t_buffer_end = t_buffer + std::strlen(t_buffer);

        while (t_first != t_buffer_end)
        {
            *first++ = static_cast<char>(*t_first++);
        }
    }
    else
    {
        std::memcpy(buffer, t_buffer, sizeof(t_buffer));
    }

    detail::convert_string_to_c_locale(buffer);

    chars_format fmt = chars_format::general;
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
    std::size_t offset {};
    if (*first == '+')
    {
        // Having a leading + sign is legal in iostream, but not allowed with charconv
        // Pre-processing this case away helps support for both
        ++first;
        ++offset;
    }

    auto r = from_chars(first, buffer + std::strlen(buffer), d, fmt);

    if (BOOST_DECIMAL_UNLIKELY(r.ec == std::errc::not_supported))
    {
        d = std::numeric_limits<DecimalType>::signaling_NaN(); // LCOV_EXCL_LINE
    }
    else if (static_cast<int>(r.ec) == EINVAL)
    {
        errno = EINVAL;
    }

    // Put back unconsumed characters
    const auto consumed {static_cast<std::size_t>(r.ptr - buffer) + offset};
    const auto return_chars {static_cast<std::size_t>(t_buffer_len - consumed)};

    for (std::size_t i {}; i < return_chars; ++i)
    {
        is.putback(t_buffer[t_buffer_len - i - 1]);
    }

    return is;
}

// GCC UBSAN warns of format truncation from the constexpr calculation of the format
// This warning was added in GCC 7.1
#if defined(__GNUC__) && __GNUC__ >= 7
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wformat-truncation"
#endif

// 3.2.11 Formatted output
BOOST_DECIMAL_EXPORT template <typename charT, typename traits, BOOST_DECIMAL_DECIMAL_FLOATING_TYPE DecimalType>
auto operator<<(std::basic_ostream<charT, traits>& os, const DecimalType& d)
    -> std::enable_if_t<detail::is_decimal_floating_point_v<DecimalType>, std::basic_ostream<charT, traits>&>
{
    chars_format fmt = chars_format::general;
    const auto flags {os.flags()};
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

    auto precision {os.precision()};
    if (precision > std::numeric_limits<DecimalType>::digits10)
    {
        precision = std::numeric_limits<DecimalType>::digits10;
    }

    char buffer[1024U] {};
    auto r = to_chars(buffer, buffer + sizeof(buffer), d, fmt, static_cast<int>(precision));

    if (BOOST_DECIMAL_UNLIKELY(!r))
    {
        errno = static_cast<int>(r.ec); // LCOV_EXCL_LINE
    }

    *r.ptr = '\0';

    detail::convert_string_to_local_locale(buffer);

    BOOST_DECIMAL_IF_CONSTEXPR (!std::is_same<charT, char>::value)
    {
        charT t_buffer[1024U] {};

        auto first = buffer;
        auto t_first = t_buffer;
        while (first != r.ptr)
        {
            *t_first++ = static_cast<charT>(*first++);
        }

        os << t_buffer;
    }
    else
    {
        os << buffer;
    }

    return os;
}

#if defined(__GNUC__) && __GNUC__ >= 7
#  pragma GCC diagnostic pop
#endif

} //namespace decimal
} //namespace boost

#endif // BOOST_DECIMAL_DISABLE_CLIB

#endif //BOOST_DECIMAL_DETAIL_IO_HPP
