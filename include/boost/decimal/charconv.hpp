// Copyright 2024 - 2025 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_DECIMAL_CHARCONV_HPP
#define BOOST_DECIMAL_CHARCONV_HPP

#include <boost/decimal/decimal32_t.hpp>
#include <boost/decimal/decimal64_t.hpp>
#include <boost/decimal/decimal128_t.hpp>
#include <boost/decimal/decimal_fast32_t.hpp>
#include <boost/decimal/decimal_fast64_t.hpp>
#include <boost/decimal/decimal_fast128_t.hpp>
#include <boost/decimal/detail/config.hpp>
#include <boost/decimal/detail/parser.hpp>
#include <boost/decimal/detail/utilities.hpp>
#include <boost/decimal/detail/int128.hpp>
#include <boost/decimal/detail/from_chars_result.hpp>
#include <boost/decimal/detail/chars_format.hpp>
#include <boost/decimal/detail/concepts.hpp>
#include <boost/decimal/detail/to_chars_result.hpp>
#include <boost/decimal/detail/to_chars_integer_impl.hpp>
#include <boost/decimal/detail/buffer_sizing.hpp>
#include <boost/decimal/detail/cmath/frexp10.hpp>
#include <boost/decimal/detail/attributes.hpp>
#include <boost/decimal/detail/countl.hpp>
#include <boost/decimal/detail/remove_trailing_zeros.hpp>
#include <boost/decimal/detail/promotion.hpp>
#include <boost/decimal/detail/write_payload.hpp>
#include <boost/decimal/detail/formatting_limits.hpp>
#include <boost/decimal/detail/from_chars_impl.hpp>

#ifndef BOOST_DECIMAL_BUILD_MODULE
#include <cstdint>
#include <limits>
#include <system_error>
#include <type_traits>
#endif

#if !defined(BOOST_DECIMAL_DISABLE_CLIB)

#ifndef BOOST_DECIMAL_BUILD_MODULE
#include <string>
#endif

