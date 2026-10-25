// Copyright 2023 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_DECIMAL_DETAIL_PARSER_HPP
#define BOOST_DECIMAL_DETAIL_PARSER_HPP

#include <boost/decimal/detail/config.hpp>
#include <boost/decimal/detail/from_chars_result.hpp>
#include <boost/decimal/detail/from_chars_integer_impl.hpp>
#include <boost/decimal/detail/integer_search_trees.hpp>
#include <boost/decimal/detail/power_tables.hpp>
#include <boost/decimal/detail/chars_format.hpp>

#ifndef BOOST_DECIMAL_BUILD_MODULE
#include <cerrno>
#include <cstdint>
#include <cstring>
#include <limits>
#if !defined(BOOST_DECIMAL_DISABLE_CLIB)
#include <system_error>
#endif
#include <type_traits>

#endif // BOOST_DECIMAL_BUILD_MODULE

namespace boost {
namespace decimal {
namespace detail {

BOOST_DECIMAL_CUDA_CONSTEXPR auto is_integer_char(char c) noexcept -> bool
{
    return (c >= '0') && (c <= '9');
}

BOOST_DECIMAL_CUDA_CONSTEXPR auto is_hex_char(char c) noexcept -> bool
{
    return is_integer_char(c) || (((c >= 'a') && (c <= 'f')) || ((c >= 'A') && (c <= 'F')));
}

// Tells if c is a digit of the significand in the format fmt
BOOST_DECIMAL_CUDA_CONSTEXPR auto is_significand_char(const char c, const chars_format fmt) noexcept -> bool
{
    return fmt != chars_format::hex ? is_integer_char(c) : is_hex_char(c);
}

BOOST_DECIMAL_CUDA_CONSTEXPR auto is_payload_char(const char c) noexcept -> bool
{
    return is_integer_char(c) || (((c >= 'a') && (c <= 'z')) || ((c >= 'A') && (c <= 'Z')));
}

BOOST_DECIMAL_CUDA_CONSTEXPR auto is_delimiter(char c, chars_format fmt) noexcept -> bool
{
    if (fmt != chars_format::hex)
    {
        // In the fixed format an 'e' ends the number
        return !is_integer_char(c) && ((c != 'e' && c != 'E') || fmt == chars_format::fixed);
    }

    return !is_hex_char(c) && c != 'p' && c != 'P';
}

#if !defined(BOOST_DECIMAL_DISABLE_CLIB)
BOOST_DECIMAL_CUDA_CONSTEXPR auto from_chars_dispatch(const char* first, const char* last, std::uint64_t& value, int base) noexcept -> from_chars_result
{
    return boost::decimal::detail::from_chars(first, last, value, base);
}

BOOST_DECIMAL_CUDA_CONSTEXPR auto from_chars_dispatch(const char* first, const char* last, int128::uint128_t& value, int base) noexcept -> from_chars_result
{
    return boost::decimal::detail::from_chars128(first, last, value, base);
}
#endif

#ifdef BOOST_DECIMAL_HAS_INT128
BOOST_DECIMAL_CUDA_CONSTEXPR auto from_chars_dispatch(const char* first, const char* last, builtin_uint128_t& value, int base) noexcept -> from_chars_result
{
    return boost::decimal::detail::from_chars128(first, last, value, base);
}
#endif

#if !defined(BOOST_DECIMAL_DISABLE_CLIB)
// Takes the significand digits as the parser reads them, so that they are not read twice.
// A type wider than 64 bits takes them in 64-bit groups of 19 decimal or 15 hex digits
template <typename Unsigned_Integer>
struct significand_accumulator
{
    Unsigned_Integer high {};
    std::uint64_t low {};
    std::size_t low_digits {};

    // The value of high goes up by base^low_digits when low joins it
    BOOST_DECIMAL_FORCE_INLINE BOOST_DECIMAL_CUDA_CONSTEXPR auto join(const int base) const noexcept -> Unsigned_Integer
    {
        const auto scale {base == 10 ? pow10(static_cast<std::uint64_t>(low_digits)) : std::uint64_t {1U} << (4U * low_digits)};
        return static_cast<Unsigned_Integer>(high * static_cast<Unsigned_Integer>(scale) + static_cast<Unsigned_Integer>(low));
    }

