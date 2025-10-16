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

inline void convert_pointer_pair_to_local_locale(char* first, char* last) noexcept
{
    const auto* lconv {std::localeconv()};
    const auto locale_decimal_point {*lconv->decimal_point};
    auto locale_thousands_sep {*lconv->thousands_sep};
    if (locale_thousands_sep == -30)
    {
        locale_thousands_sep = ' ';
    }
    const bool has_grouping {lconv->grouping && lconv->grouping[0] > 0};
    const int grouping_size {has_grouping ? lconv->grouping[0] : 0};

    // Find the start of the number (skip sign if present)
    char* start = first;
    if (start < last && (*start == '-' || *start == '+'))
    {
        ++start;
    }

    // Find decimal point position
    char* decimal_pos {nullptr};
    for (char* p = start; p < last; ++p)
    {
        if (*p == '.')
        {
            decimal_pos = p;
            *decimal_pos = locale_decimal_point;
            break;
        }
    }

    const auto int_end {decimal_pos != nullptr ? decimal_pos : last};
    const auto int_digits {static_cast<int>(int_end - start)};

    // Calculate how many separators we need
    int num_separators {};
    if (has_grouping && locale_thousands_sep != '\0' && int_digits > 0)
    {
        if (int_digits > grouping_size)
        {
            num_separators = (int_digits - 1) / grouping_size;
        }
    }

    // If we need to add separators, shift content and insert them
    if (num_separators > 0)
    {
        const auto original_length {static_cast<int>(last - first)};
        const auto new_length {original_length + num_separators};

        // Shift everything after the integer part to make room
        // Work backwards to avoid overwriting
        auto old_pos {last - 1};
        auto new_pos {first + new_length - 1};

        // Copy from end back to the end of integer part
        while (old_pos >= int_end)
        {
            *new_pos-- = *old_pos--;
        }

        int digit_count {};
        old_pos = int_end - 1;

        while (old_pos >= start)
        {
            *new_pos-- = *old_pos--;
            ++digit_count;

            // Insert separator after every grouping_size digits (but not at the start)
            if (digit_count % grouping_size == 0 && old_pos >= start)
            {
                *new_pos-- = locale_thousands_sep;
            }
        }
    }
}

inline void convert_string_to_local_locale(char* buffer) noexcept
{
    return convert_pointer_pair_to_local_locale(buffer, buffer + std::strlen(buffer));
}

} //namespace detail
} //namespace decimal
} //namespace boost

#endif //BOOST_DECIMAL_DETAIL_LOCALE_CONVERSION_HPP