namespace boost {
namespace decimal {

// ---------------------------------------------------------------------------------------------------------------------
// from_chars
// ---------------------------------------------------------------------------------------------------------------------

BOOST_DECIMAL_EXPORT template <BOOST_DECIMAL_DECIMAL_FLOATING_TYPE TargetDecimalType>
BOOST_DECIMAL_CUDA_CONSTEXPR auto from_chars(const char* first, const char* last, TargetDecimalType& value, const chars_format fmt = chars_format::general) noexcept -> from_chars_result
{
    return detail::from_chars_general_impl(first, last, value, fmt);
}

#ifndef BOOST_DECIMAL_HAS_STD_STRING_VIEW

BOOST_DECIMAL_EXPORT template <BOOST_DECIMAL_DECIMAL_FLOATING_TYPE TargetDecimalType>
constexpr auto from_chars(const std::string& str, TargetDecimalType& value, const chars_format fmt = chars_format::general) noexcept -> from_chars_result
{
    return detail::from_chars_general_impl(str.data(), str.data() + str.size(), value, fmt);
}

#else

BOOST_DECIMAL_EXPORT template <BOOST_DECIMAL_DECIMAL_FLOATING_TYPE TargetDecimalType>
constexpr auto from_chars(std::string_view str, TargetDecimalType& value, chars_format fmt = chars_format::general) noexcept -> from_chars_result
{
    return detail::from_chars_general_impl(str.data(), str.data() + str.size(), value, fmt);
}

#endif

#ifdef BOOST_DECIMAL_HAS_STD_CHARCONV
BOOST_DECIMAL_EXPORT template <typename DecimalType>
constexpr auto from_chars(const char* first, const char* last, DecimalType& value, std::chars_format fmt) noexcept
    BOOST_DECIMAL_REQUIRES_RETURN(detail::is_decimal_floating_point_v, DecimalType, std::from_chars_result)
{
    from_chars_result boost_r {};
    switch (fmt)
    {
        case std::chars_format::scientific:
            boost_r = from_chars(first, last, value, chars_format::scientific);
            break;
        case std::chars_format::fixed:
            boost_r = from_chars(first, last, value, chars_format::fixed);
            break;
        case std::chars_format::hex:
            boost_r = from_chars(first, last, value, chars_format::hex);
            break;
        case std::chars_format::general:
            boost_r = from_chars(first, last, value, chars_format::general);
            break;
        // LCOV_EXCL_START
        default:
            BOOST_DECIMAL_UNREACHABLE;
        // LCOV_EXCL_STOP
    }

    return std::from_chars_result {boost_r.ptr, boost_r.ec};
}

BOOST_DECIMAL_EXPORT template <typename DecimalType>
constexpr auto from_chars(std::string_view str, DecimalType& value, std::chars_format fmt) noexcept
    BOOST_DECIMAL_REQUIRES_RETURN(detail::is_decimal_floating_point_v, DecimalType, std::from_chars_result)
{
    return from_chars(str.data(), str.data() + str.size(), value, fmt);
}
#endif

// ---------------------------------------------------------------------------------------------------------------------
// to_chars and implementation
// ---------------------------------------------------------------------------------------------------------------------

namespace detail {

template <BOOST_DECIMAL_DECIMAL_FLOATING_TYPE TargetDecimalType>
BOOST_DECIMAL_CUDA_CONSTEXPR auto to_chars_nonfinite(char* first, char* last, const TargetDecimalType& value, const int fp, const chars_format fmt, const int local_precision) noexcept -> to_chars_result
{
    const auto buffer_len = last - first;

    switch (fp)
    {
        case FP_INFINITE:
            if (buffer_len >= 3)
            {
                boost::decimal::detail::memcpy(first, "inf", 3U);
                return {first + 3U, std::errc()};
            }

            return {last, std::errc::value_too_large};
        case FP_ZERO:
            // A minus sign can take the last character
            if (buffer_len < 1)
            {
                return {last, std::errc::value_too_large};
            }

            if (fmt == chars_format::general)
            {
                *first++ = '0';
                return {first, std::errc()};
            }
            else if (fmt == chars_format::hex || fmt == chars_format::scientific)
            {
                // 0e+00, or 0. with the precision zeros and e+00
                if (local_precision <= 0 ? buffer_len >= 5 : buffer_len - 6 >= local_precision)
                {
                    if (local_precision <= 0)
                    {
                        *first++ = '0';
                    }
                    else
                    {
                        boost::decimal::detail::memcpy(first, "0.0", 3U);
                        first += 3U;

                        if (local_precision != 1)
                        {
                            boost::decimal::detail::memset(first, '0', static_cast<std::size_t>(local_precision - 1));
                            first += local_precision - 1;
                        }
                    }

                    if (fmt == chars_format::hex)
                    {
                        *first++ = 'p';
                    }
                    else
                    {
                        *first++ = 'e';
                    }

                    boost::decimal::detail::memcpy(first, "+00", 3U);
                    return {first + 3U, std::errc()};
                }
            }
            else
            {
                if (local_precision == -1 || local_precision == 0)
                {
                    *first++ = '0';
                    return {first, std::errc()};
                }
                else if (buffer_len - 2 >= local_precision)
                {
                    boost::decimal::detail::memcpy(first, "0.0", 3U);
                    first += 3U;

                    if (local_precision > 1)
                    {
                        boost::decimal::detail::memset(first, '0', static_cast<std::size_t>(local_precision - 1));
                        first += local_precision - 1;
                    }

                    return {first, std::errc()};
                }
            }

            return {last, std::errc::value_too_large};
        case FP_NAN:
            if (issignaling(value) && buffer_len >= 9)
            {
                boost::decimal::detail::memcpy(first, "nan(snan)", 9U);
                return {first + 9U, std::errc()};
            }
            else if (signbit(value) && buffer_len >= 8)
            {
                boost::decimal::detail::memcpy(first, "nan(ind)", 8U);
                return {first + 8U, std::errc()};
            }
            else if (buffer_len >= 3)
            {
                boost::decimal::detail::memcpy(first, "nan", 3U);
                return {first + 3U, std::errc()};
            }

            return {last, std::errc::value_too_large};
        // LCOV_EXCL_START
        default:
            BOOST_DECIMAL_UNREACHABLE;
        // LCOV_EXCL_STOP
    }
}

template <BOOST_DECIMAL_DECIMAL_FLOATING_TYPE TargetDecimalType>
BOOST_DECIMAL_CUDA_CONSTEXPR auto to_chars_scientific_impl(char* first, char* last, const TargetDecimalType& value, const chars_format fmt) noexcept -> to_chars_result
{
    bool is_neg {false};
    if (signbit(value))
    {
        *first++ = '-';
        is_neg = true;
    }

    const auto fp = fpclassify(value);
    if (!(fp == FP_NORMAL || fp == FP_SUBNORMAL))
    {
        return to_chars_nonfinite(first, last, value, fp, fmt, -1);
    }

    const auto buffer_size {last - first};
    const auto real_precision {get_real_precision<TargetDecimalType>()};

    // Dummy check the bounds
    if (BOOST_DECIMAL_UNLIKELY(buffer_size < real_precision))
    {
        return {last, std::errc::value_too_large};
    }

    using uint_type = std::conditional_t<(std::numeric_limits<typename TargetDecimalType::significand_type>::digits >
                                              std::numeric_limits<std::uint64_t>::digits),
                                              int128::uint128_t, std::uint64_t>;

    // Need to offset the exp for the fact that it's not 123e+2, it's 1.23e+4
    const auto components {value.to_components()};
    auto r = to_chars_integer_impl(first + 1, last, static_cast<uint_type>(components.sig));

    // Only real reason we will hit this is a buffer overflow,
    // which we have already checked for
    if (BOOST_DECIMAL_UNLIKELY(!r))
    {
        return r;
    }

    auto current_digits {r.ptr - (first + 1)};
    auto exp {components.exp + current_digits - 1};

    // Any trailing zeros can be removed
    // This is faster than stripping them from the normalized number
    --r.ptr;
    while (*r.ptr == '0')
    {
        --r.ptr;
        --current_digits;
    }
    ++r.ptr;

    // Make sure the result will fit in the buffer before continuing progress
    const auto total_length {total_buffer_length<TargetDecimalType>(static_cast<int>(current_digits), exp, is_neg)};
    if (BOOST_DECIMAL_UNLIKELY(total_length > buffer_size))
    {
        return {last, std::errc::value_too_large};
    }

    // Insert our decimal point (or don't in the 1 digit case)
    *first = *(first + 1);
    if (BOOST_DECIMAL_LIKELY(current_digits != 1))
    {
        *(first + 1) = '.';
    }
    else
    {
        --r.ptr;
    }
    first = r.ptr;

    *first++ = 'e';
    if (exp >= 0)
    {
        *first++ = '+';
    }
    else
    {
        *first++ = '-';
        exp = -exp;
    }

    // Need at least two digits e.g. e-09
    if (exp < 10)
    {
        *first++ = '0';
    }

    const auto exp_r {to_chars_integer_impl(first, last, exp)};

    if (BOOST_DECIMAL_UNLIKELY(!exp_r))
    {
        return exp_r;
    }

    return {exp_r.ptr, std::errc{}};
}

// Rounds sig / 10^shift, with shift > 0, to an integer in the current mode. The digits below
// the first dropped one go into the sticky bit, so a dropped 5 with more digits is not a tie.
template <BOOST_DECIMAL_DECIMAL_FLOATING_TYPE TargetDecimalType, typename T>
BOOST_DECIMAL_CUDA_CONSTEXPR auto to_chars_round(T sig, int shift, const bool is_neg) noexcept -> T
{
    bool sticky {false};
    if (shift > std::numeric_limits<T>::digits10)
    {
        // All digits are below the rounding digit, which is then 0
        sticky = sig != 0U;
        sig = 0U;
        shift = 1;
    }

    if (shift > 1)
    {
        const auto power {pow10(static_cast<T>(shift - 1))};
        sticky = sig % power != 0U;
        sig /= power;
    }

    fenv_round<TargetDecimalType>(sig, is_neg, sticky);
    return sig;
}

// Fixed format with fraction_digits digits after the point: round(|x| * 10^fraction_digits)
// in the current mode, with the point before its last fraction_digits digits
template <BOOST_DECIMAL_DECIMAL_FLOATING_TYPE TargetDecimalType>
BOOST_DECIMAL_CUDA_CONSTEXPR auto to_chars_fixed_precision(char* first, char* last, const TargetDecimalType& value, const int fraction_digits) noexcept -> to_chars_result
{
    using uint_type = std::conditional_t<(std::numeric_limits<typename TargetDecimalType::significand_type>::digits >
                                          std::numeric_limits<std::uint64_t>::digits),
                                          int128::uint128_t, std::uint64_t>;

    int exp {};
    auto sig {static_cast<uint_type>(frexp10(value, &exp))};

    // |x| * 10^fraction_digits is sig * 10^zeros. The lengths are 64-bit, because a precision
    // near INT_MAX and the exponent overflow an int
    std::int64_t zeros {static_cast<std::int64_t>(exp) + fraction_digits};
    if (zeros < 0)
    {
        sig = to_chars_round<TargetDecimalType>(sig, static_cast<int>(-zeros), signbit(value));
        zeros = 0;
    }

    char digits[64] {};
    const auto r {to_chars_integer_impl<uint_type>(digits, digits + sizeof(digits), sig)};
    const std::int64_t num_digits {r.ptr - digits};
    const auto total_digits {num_digits + zeros};
    const auto leading_zeros {total_digits > fraction_digits ? 0 : static_cast<std::int64_t>(fraction_digits) + 1 - total_digits};
    const auto length {leading_zeros + total_digits + (fraction_digits > 0 ? 1 : 0)};

    if (last - first < length)
    {
        return {last, std::errc::value_too_large};
    }

    // Write the digits with the leading and trailing zeros, and put the point in front of the last fraction_digits ones
    const auto integer_length {leading_zeros + total_digits - fraction_digits};
    for (std::int64_t i {}; i < leading_zeros + total_digits; ++i)
    {
        if (i == integer_length)
        {
            *first++ = '.';
        }

        const auto digit_index {i - leading_zeros};
        *first++ = digit_index < 0 || digit_index >= num_digits ? '0' : digits[digit_index];
    }

    return {first, std::errc()};
}

template <BOOST_DECIMAL_DECIMAL_FLOATING_TYPE TargetDecimalType>
BOOST_DECIMAL_CUDA_CONSTEXPR auto to_chars_scientific_impl(char* first, char* last, const TargetDecimalType& value, const chars_format fmt, const int local_precision) noexcept -> to_chars_result
{
    if (signbit(value))
    {
        if (first == last)
        {
            return {last, std::errc::value_too_large};
        }
        *first++ = '-';
    }

    const auto fp = fpclassify(value);
    if (!(fp == FP_NORMAL || fp == FP_SUBNORMAL))
    {
        return to_chars_nonfinite(first, last, value, fp, fmt, local_precision);
    }

    // The shortest finite result is 1e+00. The general format comes here only for zero, inf and nan
    if (last - first < 5)
    {
        return {last, std::errc::value_too_large};
    }

    int exp {};
    auto significand {frexp10(value, &exp)};

    using uint_type = std::conditional_t<(std::numeric_limits<typename TargetDecimalType::significand_type>::digits >
                                          std::numeric_limits<std::uint64_t>::digits),
                                          int128::uint128_t, std::uint64_t>;

    // Since frexp10 normalizes the value, we by default know the number of digits in the significand
    auto significand_digits = std::numeric_limits<TargetDecimalType>::digits;
    exp += significand_digits - 1;
    bool append_zeros = false;

    if (local_precision != -1)
    {
        if (significand_digits - 1 > local_precision)
        {
            // Round to local_precision + 1 digits in the current mode. A carry gives one more
            // digit: 9.96 with two digits after the point is 1.00 one decade up.
            significand = to_chars_round<TargetDecimalType>(significand, significand_digits - (local_precision + 1), signbit(value));
            significand_digits = local_precision + 1;
            if (significand == pow10(static_cast<typename TargetDecimalType::significand_type>(significand_digits)))
            {
                significand /= 10U;
                ++exp;
            }
        }
        else if (significand_digits < local_precision)
        {
            append_zeros = true;
        }
    }

    // Offset the value of first by 1 so that we can copy the leading digit and insert a decimal point
    auto r = to_chars_integer_impl<uint_type>(first + 1, last, significand);

    // Only real reason we will hit this is a buffer overflow
    if (BOOST_DECIMAL_UNLIKELY(!r))
    {
        return r;
    }

    const auto current_digits = r.ptr - (first + 1) - 1;

    if (current_digits < local_precision)
    {
        append_zeros = true;
    }

    if (append_zeros)
    {
        const auto zeros_inserted {static_cast<std::size_t>(local_precision - current_digits)};

        if (static_cast<std::size_t>(last - r.ptr) < zeros_inserted)
        {
            return {last, std::errc::value_too_large};
        }

        boost::decimal::detail::memset(r.ptr, '0', zeros_inserted);
        r.ptr += zeros_inserted;
    }

    // Insert our decimal point
    *first = *(first + 1);
    *(first + 1) = '.';
    first = r.ptr;

    if (local_precision == 0)
    {
        --first;
    }

    // The exponent is e, its sign and at least two digits
    const int abs_exp { (exp < 0) ? -exp : exp };
    if (last - first < (abs_exp < 100 ? 4 : abs_exp < 1000 ? 5 : 6))
    {
        return {last, std::errc::value_too_large};
    }

    // Insert the exponent character
    *first++ = 'e';

    if (exp < 0)
    {
        *first++ = '-';
    }
    else
    {
        *first++ = '+';
    }

    // Always give 2 digits in the exp (ex. 2.0e+09)
    if (abs_exp <= 9)
    {
        *first++ = '0';
    }

    r = to_chars_integer_impl<int>(first, last, abs_exp);
    if (BOOST_DECIMAL_UNLIKELY(!r))
    {
        return r;
    }

    return {r.ptr, std::errc()};
}

template <BOOST_DECIMAL_DECIMAL_FLOATING_TYPE TargetDecimalType>
BOOST_DECIMAL_CUDA_CONSTEXPR auto to_chars_fixed_impl(char* first, char* last, const TargetDecimalType& value, const chars_format fmt) noexcept -> to_chars_result
{
    const auto buffer_size {last - first};
    const auto real_precision {get_real_precision<TargetDecimalType>()};

    // Dummy check the bounds
    if (BOOST_DECIMAL_UNLIKELY(buffer_size < real_precision))
    {
        return {last, std::errc::value_too_large};
    }

    char* current = first;
    if (signbit(value))
    {
        *current++ = '-';
    }

    const auto fp = fpclassify(value);
    if (!(fp == FP_NORMAL || fp == FP_SUBNORMAL))
    {
        return to_chars_nonfinite(current, last, value, fp, fmt, -1);
    }

    auto components {value.to_components()};
    if (components.sig % 10U == 0U)
    {
        const auto zeros_removal_result {remove_trailing_zeros(components.sig)};
        components.sig = zeros_removal_result.trimmed_number;
        components.exp += static_cast<int>(zeros_removal_result.number_of_removed_zeros);
    }

    const auto r {to_chars_integer_impl(current, last, components.sig)};

    if (BOOST_DECIMAL_UNLIKELY(!r))
    {
        return r;
    }

    const auto num_digits {r.ptr - current};
    const auto exp {components.exp};
    const auto abs_exp {exp < 0 ? -exp : exp};

    // There are now three cases that we need to handle
    // 1) We need to append trailing zeros e.g. 12345000000
    // 2) We need to insert the decimal point 12.345
    // 3) We need to append leading zeros e.g 0.0000012345

    if (exp >= 0)
    {
        if (BOOST_DECIMAL_UNLIKELY(buffer_size < (current - first) + num_digits + exp))
        {
            return {last, std::errc::value_too_large};
        }

        detail::memset(r.ptr, '0', static_cast<std::size_t>(exp));
        return {r.ptr + exp, std::errc{}};
    }
    else if (abs_exp < num_digits)
    {
        if (BOOST_DECIMAL_UNLIKELY(buffer_size < (current - first) + num_digits + 1))
        {
            return {last, std::errc::value_too_large};
        }

        const auto decimal_pos {num_digits - abs_exp};
        detail::memmove(current + decimal_pos + 1, current + decimal_pos, static_cast<std::size_t>(abs_exp));
        current[decimal_pos] = '.';

        return {r.ptr + 1, std::errc{}};
    }
    else
    {
        const auto leading_zeros {abs_exp - num_digits};
        if (BOOST_DECIMAL_UNLIKELY(buffer_size < (current - first) + 2 + leading_zeros + num_digits))
        {
            return {last, std::errc::value_too_large};
        }

        detail::memmove(current + 2 + leading_zeros, current, static_cast<std::size_t>(num_digits));
        current[0] = '0';
        current[1] = '.';
        detail::memset(current + 2, '0', static_cast<std::size_t>(leading_zeros));

        return {current + 2 + leading_zeros + num_digits, std::errc{}};
    }

    BOOST_DECIMAL_UNREACHABLE; // LCOV_EXCL_LINE
}

template <BOOST_DECIMAL_DECIMAL_FLOATING_TYPE TargetDecimalType>
BOOST_DECIMAL_CUDA_CONSTEXPR auto to_chars_fixed_impl(char* first, char* last, const TargetDecimalType& value, const chars_format fmt, const int local_precision) noexcept -> to_chars_result
{
    // The paths below check the room that they need, and inf and nan do not use the precision
    if (signbit(value))
    {
        if (first == last)
        {
            return {last, std::errc::value_too_large};
        }
        *first++ = '-';
    }

    const auto fp = fpclassify(value);
    if (!(fp == FP_NORMAL || fp == FP_SUBNORMAL))
    {
        return to_chars_nonfinite(first, last, value, fp, fmt, local_precision);
    }

    return to_chars_fixed_precision(first, last, value, local_precision);
}

// The general format with a precision is printf %g: P significant digits (1 for 0), fixed when the
// exponent X after the rounding is in [-4, P), with precision P - 1 - X, else scientific, no trailing zeros
template <BOOST_DECIMAL_DECIMAL_FLOATING_TYPE TargetDecimalType>
BOOST_DECIMAL_CUDA_CONSTEXPR auto to_chars_general_precision(char* first, char* last, const TargetDecimalType& value, const int local_precision) noexcept -> to_chars_result
{
    using uint_type = std::conditional_t<(std::numeric_limits<typename TargetDecimalType::significand_type>::digits >
                                          std::numeric_limits<std::uint64_t>::digits),
                                          int128::uint128_t, std::uint64_t>;

    const auto significant_digits {local_precision == 0 ? 1 : local_precision};
    const auto fp {fpclassify(value)};
    if (!(fp == FP_NORMAL || fp == FP_SUBNORMAL))
    {
        return to_chars_scientific_impl(first, last, value, chars_format::general, significant_digits - 1);
    }

    constexpr auto digits {std::numeric_limits<TargetDecimalType>::digits};
    int exp {};
    auto sig {static_cast<uint_type>(frexp10(value, &exp))};
    exp += digits - 1;
    if (significant_digits < digits)
    {
        sig = to_chars_round<TargetDecimalType>(sig, digits - significant_digits, signbit(value));
        if (sig == pow10(static_cast<uint_type>(significant_digits)))
        {
            sig /= 10U;
            ++exp;
        }
    }

    char sig_digits[64] {};
    auto num_digits {static_cast<int>(to_chars_integer_impl<uint_type>(sig_digits, sig_digits + sizeof(sig_digits), sig).ptr - sig_digits)};
    while (sig_digits[num_digits - 1] == '0')
    {
        --num_digits;
    }

    const bool scientific {exp < -4 || exp >= significant_digits};
    const auto abs_exp {exp < 0 ? -exp : exp};
    const auto exp_digits {abs_exp < 100 ? 2 : abs_exp < 1000 ? 3 : 4};
    const auto integer_digits {scientific ? 1 : exp + 1};

    // Sign, leading zeros, digits, zeros up to the point, point, exponent
    const auto length {static_cast<int>(signbit(value)) + (integer_digits <= 0 ? 1 - integer_digits : 0) + num_digits +
                       (num_digits < integer_digits ? integer_digits - num_digits : 0) +
                       (num_digits > integer_digits ? 1 : 0) + (scientific ? 2 + exp_digits : 0)};
    if (last - first < length)
    {
        return {last, std::errc::value_too_large};
    }

    if (signbit(value))
    {
        *first++ = '-';
    }

    if (integer_digits <= 0)
    {
        *first++ = '0';
        *first++ = '.';
        detail::memset(first, '0', static_cast<std::size_t>(-integer_digits));
        first += -integer_digits;
        detail::memcpy(first, sig_digits, static_cast<std::size_t>(num_digits));
        return {first + num_digits, std::errc()};
    }

    for (int i {}; i < num_digits || i < integer_digits; ++i)
    {
        if (i == integer_digits)
        {
            *first++ = '.';
        }
        *first++ = i < num_digits ? sig_digits[i] : '0';
    }

    if (scientific)
    {
        *first++ = 'e';
        *first++ = exp < 0 ? '-' : '+';
        if (abs_exp < 10)
        {
            *first++ = '0';
        }
        first = to_chars_integer_impl<int>(first, last, abs_exp).ptr;
    }

    return {first, std::errc()};
}

template <BOOST_DECIMAL_DECIMAL_FLOATING_TYPE TargetDecimalType>
BOOST_DECIMAL_CUDA_CONSTEXPR auto to_chars_hex_impl(char* first, char* last, const TargetDecimalType& value) noexcept -> to_chars_result
{
    using Unsigned_Integer = std::conditional_t<(std::numeric_limits<typename TargetDecimalType::significand_type>::digits >
                                                 std::numeric_limits<std::uint64_t>::digits),
                                                 int128::uint128_t, std::uint64_t>;

    bool is_neg {false};
    if (signbit(value))
    {
        *first++ = '-';
        is_neg = true;
    }

    const auto fp = fpclassify(value);
    if (!(fp == FP_NORMAL || fp == FP_SUBNORMAL))
    {
        return to_chars_nonfinite(first, last, value, fp, chars_format::hex, -1);
    }

    const auto buffer_size {last - first};
    const auto real_precision {get_real_precision<TargetDecimalType>()};

    // Dummy check the bounds
    if (BOOST_DECIMAL_UNLIKELY(buffer_size < real_precision))
    {
        return {last, std::errc::value_too_large}; // LCOV_EXCL_LINE
    }

    const auto components {value.to_components()};

    auto exp {components.exp};
    auto significand {static_cast<Unsigned_Integer>(components.sig)};
    BOOST_DECIMAL_ASSERT(significand != 0U);

    if (significand % 10U == 0U)
    {
        const auto zero_removal {remove_trailing_zeros(significand)};
        exp += static_cast<int>(zero_removal.number_of_removed_zeros);
        significand = zero_removal.trimmed_number;
    }

    auto r = to_chars_integer_impl<Unsigned_Integer, Unsigned_Integer>(first + 1, last, significand, 16);
    if (BOOST_DECIMAL_UNLIKELY(!r))
    {
        return r; // LCOV_EXCL_LINE
    }

    const auto current_digits {r.ptr - (first + 1) - 1};
    exp += static_cast<int>(current_digits);

    // Make sure the result will fit in the buffer before continuing progress
    const auto total_length {total_buffer_length<TargetDecimalType>(static_cast<int>(current_digits), exp, is_neg)};
    if (BOOST_DECIMAL_UNLIKELY(total_length > buffer_size))
    {
        return {last, std::errc::value_too_large}; // LCOV_EXCL_LINE
    }

    // Insert our decimal point (or don't in the 1 digit case)
    *first = *(first + 1);
    if (BOOST_DECIMAL_LIKELY(current_digits > 0))
    {
        *(first + 1) = '.';
    }
    else
    {
        --r.ptr;
    }
    first = r.ptr;

    *first++ = 'p';
    if (exp < 0)
    {
        *first++ = '-';
        exp = -exp;
    }
    else
    {
        *first++ = '+';
    }

    if (exp < 10)
    {
        *first++ = '0';
    }

    return to_chars_integer_impl<std::uint32_t>(first, last, static_cast<std::uint32_t>(exp));
}

template <BOOST_DECIMAL_DECIMAL_FLOATING_TYPE TargetDecimalType>
BOOST_DECIMAL_CUDA_CONSTEXPR auto to_chars_hex_impl(char* first, char* last, const TargetDecimalType& value, const int local_precision) noexcept -> to_chars_result
{
    using Unsigned_Integer = std::conditional_t<(std::numeric_limits<typename TargetDecimalType::significand_type>::digits >
                                                 std::numeric_limits<std::uint64_t>::digits),
                                                 int128::uint128_t, std::uint64_t>;

    if (signbit(value))
    {
        *first++ = '-';
    }

    const auto fp = fpclassify(value);
    if (!(fp == FP_NORMAL || fp == FP_SUBNORMAL))
    {
        return to_chars_nonfinite(first, last, value, fp, chars_format::hex, local_precision);
    }

    const std::ptrdiff_t buffer_size = last - first;
    auto real_precision = get_real_precision<TargetDecimalType>(precision);

    if (local_precision != -1)
    {
        real_precision = local_precision;
    }

    if (BOOST_DECIMAL_UNLIKELY(buffer_size < real_precision))
    {
        return {last, std::errc::value_too_large};
    }

    int exp {};
    Unsigned_Integer significand = frexp10(value, &exp);
    BOOST_DECIMAL_ASSERT(significand != 0U);
    // Strip zeros of the significand since frexp10 normalizes it
    const auto zero_removal {detail::remove_trailing_zeros(significand)};
    significand = zero_removal.trimmed_number;
    exp += static_cast<int>(zero_removal.number_of_removed_zeros);

    // Calculate the number of bytes
    constexpr auto significand_bits = std::is_same<Unsigned_Integer, std::uint64_t>::value ? 64 : 128;
    auto significand_digits = static_cast<int>((static_cast<double>(significand_bits - detail::countl_zero(significand) + 1) / 4));
    bool append_zeros = false;

    if (local_precision != -1)
    {
        if (significand_digits > local_precision + 2)
        {
            const auto shift_amount {significand_digits - (local_precision + 2)};
            significand >>= (shift_amount * 4);
            significand_digits -= shift_amount;
        }

        if (significand_digits > local_precision + 1)
        {
            const auto trailing_digit = static_cast<std::uint32_t>(significand & 0xFU);
            significand >>= 4;
            ++exp;
            if (trailing_digit >= 8)
            {
                ++significand;
            }
        }

        if (significand_digits < local_precision)
        {
            append_zeros = true;
        }
    }

    auto r = to_chars_integer_impl<Unsigned_Integer, Unsigned_Integer>(first + 1, last, significand, 16);
    if (BOOST_DECIMAL_UNLIKELY(!r))
    {
        return r; // LCOV_EXCL_LINE
    }

    const auto current_digits = r.ptr - (first + 1) - 1;
    exp += static_cast<int>(current_digits);

    if (current_digits < local_precision)
    {
        append_zeros = true;
    }

    if (append_zeros)
    {
        const auto zeros_inserted {static_cast<std::size_t>(local_precision - current_digits)};

        if (r.ptr + zeros_inserted > last)
        {
            return {last, std::errc::value_too_large};
        }

        boost::decimal::detail::memset(r.ptr, '0', zeros_inserted);
        r.ptr += zeros_inserted;
    }

    // Insert our decimal point
    *first = *(first + 1);
    *(first + 1) = '.';
    first = r.ptr;

    if (local_precision == 0)
    {
        --first;
    }

    *first++ = 'p';
    if (exp < 0)
    {
        *first++ = '-';
    }
    else
    {
        *first++ = '+';
    }

    const int abs_exp { (exp < 0) ? -exp : exp };

    if (abs_exp < 10)
    {
        *first++ = '0';
    }

    return to_chars_integer_impl<std::uint32_t>(first, last, static_cast<std::uint32_t>(abs_exp));
}

#ifdef _MSC_VER
# pragma warning(push)
# pragma warning(disable: 4702) // Unreachable code
#endif

template <BOOST_DECIMAL_DECIMAL_FLOATING_TYPE TargetDecimalType>
BOOST_DECIMAL_CUDA_CONSTEXPR auto to_chars_cohort_preserving_scientific(char* first, char* last, const TargetDecimalType& value) noexcept -> to_chars_result
{
    BOOST_DECIMAL_IF_CONSTEXPR (detail::is_fast_type_v<TargetDecimalType>)
    {
        return {last, std::errc::invalid_argument};
    }

    using unsigned_integer = typename TargetDecimalType::significand_type;

    const auto fp = fpclassify(value);
    if (!(fp == FP_NORMAL || fp == FP_SUBNORMAL || fp == FP_ZERO))
    {
        // Cohorts are irrelevant for non-finite values, but the sign is still significant
        if (signbit(value))
        {
            *first++ = '-';
        }

        return to_chars_nonfinite(first, last, value, fp, chars_format::scientific, -1);
    }

    // First we print the significand of the number by decoding the value,
    // and using our existing to_chars for integers
    //
    // We possibly offset the to_chars by one in the event that we know we will have a fraction
    const auto components {value.to_components()};
    const auto significand {components.sig};
    auto exponent {static_cast<int>(components.exp)};

    if (components.sign)
    {
        *first++ = '-';
    }

    // Any significand of two or more digits gets a decimal point after the leading digit
    const bool fractional_piece {significand >= 10U};
    const auto r {to_chars_integer_impl<unsigned_integer>(first + static_cast<std::ptrdiff_t>(fractional_piece), last, significand)};

    if (BOOST_DECIMAL_UNLIKELY(!r))
    {
        return r; // LCOV_EXCL_LINE
    }

    // If there is more than one digit in the significand then we are going to need to:
    // First: insert a decimal point
    // Second: figure out how many decimal points we are going to have to adjust the exponent accordingly
    if (fractional_piece)
    {
        *first = *(first + 1);
        *(first + 1) = '.';

        const auto offset {num_digits(significand) - 1};
        exponent += offset;
    }

    // Insert the exponent characters ensuring that there are always at least two digits after the "e",
    // e.g. e+07 not e+7
    first = r.ptr;
    *first++ = 'e';
    const bool negative_exp {exponent < 0};
    *first++ = negative_exp ? '-' : '+';

    const auto abs_exp { static_cast<std::uint32_t>(negative_exp ? -exponent : exponent) };
    if (abs_exp < 10U)
    {
        *first++ = '0';
    }

    return to_chars_integer_impl<std::uint32_t>(first, last, abs_exp);
}

// Emits the value in fixed notation while preserving the exact stored significand and exponent.
// A positive exponent would need trailing zeros appended, which changes the cohort member on
// read-back, so such values are rejected with std::errc::invalid_argument.
template <BOOST_DECIMAL_DECIMAL_FLOATING_TYPE TargetDecimalType>
BOOST_DECIMAL_CUDA_CONSTEXPR auto to_chars_cohort_preserving_fixed(char* first, char* last, const TargetDecimalType& value) noexcept -> to_chars_result
{
    BOOST_DECIMAL_IF_CONSTEXPR (detail::is_fast_type_v<TargetDecimalType>)
    {
        return {last, std::errc::invalid_argument};
    }

    const auto buffer_size {last - first};

    const auto fp = fpclassify(value);
    if (!(fp == FP_NORMAL || fp == FP_SUBNORMAL || fp == FP_ZERO))
    {
        // Cohorts are irrelevant for non-finite values, but the sign is still significant
        if (signbit(value))
        {
            *first++ = '-';
        }

        return to_chars_nonfinite(first, last, value, fp, chars_format::fixed, -1);
    }

    // Decode the exact significand and exponent. Unlike the regular fixed path we must not
    // strip trailing zeros from the significand, as that would destroy the cohort.
    const auto components {value.to_components()};
    const auto significand {components.sig};
    const auto exponent {static_cast<int>(components.exp)};

    // A positive exponent can only be printed in fixed notation by appending trailing zeros
    if (exponent > 0)
    {
        return {last, std::errc::invalid_argument};
    }

    char* current = first;
    if (components.sign)
    {
        *current++ = '-';
    }

    const auto r {to_chars_integer_impl(current, last, significand)};
    if (BOOST_DECIMAL_UNLIKELY(!r))
    {
        return r; // LCOV_EXCL_LINE
    }

    const auto num_digits {r.ptr - current};
    const auto abs_exp {-exponent};

    // Three cases mirror the fixed path, minus the positive-exponent (trailing zeros) case:
    // 1) exponent == 0: the integer digits are the whole answer, e.g. 300
    // 2) abs_exp < num_digits: insert a decimal point within the digits, e.g. 1.50
    // 3) abs_exp >= num_digits: prepend "0." and any leading zeros, e.g. 0.0015
    if (abs_exp == 0)
    {
        return {r.ptr, std::errc{}};
    }
    else if (abs_exp < num_digits)
    {
        if (BOOST_DECIMAL_UNLIKELY(buffer_size < (current - first) + num_digits + 1))
        {
            return {last, std::errc::value_too_large};
        }

        const auto decimal_pos {num_digits - abs_exp};
        detail::memmove(current + decimal_pos + 1, current + decimal_pos, static_cast<std::size_t>(abs_exp));
        current[decimal_pos] = '.';

        return {r.ptr + 1, std::errc{}};
    }
    else
    {
        const auto leading_zeros {abs_exp - num_digits};
        if (BOOST_DECIMAL_UNLIKELY(buffer_size < (current - first) + 2 + leading_zeros + num_digits))
        {
            return {last, std::errc::value_too_large};
        }

        detail::memmove(current + 2 + leading_zeros, current, static_cast<std::size_t>(num_digits));
        current[0] = '0';
        current[1] = '.';
        detail::memset(current + 2, '0', static_cast<std::size_t>(leading_zeros));

        return {current + 2 + leading_zeros + num_digits, std::errc{}};
    }
}

template <BOOST_DECIMAL_DECIMAL_FLOATING_TYPE TargetDecimalType>
BOOST_DECIMAL_CUDA_CONSTEXPR auto to_chars_impl(char* first, char* last, const TargetDecimalType& value, const chars_format fmt = chars_format::general, const int local_precision = -1) noexcept -> to_chars_result
{
    // Sanity check our bounds
    if (BOOST_DECIMAL_UNLIKELY(first >= last))
    {
        return {last, std::errc::invalid_argument};
    }

    auto abs_value = abs(value);
    constexpr auto max_fractional_value = decimal_val_v<TargetDecimalType> < 64 ?  TargetDecimalType{1, 7} :
                                                          decimal_val_v<TargetDecimalType> < 128 ? TargetDecimalType{1, 16} :
                                                                                                   TargetDecimalType{1, 34};

    constexpr auto min_fractional_value = TargetDecimalType{1, -4};

    // Unspecified precision so we always go with the shortest representation
    if (local_precision == -1)
    {
        switch (fmt)
        {
            case chars_format::general:
                if (abs_value >= min_fractional_value && abs_value < max_fractional_value)
                {
                    return to_chars_fixed_impl(first, last, value, fmt);
                }
                else
                {
                    return to_chars_scientific_impl(first, last, value, fmt);
                }
            case chars_format::fixed:
                return to_chars_fixed_impl(first, last, value, fmt);
            case chars_format::scientific:
                return to_chars_scientific_impl(first, last, value, fmt);
            case chars_format::hex:
                return to_chars_hex_impl(first, last, value);
            case chars_format::cohort_preserving_scientific:

                if (local_precision != -1)
                {
                    // Precision and cohort preservation are mutually exclusive options
                    return {last, std::errc::invalid_argument};
                }

                return to_chars_cohort_preserving_scientific(first, last, value);
            case chars_format::cohort_preserving_fixed:

                if (local_precision != -1)
                {
                    // Precision and cohort preservation are mutually exclusive options
                    return {last, std::errc::invalid_argument};
                }

                return to_chars_cohort_preserving_fixed(first, last, value);
            // LCOV_EXCL_START
            default:
                BOOST_DECIMAL_UNREACHABLE;
            // LCOV_EXCL_STOP
        }
    }
    else
    {
        switch (fmt)
        {
            case chars_format::general:
                return to_chars_general_precision(first, last, value, local_precision);
            case chars_format::fixed:
                return to_chars_fixed_impl(first, last, value, fmt, local_precision);
            case chars_format::hex:
                return to_chars_hex_impl(first, last, value, local_precision);
            case chars_format::cohort_preserving_scientific:
            case chars_format::cohort_preserving_fixed:
                return {last, std::errc::invalid_argument};
            default:
                return to_chars_scientific_impl(first, last, value, fmt, local_precision);
        }
    }

    return to_chars_scientific_impl(first, last, value, fmt, local_precision); // LCOV_EXCL_LINE
}

#ifdef _MSC_VER
# pragma warning(pop)
#endif

} //namespace detail

BOOST_DECIMAL_EXPORT template <BOOST_DECIMAL_DECIMAL_FLOATING_TYPE TargetDecimalType>
BOOST_DECIMAL_CUDA_CONSTEXPR auto to_chars(char* first, char* last, const TargetDecimalType& value) noexcept -> to_chars_result
{
    return detail::to_chars_impl(first, last, value);
}

BOOST_DECIMAL_EXPORT template <BOOST_DECIMAL_DECIMAL_FLOATING_TYPE TargetDecimalType>
BOOST_DECIMAL_CUDA_CONSTEXPR auto to_chars(char* first, char* last, const TargetDecimalType& value, const chars_format fmt) noexcept -> to_chars_result
{
    return detail::to_chars_impl(first, last, value, fmt);
}

BOOST_DECIMAL_EXPORT template <BOOST_DECIMAL_DECIMAL_FLOATING_TYPE TargetDecimalType>
BOOST_DECIMAL_CUDA_CONSTEXPR auto to_chars(char* first, char* last, const TargetDecimalType& value, const chars_format fmt, int precision) noexcept -> to_chars_result
{
    if (precision < 0)
    {
        precision = 6;
    }

    return detail::to_chars_impl(first, last, value, fmt, precision);
}

namespace detail {

// The room that to_chars needs at most. The margin holds the sign, the point, the exponent,
// a carry and a NaN, and inf and NaN do not use the precision
template <BOOST_DECIMAL_DECIMAL_FLOATING_TYPE DecimalType>
auto to_chars_length(const DecimalType& d, const chars_format fmt, const int local_precision) noexcept -> std::size_t
{
    constexpr std::size_t margin {64U};
    if (!isfinite(d))
    {
        return margin;
    }

    // d is sig * 10^exp with all the digits of the type in sig
    constexpr int digits {std::numeric_limits<DecimalType>::digits};
    int exp {};
    static_cast<void>(frexp10(d, &exp));
    const auto integer_digits {static_cast<std::size_t>(exp + digits > 0 ? exp + digits : 0)};
    const auto precision_digits {static_cast<std::size_t>(local_precision > 0 ? local_precision : 0)};
    const auto fraction_digits {static_cast<std::size_t>(exp < 0 ? -exp : 0)};

    switch (fmt)
    {
        case chars_format::cohort_preserving_fixed:
            // The cohort keeps the quantum, also for a zero, so the exponent range of the type bounds it
            return margin + 2U * static_cast<std::size_t>(digits) +
                   static_cast<std::size_t>(std::numeric_limits<DecimalType>::max_exponent10 - std::numeric_limits<DecimalType>::min_exponent10);
        case chars_format::fixed:
            // The precision, or else all the digits after the point
            return margin + integer_digits + (local_precision >= 0 ? precision_digits : fraction_digits);
        case chars_format::general:
            // The trailing zeros go, so at most the digits of the type stay, and the fixed form
            // of %g fills the integer part with zeros
            return margin + static_cast<std::size_t>(digits) + integer_digits;
        default:
            return margin + static_cast<std::size_t>(digits) + precision_digits;
    }
}

// ASCII only, because std::toupper of the C locale gives "iNF" in a Turkish locale
inline void make_uppercase(char* first, const char* last) noexcept
{
    while (first != last)
    {
        if (*first >= 'a' && *first <= 'z')
        {
            *first = static_cast<char>(*first - 'a' + 'A');
        }
        ++first;
    }
}

} // namespace detail

#ifdef BOOST_DECIMAL_HAS_STD_CHARCONV

BOOST_DECIMAL_EXPORT template <typename DecimalType>
constexpr auto to_chars(char* first, char* last, DecimalType value, std::chars_format fmt)
    BOOST_DECIMAL_REQUIRES_RETURN(detail::is_decimal_floating_point_v, DecimalType, std::to_chars_result)
{
    to_chars_result boost_r {};
    switch (fmt)
    {
        case std::chars_format::scientific:
            boost_r = detail::to_chars_impl(first, last, value, chars_format::scientific);
            break;
        case std::chars_format::fixed:
            boost_r = detail::to_chars_impl(first, last, value, chars_format::fixed);
            break;
        case std::chars_format::hex:
            boost_r = detail::to_chars_impl(first, last, value, chars_format::hex);
            break;
        case std::chars_format::general:
            boost_r = detail::to_chars_impl(first, last, value, chars_format::general);
            break;
        // LCOV_EXCL_START
        default:
            BOOST_DECIMAL_UNREACHABLE;
        // LCOV_EXCL_STOP
    }

    return std::to_chars_result {boost_r.ptr, boost_r.ec};
}

BOOST_DECIMAL_EXPORT template <typename DecimalType>
constexpr auto to_chars(char* first, char* last, DecimalType value, std::chars_format fmt, int precision)
    BOOST_DECIMAL_REQUIRES_RETURN(detail::is_decimal_floating_point_v, DecimalType, std::to_chars_result)
{
    if (precision < 0)
    {
        precision = 6;
    }

    to_chars_result boost_r {};
    switch (fmt)
    {
        case std::chars_format::scientific:
            boost_r = detail::to_chars_impl(first, last, value, chars_format::scientific, precision);
            break;
        case std::chars_format::fixed:
            boost_r = detail::to_chars_impl(first, last, value, chars_format::fixed, precision);
            break;
        case std::chars_format::hex:
            boost_r = detail::to_chars_impl(first, last, value, chars_format::hex, precision);
            break;
        case std::chars_format::general:
            boost_r = detail::to_chars_impl(first, last, value, chars_format::general, precision);
            break;
        // LCOV_EXCL_START
        default:
            BOOST_DECIMAL_UNREACHABLE;
        // LCOV_EXCL_STOP
    }

    return std::to_chars_result {boost_r.ptr, boost_r.ec};
}

#endif // BOOST_DECIMAL_HAS_STD_CHARCONV

} //namespace decimal
} //namespace boost

#endif

#endif //BOOST_DECIMAL_CHARCONV_HPP