    // Moves low into high, so that low can take the next group of digits
    BOOST_DECIMAL_FORCE_INLINE BOOST_DECIMAL_CUDA_CONSTEXPR void flush(const int base) noexcept
    {
        high = join(base);
        low = 0U;
        low_digits = 0U;
    }

    BOOST_DECIMAL_FORCE_INLINE BOOST_DECIMAL_CUDA_CONSTEXPR void push(const char c, const int base) noexcept
    {
        BOOST_DECIMAL_IF_CONSTEXPR (sizeof(Unsigned_Integer) > sizeof(std::uint64_t))
        {
            if (low_digits == (base == 10 ? 19U : 15U))
            {
                flush(base);
            }
            ++low_digits;
        }

        low = low * static_cast<std::uint64_t>(base) + (base == 10 ? static_cast<std::uint64_t>(c - '0') : static_cast<std::uint64_t>(digit_from_char(c)));
    }

    static BOOST_DECIMAL_FORCE_INLINE BOOST_DECIMAL_CUDA_CONSTEXPR auto byte_at(const char* p, const int k) noexcept -> std::uint64_t
    {
        return static_cast<std::uint64_t>(static_cast<unsigned char>(p[k])) << (8 * k);
    }

    // Takes eight decimal digits from one 64-bit word, when all eight are digits. The word is put together
    // from single bytes, which keeps the order of the digits on all platforms
    BOOST_DECIMAL_FORCE_INLINE BOOST_DECIMAL_CUDA_CONSTEXPR auto push_eight(const char* p) noexcept -> bool
    {
        auto word {byte_at(p, 0) | byte_at(p, 1) | byte_at(p, 2) | byte_at(p, 3) | byte_at(p, 4) | byte_at(p, 5) | byte_at(p, 6) | byte_at(p, 7)};

        // A byte below '0' borrows in the subtraction, and a byte above '9' carries in the addition
        if ((((word + UINT64_C(0x4646464646464646)) | (word - UINT64_C(0x3030303030303030))) & UINT64_C(0x8080808080808080)) != 0U)
        {
            return false;
        }

        BOOST_DECIMAL_IF_CONSTEXPR (sizeof(Unsigned_Integer) > sizeof(std::uint64_t))
        {
            if (low_digits > 19U - 8U)
            {
                flush(10);
            }
            low_digits += 8U;
        }

        // The digits join in pairs, then the pairs in fours, then the fours in one value
        word -= UINT64_C(0x3030303030303030);
        word = word * 10U + (word >> 8U);
        word = (((word & UINT64_C(0x000000FF000000FF)) * (100U + (UINT64_C(1000000) << 32U))) +
                (((word >> 16U) & UINT64_C(0x000000FF000000FF)) * (1U + (UINT64_C(10000) << 32U)))) >> 32U;

        low = low * UINT64_C(100000000) + word;
        return true;
    }

    // The parser keeps more digits than any type, so a nonzero last digit is below the rounding position
    // of the type, and keeps the effect of the dropped digits on rounding
    BOOST_DECIMAL_FORCE_INLINE BOOST_DECIMAL_CUDA_CONSTEXPR void mark_dropped_nonzero(const int base) noexcept
    {
        if (low % static_cast<std::uint64_t>(base) == 0U)
        {
            ++low;
        }
    }

