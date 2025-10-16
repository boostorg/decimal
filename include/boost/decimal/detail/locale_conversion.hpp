// Copyright 2024 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_DECIMAL_DETAIL_LOCALE_CONVERSION_HPP
#define BOOST_DECIMAL_DETAIL_LOCALE_CONVERSION_HPP

#ifndef BOOST_DECIMAL_BUILD_MODULE
#include <locale>
#include <clocale>
#include <cstring>
#endif

namespace boost {
namespace decimal {
namespace detail {

inline void convert_string_to_c_locale(char* buffer) noexcept
{
    const auto* lconv {std::localeconv()};
    const auto locale_decimal_point {*lconv->decimal_point};
    auto locale_thousands_sep {*lconv->thousands_sep};
    if (locale_thousands_sep == -30)
    {
        // Locales like french use a space, but thousands_sep does not give a space character
        locale_thousands_sep = ' ';
    }
    const bool has_grouping {lconv->grouping && lconv->grouping[0] > 0};

    // Remove thousands separator if it exists and grouping is enabled
    if (has_grouping && locale_thousands_sep != '\0')
    {
        // Find the decimal point first to know where the integer part ends
        const auto decimal_pos {std::strchr(buffer, static_cast<int>(locale_decimal_point))};
        const auto int_end {decimal_pos ? decimal_pos : (buffer + std::strlen(buffer))};

        // Find the start of the number to include skipping sign
        auto start {buffer};
        if (*start == '-' || *start == '+')
        {
            ++start;
        }

        // Only remove thousands separators from the integer part
        auto read {start};
        auto write {start};

        while (read < int_end)
        {
            const auto ch = *read;
            if (ch != locale_thousands_sep)
            {
                *write++ = *read;
            }
            ++read;
        }

        // Copy the rest of the string (decimal point and fractional part)
        while (*read != '\0')
        {
            *write++ = *read++;
        }
        *write = '\0';
    }

    if (locale_decimal_point != '.')
    {
        const auto p {std::strchr(buffer, static_cast<int>(locale_decimal_point))};
        if (p != nullptr)
        {
            *p = '.';
        }
    }
}

inline void convert_string_to_local_locale(char* buffer) noexcept
{
    const auto locale_decimal_point = *std::localeconv()->decimal_point;
    if (locale_decimal_point != '.')
    {
        auto p = std::strchr(buffer, static_cast<int>('.'));
        if (p != nullptr)
        {
            *p = locale_decimal_point;
        }
    }
}

inline void convert_pointer_pair_to_local_locale(char* first, const char* last) noexcept
{
    const auto locale_decimal_point = *std::localeconv()->decimal_point;
    if (locale_decimal_point != '.')
    {
        while (first != last)
        {
            if (*first == '.')
            {
                *first = locale_decimal_point;
            }

            ++first;
        }
    }
}

} //namespace detail
} //namespace decimal
} //namespace boost

#endif //BOOST_DECIMAL_DETAIL_LOCALE_CONVERSION_HPP
