// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_DECIMAL_DETAIL_INT128_DETAIL_FLOAT_CONVERSION_HPP
#define BOOST_DECIMAL_DETAIL_INT128_DETAIL_FLOAT_CONVERSION_HPP

#include <boost/decimal/detail/int128/detail/config.hpp>
#include <boost/decimal/detail/int128/detail/constants.hpp>
#include <boost/decimal/detail/int128/detail/clz.hpp>

#ifndef BOOST_DECIMAL_DETAIL_INT128_BUILD_MODULE

#include <cstdint>
#include <limits>
#include <type_traits>

#endif

namespace boost {
namespace int128 {
namespace detail {

// 2^exp as a T, exactly, for 0 <= exp <= 127. Splitting at 2^64 keeps both factors inside the
// range of an exact conversion from a 64-bit integer, so no rounding happens here
template <typename T>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr T exact_power_of_two(const int exp) noexcept
{
    return exp < 64 ? static_cast<T>(UINT64_C(1) << exp)
                    : static_cast<T>(UINT64_C(1) << (exp - 64)) * offset_value_v<T>;
}

// A significand of 64 bits or more holds each word exactly, and holds the scaled high word
// exactly as well, so the addition is the only rounding and the two term form is already
// correctly rounded. x87 80-bit (64), IEEE binary128 (113) and IBM double-double (106) all
// take this path
template <typename T>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr T unsigned_words_to_float_impl(const std::uint64_t high, const std::uint64_t low,
                                                                 std::true_type) noexcept
{
    return static_cast<T>(high) * offset_value_v<T> + static_cast<T>(low);
}

// float and double cannot hold the high word or the sum exactly, so high * 2^64 + low rounds
// as many as three times and lands up to one ulp away from the correctly rounded result.
// Round the 128-bit value to exactly digits bits here instead, once, then apply an exact
// power of two. See the note on ties in the body
template <typename T>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr T unsigned_words_to_float_impl(const std::uint64_t high, const std::uint64_t low,
                                                                 std::false_type) noexcept
{
    // Anything below 2^64 is one conversion the compiler already rounds correctly
    if (high == UINT64_C(0))
    {
        return static_cast<T>(low);
    }

    constexpr int digits {std::numeric_limits<T>::digits};
    constexpr int residue_bits {64 - digits};
    constexpr std::uint64_t residue_mask {(UINT64_C(1) << residue_bits) - UINT64_C(1)};
    constexpr std::uint64_t halfway {UINT64_C(1) << (residue_bits - 1)};

    // Normalize so bit 127 of the pair is set, which puts the significand at the top of the
    // high word. high is non-zero here, so the distance is always less than 64
    const auto shift {static_cast<unsigned>(countl_zero(high))};
    const auto norm_high {shift == 0U ? high : ((high << shift) | (low >> (64U - shift)))};
    const auto norm_low {shift == 0U ? low : (low << shift)};

    const auto significand {norm_high >> residue_bits};
    const auto residue {norm_high & residue_mask};

    // Round to nearest, ties to even. The discarded part is residue * 2^64 + norm_low, so it
    // is above the halfway point when residue is, and it is exactly the halfway point only
    // when residue equals halfway and every lower bit is clear
    const bool round_up {residue > halfway ||
                         (residue == halfway && (norm_low != UINT64_C(0) || (significand & UINT64_C(1)) != UINT64_C(0)))};

    const auto rounded {significand + (round_up ? UINT64_C(1) : UINT64_C(0))};

    // rounded holds digits bits, or digits + 1 when it carried, in which case it is a power of
    // two. Either way the conversion and the scaling are exact, so the product is the value
    // rounded exactly once. It overflows to infinity precisely when the correctly rounded
    // result does, which is the required behavior for round to nearest
    return static_cast<T>(rounded) * exact_power_of_two<T>(static_cast<int>(64U - shift) + residue_bits);
}

// Converts the 128-bit value (high, low) to T, correctly rounded to nearest with ties to even
template <typename T>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr T unsigned_words_to_float(const std::uint64_t high, const std::uint64_t low) noexcept
{
    return unsigned_words_to_float_impl<T>(high, low,
                                           std::integral_constant<bool, (std::numeric_limits<T>::digits >= 64)>{});
}

template <typename T>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr T signed_words_to_float(const std::int64_t high, const std::uint64_t low) noexcept
{
    if (high < 0)
    {
        // Two's complement magnitude of the full 128-bit value.
        // INT128_MIN needs no special case since it yields high = 2^63, low = 0
        const auto abs_low {~low + UINT64_C(1)};
        const auto abs_high {~static_cast<std::uint64_t>(high) + (abs_low == UINT64_C(0) ? UINT64_C(1) : UINT64_C(0))};

        // The magnitude is rounded once and the negation is exact
        return -unsigned_words_to_float<T>(abs_high, abs_low);
    }

    return unsigned_words_to_float<T>(static_cast<std::uint64_t>(high), low);
}

// The other direction: a value in [0, 2^64) truncated toward zero into a 64-bit word.
// A cast straight to an unsigned type takes the compiler's unsigned conversion path, which
// subtracts 2^63 and puts the top bit back afterwards. Clang 7 emits that subtraction inside the
// window where it has already set the x87 control word to single precision, so an 80-bit long
// double loses every significand bit past the 24th. Splitting at 2^63 here keeps both conversions
// inside the signed range, where the conversion is one instruction on every compiler
template <typename T>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr std::uint64_t float_to_uint64(const T value) noexcept
{
    constexpr T two_63 {static_cast<T>(UINT64_C(1) << 63)};

    // value - two_63 is exact: it is a multiple of the ulp of value, and it is below 2^63
    return value < two_63
           ? static_cast<std::uint64_t>(static_cast<std::int64_t>(value))
           : static_cast<std::uint64_t>(static_cast<std::int64_t>(value - two_63)) | (UINT64_C(1) << 63);
}

} // namespace detail
} // namespace int128
} // namespace boost

#endif // BOOST_DECIMAL_DETAIL_INT128_DETAIL_FLOAT_CONVERSION_HPP
