// Copyright 2024 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_DECIMAL_CSTDIO_HPP
#define BOOST_DECIMAL_CSTDIO_HPP

#include <boost/decimal/decimal32_t.hpp>
#include <boost/decimal/decimal64_t.hpp>
#include <boost/decimal/decimal128_t.hpp>
#include <boost/decimal/decimal_fast32_t.hpp>
#include <boost/decimal/decimal_fast64_t.hpp>
#include <boost/decimal/decimal_fast128_t.hpp>
#include <boost/decimal/detail/config.hpp>
#include <boost/decimal/detail/locale_conversion.hpp>
#include <boost/decimal/detail/parser.hpp>
#include <boost/decimal/detail/concepts.hpp>
#include <boost/decimal/detail/attributes.hpp>
#include <boost/decimal/charconv.hpp>

#ifndef BOOST_DECIMAL_BUILD_MODULE
#include <memory>
#include <new>
#include <cerrno>
#include <climits>
#include <cstring>
#include <cstdio>
#include <initializer_list>
#include <type_traits>
#endif

#if !defined(BOOST_DECIMAL_DISABLE_CLIB)

// H is the type modifier used for decimal32_t
// D is the type modifier used for deciaml64
// DD is the type modifier used for decimal128_t

namespace boost {
namespace decimal {

namespace detail {

enum class decimal_type : unsigned
{
    decimal32_t = 1 << 0,
    decimal64_t = 1 << 1,
    decimal128_t = 1 << 2
};

// Internal use only
#ifdef __GNUC__
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wpadded"
#endif

struct parameters
{
    int precision;
    int width;
    chars_format fmt;
    decimal_type return_type;
    bool upper_case;
    bool left_align;
    bool zero_pad;
    bool overflow;
    char sign;
};

#ifdef __GNUC__
#  pragma GCC diagnostic pop
#endif

// A number above INT_MAX sets overflow, as printf fails for a width or a precision that an int cannot hold
inline auto parse_int(const char*& iter, bool& overflow) noexcept -> int
{
    int value {};
    while (is_integer_char(*iter))
    {
        const int digit {digit_from_char(*iter++)};
        if (value > (INT_MAX - digit) / 10)
        {
            overflow = true;
            value = INT_MAX;
        }
        else
        {
            value = value * 10 + digit;
        }
    }

    return value;
}

// Reads the conversion specification that starts at the '%' at iter, and moves iter past it.
// A missing or unknown conversion character gives the general format and stays in the text.
inline auto parse_format(const char*& iter) noexcept -> parameters
{
    parameters params {-1, 0, chars_format::general, decimal_type::decimal64_t, false, false, false, false, '\0'};

    for (++iter; ; ++iter)
    {
        if (*iter == '-')
        {
            params.left_align = true;
        }
        else if (*iter == '0')
        {
            params.zero_pad = true;
        }
        else if (*iter == '+' || (*iter == ' ' && params.sign == '\0'))
        {
            params.sign = *iter;
        }
        else if (*iter != ' ')
        {
            break;
        }
    }

    params.width = parse_int(iter, params.overflow);

    if (*iter == '.')
    {
        ++iter;
        params.precision = parse_int(iter, params.overflow);
    }

    // Now get the type specifier
    if (*iter == 'H')
    {
        params.return_type = decimal_type::decimal32_t;
        ++iter;
    }
    else if (*iter == 'D')
    {
        ++iter;
        if (*iter == 'D')
        {
            params.return_type = decimal_type::decimal128_t;
            ++iter;
        }
    }

    // And lastly the format
    switch (*iter)
    {
        case 'g':
        case 'G':
            break;
        case 'e':
        case 'E':
            params.fmt = chars_format::scientific;
            break;
        case 'f':
        case 'F':
            params.fmt = chars_format::fixed;
            break;
        case 'a':
        case 'A':
            params.fmt = chars_format::hex;
            break;
        default:
            return params;
    }

    params.upper_case = *iter < 'a';
    ++iter;

    return params;
}

// Prints v, which has the decimal type that the modifier of the conversion selects
template <typename DecimalType>
inline auto print_value(char* first, char* last, const DecimalType v, const parameters& params) noexcept -> to_chars_result
{
    // A missing precision is 6, but %a without a precision prints the exact value as printf does
    const auto r {params.precision < 0 && params.fmt == chars_format::hex ? to_chars(first, last, v, params.fmt)
                                                                         : to_chars(first, last, v, params.fmt, params.precision)};
    if (r && params.upper_case)
    {
        make_uppercase(first, r.ptr);
    }

    return r;
}

template <typename... T>
inline auto snprintf_impl(char* buffer, const std::size_t buf_size, const char* format, const T... values) noexcept
    #ifndef BOOST_DECIMAL_HAS_CONCEPTS
    -> std::enable_if_t<detail::conjunction_v<detail::is_decimal_floating_point<T>...>, int>
    #else
    -> int requires detail::conjunction_v<detail::is_decimal_floating_point<T>...>
    #endif
{
    if ((buffer == nullptr && buf_size != 0) || format == nullptr)
    {
        return -1;
    }

    const std::initializer_list<std::common_type_t<T...>> values_list {values...};
    auto value_iter = values_list.begin();

    // Like std::snprintf, count the full output but write only what fits before the null terminator
    std::size_t count {};
    const auto room = [&](const std::size_t n) noexcept
    {
        const auto available {count + 1U < buf_size ? buf_size - 1U - count : 0U};
        return n < available ? n : available;
    };
    const auto put = [&](const char c) noexcept
    {
        if (room(1U) != 0U)
        {
            buffer[count] = c;
        }
        ++count;
    };
    const auto put_fill = [&](const char c, const std::size_t n) noexcept
    {
        if (room(n) != 0U)
        {
            std::memset(buffer + count, c, room(n));
        }
        count += n;
    };
    const auto put_text = [&](const char* text, const std::size_t n) noexcept
    {
        if (room(n) != 0U)
        {
            std::memcpy(buffer + count, text, room(n));
        }
        count += n;
    };

    char local_buffer[256];
    std::unique_ptr<char[]> heap_buffer;
    int error {};

    for (const char* iter {format}; *iter != '\0' && error == 0; )
    {
        if (*iter != '%' || iter[1] == '%')
        {
            put(*iter);
            iter += *iter == '%' ? 2 : 1;
            continue;
        }

        const auto params {parse_format(iter)};
        if (params.overflow)
        {
            error = EOVERFLOW;
            break;
        }
        if (value_iter == values_list.end())
        {
            error = EINVAL;
            break;
        }

        // The modifier can narrow a finite value to inf, so the cast value sizes the buffer, decides
        // the padding and gets printed. The locale separators go only between the integer digits,
        // so the length of the integer part is enough room for them
        char* str {local_buffer};
        std::size_t str_size {};
        bool finite {};
        to_chars_result r {};
        const auto convert = [&](const auto v) noexcept
        {
            finite = isfinite(v);
            const auto max_length {to_chars_length(v, params.fmt, params.precision)};
            str_size = max_length + to_chars_length(v, chars_format::fixed, 0);
            if (str_size > sizeof(local_buffer))
            {
                heap_buffer.reset(new(std::nothrow) char[str_size]);
                if (heap_buffer == nullptr)
                {
                    error = ENOMEM;
                    return;
                }
                str = heap_buffer.get();
            }

            r = print_value(str, str + max_length, v, params);
            if (!r)
            {
                error = static_cast<int>(r.ec);
            }
        };

        const auto value {*value_iter++};
        switch (params.return_type)
        {
            case detail::decimal_type::decimal32_t:
                convert(static_cast<decimal32_t>(value));
                break;
            case detail::decimal_type::decimal64_t:
                convert(static_cast<decimal64_t>(value));
                break;
            default:
                convert(static_cast<decimal128_t>(value));
                break;
        }
        if (error != 0)
        {
            break;
        }

        *r.ptr = '\0';
        const auto offset {convert_pointer_pair_to_local_locale(str, str + str_size)};
        const char* const str_end {r.ptr + (offset == -1 ? 0 : offset)};

        const bool negative {*str == '-'};
        const char sign {negative ? '-' : params.sign};
        const char* digits {str + static_cast<int>(negative)};
        const auto length {static_cast<std::size_t>(str_end - digits) + static_cast<std::size_t>(sign != '\0')};
        auto padding {params.width > 0 && length < static_cast<std::size_t>(params.width) ? static_cast<std::size_t>(params.width) - length : 0U};

        // The full count must fit in the int that snprintf returns, so the count cannot wrap
        if (count > static_cast<std::size_t>(INT_MAX) || length + padding > static_cast<std::size_t>(INT_MAX) - count)
        {
            error = EOVERFLOW;
            break;
        }

        // A zero pad goes after the sign, and inf or nan pads with spaces
        const bool zero_pad {params.zero_pad && !params.left_align && finite};
        if (!params.left_align && !zero_pad)
        {
            put_fill(' ', padding);
            padding = 0U;
        }
        if (sign != '\0')
        {
            put(sign);
        }
        if (zero_pad)
        {
            put_fill('0', padding);
            padding = 0U;
        }
        put_text(digits, static_cast<std::size_t>(str_end - digits));
        put_fill(' ', padding);
    }

    if (buf_size != 0)
    {
        buffer[count < buf_size ? count : buf_size - 1] = '\0';
    }

    if (error == 0 && count > static_cast<std::size_t>(INT_MAX))
    {
        error = EOVERFLOW;
    }

    if (error != 0)
    {
        errno = error;
        return -1;
    }

    return static_cast<int>(count);
}

} // namespace detail

template <typename... T>
inline auto snprintf(char* buffer, const std::size_t buf_size, const char* format, const T... values) noexcept
    #ifndef BOOST_DECIMAL_HAS_CONCEPTS
    -> std::enable_if_t<detail::conjunction_v<detail::is_decimal_floating_point<T>...>, int>
    #else
    -> int requires detail::conjunction_v<detail::is_decimal_floating_point<T>...>
    #endif
{
    return detail::snprintf_impl(buffer, buf_size, format, values...);
}

template <typename... T>
inline auto fprintf(std::FILE* buffer, const char* format, const T... values) noexcept
    #ifndef BOOST_DECIMAL_HAS_CONCEPTS
    -> std::enable_if_t<detail::conjunction_v<detail::is_decimal_floating_point<T>...>, int>
    #else
    -> int requires detail::conjunction_v<detail::is_decimal_floating_point<T>...>
    #endif
{
    char local_buffer[1024];
    const auto bytes {detail::snprintf_impl(local_buffer, sizeof(local_buffer), format, values...)};
    if (bytes < 0)
    {
        return bytes;
    }

    const char* str {local_buffer};
    std::unique_ptr<char[]> heap_buffer;
    const auto size {static_cast<std::size_t>(bytes)};
    if (size >= sizeof(local_buffer))
    {
        heap_buffer.reset(new(std::nothrow) char[size + 1U]);
        if (heap_buffer == nullptr)
        {
            errno = ENOMEM;
            return -1;
        }
        // The second pass can fail to get its own buffers
        if (detail::snprintf_impl(heap_buffer.get(), size + 1U, format, values...) != bytes)
        {
            return -1;
        }
        str = heap_buffer.get();
    }

    return std::fwrite(str, sizeof(char), size, buffer) == size ? bytes : -1;
}

template <typename... T>
inline auto printf(const char* format, const T... values) noexcept
    #ifndef BOOST_DECIMAL_HAS_CONCEPTS
    -> std::enable_if_t<detail::conjunction_v<detail::is_decimal_floating_point<T>...>, int>
    #else
    -> int requires detail::conjunction_v<detail::is_decimal_floating_point<T>...>
    #endif
{
    return fprintf(stdout, format, values...);
}

} // namespace decimal
} // namespace boost

#endif // #if !defined(BOOST_DECIMAL_DISABLE_CLIB)

#endif //BOOST_DECIMAL_CSTDIO_HPP