    // Without digits there is no number, and only hex digits can be too many for the type
    BOOST_DECIMAL_FORCE_INLINE BOOST_DECIMAL_CUDA_CONSTEXPR auto get(Unsigned_Integer& value, const std::size_t count, const int base) const noexcept -> std::errc
    {
        if (count == 0U)
        {
            return std::errc::invalid_argument;
        }
        if (base != 10 && count > 2U * sizeof(Unsigned_Integer))
        {
            return std::errc::result_out_of_range;
        }

        BOOST_DECIMAL_IF_CONSTEXPR (sizeof(Unsigned_Integer) > sizeof(std::uint64_t))
        {
            value = join(base);
        }
        else
        {
            value = static_cast<Unsigned_Integer>(low);
        }
        return std::errc();
    }
};

// Reads significand digits until the type can take no more, eight at a time where it can.
// A literal keeps its digit separators, as in 1'000_DF, and with Separators the one after a digit is skipped
template <bool Separators, typename Unsigned_Integer>
BOOST_DECIMAL_FORCE_INLINE BOOST_DECIMAL_CUDA_CONSTEXPR auto read_significand_digits(const char* next, const char* last, std::size_t& i,
                                                                                     significand_accumulator<Unsigned_Integer>& digits, const chars_format fmt) noexcept -> const char*
{
    constexpr std::size_t max_digits {std::numeric_limits<Unsigned_Integer>::digits10};
    const int base {fmt != chars_format::hex ? 10 : 16};

    while (fmt != chars_format::hex && last - next >= 8 && i + 8U <= max_digits && digits.push_eight(next))
    {
        next += 8;
        if (next != last && *next == '\'' && Separators)
        {
            ++next;
        }
        i += 8U;
    }

    while (i < max_digits && next != last && is_significand_char(*next, fmt))
    {
        digits.push(*next, base);
        ++next;
        if (next != last && *next == '\'' && Separators)
        {
            ++next;
        }
        ++i;
    }

    return next;
}

// When the exponent reaches the parser limit, the digit positions can still cancel it,
// so it is read again in 64 bits and clamped to 10^8, outside every type
template <bool Separators, typename Integer>
BOOST_DECIMAL_NO_INLINE BOOST_DECIMAL_CUDA_CONSTEXPR auto read_long_exponent(const char* first, const char* last, const bool negative,
                                                                             const std::int64_t positions) noexcept -> Integer
{
    constexpr std::int64_t long_limit {INT64_C(100000000000000000)};
    constexpr std::int64_t limit {100000000};
    std::int64_t value {};
    for (; first != last; ++first)
    {
        if (*first != '\'' || !Separators)
        {
            value = value < long_limit ? value * 10 + digit_from_char(*first) : long_limit;
        }
    }

    value = (negative ? -value : value) + positions;
    return static_cast<Integer>(value < -limit ? -limit : value > limit ? limit : value);
}

// Separators is set only for a literal. It skips a digit separator after each digit, as read_significand_digits does
template <typename Unsigned_Integer, typename Integer, bool Separators>
BOOST_DECIMAL_FORCE_INLINE BOOST_DECIMAL_CUDA_CONSTEXPR auto parser_body(const char* first, const char* last, bool& sign, Unsigned_Integer& significand, Integer& exponent, const chars_format fmt) noexcept -> from_chars_result
{
    if (first >= last)
    {
        return {first, std::errc::invalid_argument};
    }

    auto next = first;

    // First extract the sign
    if (*next == '-')
    {
        sign = true;
        ++next;
    }
    else if (*next == '+')
    {
        return {next, std::errc::invalid_argument};
    }
    else
    {
        sign = false;
    }

    constexpr std::size_t significand_buffer_size = std::numeric_limits<Unsigned_Integer>::digits10 ;
    constexpr std::size_t max_digits {std::numeric_limits<Unsigned_Integer>::digits10};

    // Handle non-finite values
    // Stl allows for string like "iNf" to return inf
    //
    // This is nested ifs rather than a big one-liner to ensure that once we hit an invalid character
    // or an end of buffer we return the correct value of next
    bool signaling {};
    if (next != last && (*next == 'i' || *next == 'I'))
    {
        ++next;
        if (next != last && (*next == 'n' || *next == 'N'))
        {
            ++next;
            if (next != last && (*next == 'f' || *next == 'F'))
            {
                ++next;
                exponent = 0;

                // The long form "infinity" is consumed only when it is complete
                // and | 0x20 makes an ASCII letter lowercase
                constexpr char tail[] {"inity"};
                constexpr std::size_t tail_size {sizeof(tail) - 1U};
                std::size_t n {};
                while (n < tail_size && next + n != last && (next[n] | 0x20) == tail[n])
                {
                    ++n;
                }
                return {n == tail_size ? next + tail_size : next, std::errc::value_too_large};
            }
        }

        return {first, std::errc::invalid_argument};
    }

    if (next != last && (*next == 's' || *next == 'S'))
    {
        ++next;
        signaling = true;
    }

    if (next != last && (*next == 'n' || *next == 'N'))
    {
        ++next;
        if (next != last && (*next == 'a' || *next == 'A'))
        {
            ++next;
            if (next != last && (*next == 'n' || *next == 'N'))
            {
                ++next;
                if (next != last)
                {
                    const auto current_pos {next};

                    bool any_valid_char {false};
                    bool has_opening_brace {false};
                    if (*next == '(')
                    {
                        ++next;
                        has_opening_brace = true;
                    }

                    // Handle nan(SNAN)
                    if ((last - next) >= 4 && (*next == 's' || *next == 'S') && (*(next + 1) == 'n' || *(next + 1) == 'N')
                        && (*(next + 2) == 'a' || *(next + 2) == 'A') && (*(next + 3) == 'n' || *(next + 3) == 'N'))
                    {
                        next += 4;
                        signaling = true;
                        any_valid_char = true;
                    }
                    // Handle Nan(IND)
                    else if ((last - next) >= 3 && (*next == 'i' || *next == 'I') && (*(next + 1) == 'n' || *(next + 1) == 'N')
                        && (*(next + 2) == 'd' || *(next + 2) == 'D'))
                    {
                        next += 3;
                        sign = true;
                        any_valid_char = true;
                    }

                    // Arbitrary numerical payload
                    bool has_numerical_payload {false};
                    char significand_buffer[significand_buffer_size] {};
                    auto significand_buffer_first {significand_buffer};
                    std::size_t significand_characters {};
                    while (next != last && (*next != ')'))
                    {
                        if (significand_characters < significand_buffer_size && is_integer_char(*next))
                        {
                            ++significand_characters;
                            *significand_buffer_first++ = *next++;
                            any_valid_char = true;
                            has_numerical_payload = true;
                        }
                        else
                        {
                            // End of valid payload even if there are more characters
                            // e.g. SNAN42JUNK stops at J
                            break;
                        }
                    }

                    // Non-numerical payload still needs to be parsed
                    // e.g. nan(PAYLOAD)
                    if (!has_numerical_payload && has_opening_brace)
                    {
                        while (next != last && (*next != ')'))
                        {
                            if (is_payload_char(*next))
                            {
                                any_valid_char = true;
                                ++next;
                            }
                            else
                            {
                                break;
                            }
                        }
                    }

                    if (next != last && any_valid_char)
                    {
                        // One past the end if we need to
                        ++next;
                    }

                    if (significand_characters != 0)
                    {
                        from_chars_dispatch(significand_buffer, significand_buffer + significand_characters, significand, 10);
                    }

                    if (!any_valid_char)
                    {
                        // If we have nan(..BAD..) we should point to (
                        next = current_pos;
                    }

                    exponent = static_cast<Integer>(signaling);
                    return {next, std::errc::not_supported};
                }
                else
                {
                    exponent = static_cast<Integer>(signaling);
                    return {next, std::errc::not_supported};
                }
            }
        }

        return {first, std::errc::invalid_argument};
    }

    // An s starts only a signaling NaN, and a sign needs a number after it,
    // so "s5", "s" and "-" are not numbers
    if (signaling || next == last)
    {
        return {first, std::errc::invalid_argument};
    }

    // Ignore leading zeros (e.g. 00005 or -002.3e+5)
    const auto digits_first {next};
    while (next != last && *next == '0')
    {
        ++next;
        if (next != last && *next == '\'' && Separators)
        {
            ++next;
        }
    }

    const char exp_char {fmt != chars_format::hex ? 'e' : 'p'};
    const char capital_exp_char {fmt != chars_format::hex ? 'E' : 'P'};

    // Plain "0" (or "00...") with no fractional or exponent part: significand and exponent are both 0.
    if (next == last)
    {
        if (fmt == chars_format::scientific || fmt == chars_format::cohort_preserving_scientific)
        {
            return {first, std::errc::invalid_argument};
        }

        significand = 0;
        exponent = 0;
        return {next, std::errc()};
    }

    // "0e-3" or "0E+5": significand is 0, but the exponent must be parsed so the cohort is preserved
    // per IEEE 754-2008 3.5.1 ("the cohort of +0 contains a representation for each exponent").
    // Set significand to 0 here and fall through to the existing significand-then-exponent flow,
    // which reads the exponent at the exp_char branch below, unless fixed ends the number at the e.
    if (*next == exp_char || *next == capital_exp_char)
    {
        significand = 0;
    }

    // Next we get the significand
    std::size_t i = 0;
    std::size_t dot_position = 0;
    Integer extra_zeros = 0;
    Integer leading_zero_powers = 0;
    const int base = (fmt != chars_format::hex) ? 10 : 16;
    significand_accumulator<Unsigned_Integer> digits {};

    next = read_significand_digits<Separators>(next, last, i, digits, fmt);

    bool fractional = false;
    if (next == last)
    {
        // if fmt is chars_format::scientific the e is required
        if (fmt == chars_format::scientific || fmt == chars_format::cohort_preserving_scientific)
        {
            return {first, std::errc::invalid_argument};
        }

        exponent = 0;

        switch (digits.get(significand, i, base))
        {
            case std::errc::invalid_argument:
                return {first, std::errc::invalid_argument};
            case std::errc::result_out_of_range:
                return {next, std::errc::result_out_of_range};
            default:
                return {next, std::errc()};
        }
    }
    else if (*next == '.')
    {
        ++next;
        fractional = true;
        dot_position = i;

        // Process the fractional part if we have it
        //
        // if fmt is chars_format::scientific the e is required
        // if fmt is chars_format::fixed the e ends the number
        // if fmt is chars_format::general (which is scientific and fixed) the e is optional

        // If we have the value 0.00001 we can continue to chop zeros and adjust the exponent
        // so that we get the useful parts of the fraction
        if (i == 0)
        {
            while (next != last && *next == '0')
            {
                ++next;
                if (next != last && *next == '\'' && Separators)
                {
                    ++next;
                }
                --leading_zero_powers;
            }

            // A significand needs at least one digit, so "." and ".e5" are not numbers
            if (next == digits_first + 1 && (next == last || !is_significand_char(*next, fmt)))
            {
                return {first, std::errc::invalid_argument};
            }
        }

        next = read_significand_digits<Separators>(next, last, i, digits, fmt);
    }

    if (i == max_digits)
    {
        // We can not process any more significant figures into the significand so skip to the end
        // or the exponent part and capture the additional orders of magnitude for the exponent
        bool found_dot = fractional;
        bool dropped_nonzero = false;
        while (next != last && (is_significand_char(*next, fmt) || (*next == '.' && !found_dot)))
        {
            if (*next == '.')
            {
                found_dot = true;
            }
            else
            {
                dropped_nonzero = dropped_nonzero || *next != '0';
                if (!found_dot)
                {
                    ++extra_zeros;
                }
            }
            ++next;
            if (next != last && *next == '\'' && Separators)
            {
                ++next;
            }
        }

        if (dropped_nonzero)
        {
            digits.mark_dropped_nonzero(base);
        }
    }

    // Too many hex digits are an error, but the number still ends after its exponent
    bool too_many_digits {false};
    const auto exp_char_pos {next};
    if (next == last || is_delimiter(*next, fmt))
    {
        if (fmt == chars_format::scientific || fmt == chars_format::cohort_preserving_scientific)
        {
            return {first, std::errc::invalid_argument};
        }

        if (dot_position != 0 || fractional)
        {
            exponent = static_cast<Integer>(dot_position) - static_cast<Integer>(i) + extra_zeros + leading_zero_powers;
        }
        else
        {
            exponent = extra_zeros + leading_zero_powers;
        }

        // A significand of only zeros, as in "0," or "0.0 ", gives zero
        if (i == 0 && next != digits_first)
        {
            significand = 0;
            return {next, std::errc()};
        }

        switch (digits.get(significand, i, base))
        {
            case std::errc::result_out_of_range:
                return {next, std::errc::result_out_of_range};
            case std::errc::invalid_argument:
                return {first, std::errc::invalid_argument};
            default:
                return {next, std::errc()};
        }
    }
    else if (*next == exp_char || *next == capital_exp_char)
    {
        // Would be a number without a significand e.g. e+03
        if (next == digits_first)
        {
            return {first, std::errc::invalid_argument};
        }
        // cohort_preserving_fixed has an implied exponent of zero, so an explicit exponent is not allowed
        if (fmt == chars_format::cohort_preserving_fixed)
        {
            return {next, std::errc::invalid_argument};
        }

        ++next;

        exponent = static_cast<Integer>(i - 1);

        // If the significand is 0 there are no digits, but it is a valid value. We need to continue
        // parsing to get the correct value of ptr even though we know we could bail now.
        //
        // See GitHub issue #29: https://github.com/cppalliance/charconv/issues/29
        if (i != 0)
        {
            switch (digits.get(significand, i, base))
            {
                case std::errc::invalid_argument:
                    return {first, std::errc::invalid_argument};
                case std::errc::result_out_of_range:
                    too_many_digits = true;
                    break;
                default:
                    break;
            }
        }
    }
    else
    {
        return {first, std::errc::invalid_argument}; // LCOV_EXCL_LINE
    }

    // Finally we get the exponent. A long exponent stops at a limit, because a value that far
    // outside the range of the types is zero or infinity in each of them.
    bool negative_exponent {false};
    if (next != last && (*next == '-' || *next == '+'))
    {
        negative_exponent = *next == '-';
        ++next;
    }

    const auto exp_digits_first {next};
    // Far past the exponent range of every type. A value at the limit or above becomes the limit
    // at the next digit, so exponent_value * 10 + 9 always fits
    constexpr Integer exponent_limit {100000};
    Integer exponent_value {};
    while (next != last && is_integer_char(*next))
    {
        exponent_value = exponent_value < exponent_limit ? static_cast<Integer>(exponent_value * 10 + digit_from_char(*next)) : exponent_limit;
        ++next;
        if (next != last && *next == '\'' && Separators)
        {
            ++next;
        }
    }

    // Without exponent digits, the "e" and its sign are not part of the number
    if (next == exp_digits_first)
    {
        if (fmt == chars_format::scientific || fmt == chars_format::cohort_preserving_scientific)
        {
            return {first, std::errc::invalid_argument};
        }
        next = exp_char_pos;
    }

    if (BOOST_DECIMAL_UNLIKELY(exponent_value == exponent_limit))
    {
        const auto positions {static_cast<std::int64_t>(leading_zero_powers) + (fractional ? -static_cast<std::int64_t>(i - dot_position) : extra_zeros)};
        exponent = read_long_exponent<Separators, Integer>(exp_digits_first, next, negative_exponent, positions);
        return {next, too_many_digits ? std::errc::result_out_of_range : std::errc()};
    }

    exponent = (negative_exponent ? -exponent_value : exponent_value) + leading_zero_powers;

    if (fractional)
    {
        // Need to take the offset from 1.xxx because compute_floatXXX assumes the significand is an integer
        // so the exponent is off by the number of digits in the significand - 1
        exponent -= static_cast<Integer>(i - dot_position);
    }
    else
    {
        exponent += extra_zeros;
    }

    return {next, too_many_digits ? std::errc::result_out_of_range : std::errc()};
}

template <typename Unsigned_Integer, typename Integer, bool Separators = false>
BOOST_DECIMAL_CUDA_CONSTEXPR auto parser(const char* first, const char* last, bool& sign, Unsigned_Integer& significand, Integer& exponent, const chars_format fmt = chars_format::general) noexcept -> from_chars_result
{
    return fmt != chars_format::hex ? parser_body<Unsigned_Integer, Integer, Separators>(first, last, sign, significand, exponent, fmt)
                                    : parser_body<Unsigned_Integer, Integer, Separators>(first, last, sign, significand, exponent, chars_format::hex);
}
#endif

} // namespace detail
} // namespace decimal
} // namespace boost

#endif // BOOST_DECIMAL_DETAIL_PARSER_HPP
