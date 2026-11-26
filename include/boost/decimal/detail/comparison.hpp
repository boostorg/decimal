// Copyright 2023 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_DECIMAL_DETAIL_COMPARISON_HPP
#define BOOST_DECIMAL_DETAIL_COMPARISON_HPP

#include <boost/decimal/detail/config.hpp>
#include <boost/decimal/detail/type_traits.hpp>
#include <boost/decimal/detail/apply_sign.hpp>
#include <boost/decimal/detail/promotion.hpp>
#include <boost/decimal/detail/shrink_significand.hpp>
#include <boost/decimal/detail/cmath/isfinite.hpp>
#include <boost/decimal/detail/concepts.hpp>
#include <boost/decimal/detail/power_tables.hpp>
#include <boost/decimal/detail/u256.hpp>
#include <boost/decimal/detail/select_bits.hpp>
#include "int128.hpp"
#include <boost/decimal/detail/attributes.hpp>

#ifndef BOOST_DECIMAL_BUILD_MODULE
#include <limits>
#include <cstdint>
#endif

namespace boost {
namespace decimal {

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4127)
#endif

// The significands fit in the type of DecimalType, the type with more digits. They are
// references, because a copy of a 128-bit significand reloads it just after two 64-bit stores.
template <BOOST_DECIMAL_DECIMAL_FLOATING_TYPE DecimalType, typename SigType>
BOOST_DECIMAL_FORCE_INLINE BOOST_DECIMAL_CUDA_CONSTEXPR auto equal_finite_parts(const SigType& lhs_sig, const int lhs_exp, const bool lhs_sign,
                                                                                const SigType& rhs_sig, const int rhs_exp, const bool rhs_sign) noexcept -> bool
{
    // Zeros from any cohort and of any sign compare equal (IEEE 754-2008 3.5.1), and
    // a zero never equals a nonzero value
    if (BOOST_DECIMAL_UNLIKELY(lhs_sig == 0U || rhs_sig == 0U))
    {
        return lhs_sig == rhs_sig;
    }

    // If the difference is greater than we can represent in the significand than we can assume they are different
    const auto delta_exp {lhs_exp - rhs_exp};

    if (BOOST_DECIMAL_UNLIKELY(delta_exp > detail::precision_v<DecimalType> || delta_exp < -detail::precision_v<DecimalType>))
    {
        return false;
    }

    // Scale the significand with the larger exponent. Random signs and exponents
    // mispredict a branch, so the operands are selected and the signs test last.
    const bool same_sign {lhs_sign == rhs_sign};
    const bool lhs_larger {delta_exp > 0};
    const auto scale {delta_exp < 0 ? -delta_exp : delta_exp};
    const SigType larger_sig {detail::select_bits(lhs_larger, lhs_sig, rhs_sig)};
    const SigType smaller_sig {detail::select_bits(lhs_larger, rhs_sig, lhs_sig)};

    BOOST_DECIMAL_IF_CONSTEXPR (detail::decimal_val_v<DecimalType> >= 128)
    {
        // The product fits, which needs no digit count for a small delta, or else it has more
        // digits than any significand
        constexpr auto max_delta_diff {std::numeric_limits<SigType>::digits10 - detail::precision_v<DecimalType>};
        constexpr auto max_digits {std::numeric_limits<SigType>::digits10};
        if (scale > max_delta_diff && detail::num_digits(larger_sig) + scale > max_digits)
        {
            return false;
        }
        return static_cast<bool>(static_cast<unsigned>(same_sign) & static_cast<unsigned>(larger_sig * detail::pow10(static_cast<SigType>(scale)) == smaller_sig));
    }
    else
    {
        // The two words of umul128, because 32-bit code calls a uint128_t product out of line
        const auto product {detail::umul128(static_cast<std::uint64_t>(larger_sig), static_cast<std::uint64_t>(detail::pow10(static_cast<SigType>(scale))))};
        return static_cast<bool>(static_cast<unsigned>(same_sign) & static_cast<unsigned>(((product.low ^ static_cast<std::uint64_t>(smaller_sig)) | product.high) == 0U));
    }
}

template <BOOST_DECIMAL_DECIMAL_FLOATING_TYPE DecimalType>
BOOST_DECIMAL_FORCE_INLINE BOOST_DECIMAL_CUDA_CONSTEXPR auto equality_impl(DecimalType lhs, DecimalType rhs) noexcept -> bool
{
    // Step 1: Check for NANs per IEEE 754
    #ifndef BOOST_DECIMAL_FAST_MATH
    if (!isfinite(lhs) || !isfinite(rhs))
    {
        if (isnan(lhs) || isnan(rhs))
        {
            return false;
        }
        // The payload of an infinity has no meaning, so two infinities compare by sign
        if (isinf(lhs) && isinf(rhs))
        {
            return lhs.isneg() == rhs.isneg();
        }
    }
    #endif

    // Step 2: Fast path
    if (lhs.bits_ == rhs.bits_)
    {
        return true;
    }

    const auto lhs_components {lhs.to_components()};
    const auto rhs_components {rhs.to_components()};

    return equal_finite_parts<DecimalType>(lhs_components.sig, static_cast<int>(lhs_components.exp), lhs_components.sign,
                                           rhs_components.sig, static_cast<int>(rhs_components.exp), rhs_components.sign);
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif

template <BOOST_DECIMAL_FAST_DECIMAL_FLOATING_TYPE DecimalType>
BOOST_DECIMAL_FORCE_INLINE constexpr auto fast_equality_impl(const DecimalType& lhs, const DecimalType& rhs) noexcept -> bool
{
    #ifndef BOOST_DECIMAL_FAST_MATH
    if (isnan(lhs) || isnan(rhs))
    {
        return false;
    }
    #endif

    const auto lhs_sig {lhs.significand_};
    const auto rhs_sig {rhs.significand_};

    if (lhs_sig == 0U && rhs_sig == 0U)
    {
        // -0 == +0
        return true;
    }

    return  lhs_sig == rhs_sig &&
            lhs.exponent_ == rhs.exponent_ &&
            lhs.sign_ == rhs.sign_;
}

template <BOOST_DECIMAL_FAST_DECIMAL_FLOATING_TYPE DecimalType>
BOOST_DECIMAL_FORCE_INLINE constexpr auto fast_inequality_impl(const DecimalType& lhs, const DecimalType& rhs) noexcept -> bool
{
    return !fast_equality_impl(lhs, rhs);
}

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4127) // BOOST_DECIMAL_IF_CONSTEXPR prior to C++17 is constant conditional expression
#endif

// Compares the magnitudes sig * 10^exp and n exactly, and gives a negative, zero or positive value
template <BOOST_DECIMAL_INTEGRAL T1, BOOST_DECIMAL_INTEGRAL T2>
BOOST_DECIMAL_CUDA_CONSTEXPR auto compare_integer_magnitude(const T1 sig, const int exp, const T2 n) noexcept -> int
{
    using comp_type = std::conditional_t<(std::numeric_limits<T1>::digits > std::numeric_limits<T2>::digits), T1, T2>;

    BOOST_DECIMAL_IF_CONSTEXPR (std::numeric_limits<comp_type>::digits <= 64)
    {
        // A 32-bit value scaled by at most 10^9 fits in 64 bits, and a 64-bit value scaled by
        // at most 10^19 fits in 128 bits, so no digit count is needed
        using wide_type = std::conditional_t<(std::numeric_limits<comp_type>::digits <= 32), std::uint64_t, int128::uint128_t>;
        constexpr int max_scale {std::numeric_limits<comp_type>::digits <= 32 ? 9 : 19};

        if (exp >= -max_scale && exp <= max_scale)
        {
            wide_type lhs {static_cast<std::uint64_t>(sig)};
            wide_type rhs {static_cast<std::uint64_t>(n)};
            if (exp > 0)
            {
                lhs *= detail::pow10(static_cast<std::uint64_t>(exp));
            }
            else if (exp < 0)
            {
                rhs *= detail::pow10(static_cast<std::uint64_t>(-exp));
            }

            return static_cast<int>(lhs > rhs) - static_cast<int>(lhs < rhs);
        }
    }
    else
    {
        if (exp == 0)
        {
            return static_cast<int>(sig > n) - static_cast<int>(sig < n);
        }
    }

    if (sig == 0U || n == 0U)
    {
        return static_cast<int>(sig != 0U) - static_cast<int>(n != 0U);
    }

    // The positions of the leading digits decide first
    const auto lead_delta {detail::num_digits(sig) + exp - detail::num_digits(n)};
    if (lead_delta != 0)
    {
        return lead_delta;
    }

    const auto lhs {static_cast<comp_type>(sig)};
    auto rhs {static_cast<comp_type>(n)};

    if (exp < 0)
    {
        // The scaled n has the digits of sig, so it fits
        rhs *= detail::pow10(static_cast<comp_type>(-exp));
    }
    else if (exp > 0)
    {
        // The scaled sig can pass the largest value of the type, so n is divided instead
        const auto pow10_exp {detail::pow10(static_cast<comp_type>(exp))};
        const auto quotient {static_cast<comp_type>(rhs / pow10_exp)};
        if (lhs != quotient)
        {
            return lhs < quotient ? -1 : 1;
        }

        return quotient * pow10_exp == rhs ? 0 : -1;
    }

    return static_cast<int>(lhs > rhs) - static_cast<int>(lhs < rhs);
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif

template <BOOST_DECIMAL_DECIMAL_FLOATING_TYPE Decimal, BOOST_DECIMAL_INTEGRAL Integer>
BOOST_DECIMAL_CUDA_CONSTEXPR auto mixed_equality_impl(Decimal lhs, Integer rhs) noexcept
    -> std::enable_if_t<(detail::is_decimal_floating_point_v<Decimal> && detail::is_integral_v<Integer>), bool>
{
    #ifndef BOOST_DECIMAL_FAST_MATH
    if (isnan(lhs) || isinf(lhs))
    {
        return false;
    }
    #endif

    bool rhs_isneg {false};
    BOOST_DECIMAL_IF_CONSTEXPR (detail::is_signed_v<Integer>)
    {
        rhs_isneg = rhs < static_cast<Integer>(0);
    }

    const auto lhs_significand {lhs.full_significand()};
    const auto rhs_significand {detail::make_positive_unsigned(rhs)};

    // A value without an exponent needs no scale and no order
    if (lhs.biased_exponent() == 0)
    {
        // One test, because the result and the sign of the integer are random
        return static_cast<bool>(static_cast<unsigned>(lhs_significand == rhs_significand) &
                                 (static_cast<unsigned>(lhs.isneg() == rhs_isneg) | static_cast<unsigned>(lhs_significand == 0U)));
    }

    // +0 == -0, and other values also need the same sign
    return static_cast<bool>(static_cast<unsigned>(compare_integer_magnitude(lhs_significand, static_cast<int>(lhs.biased_exponent()), rhs_significand) == 0) &
                             (static_cast<unsigned>(lhs.isneg() == rhs_isneg) | static_cast<unsigned>(lhs_significand == 0U)));
}

namespace detail {

// The type with more digits holds both values, and an IEEE type also holds the subnormal values
template <typename Decimal1, typename Decimal2>
using mixed_comparison_type = std::conditional_t<(precision_v<Decimal1> > precision_v<Decimal2>) ||
    (precision_v<Decimal1> == precision_v<Decimal2> && is_ieee_type_v<Decimal1>), Decimal1, Decimal2>;

// lhs < rhs for two finite values that are not zero. Each argument is 0 or 1. If the signs
// differ, the negative operand is less. If not, the comparison of the magnitudes decides.
BOOST_DECIMAL_FORCE_INLINE constexpr auto less_by_signs(const unsigned lhs_negative, const unsigned signs_differ,
                                                        const unsigned less_for_equal_signs) noexcept -> bool
{
    return ((signs_differ & lhs_negative) | (~signs_differ & less_for_equal_signs & 1U)) != 0U;
}

} // namespace detail

template <BOOST_DECIMAL_DECIMAL_FLOATING_TYPE Decimal1, BOOST_DECIMAL_DECIMAL_FLOATING_TYPE Decimal2>
BOOST_DECIMAL_CUDA_CONSTEXPR auto mixed_decimal_equality_impl(Decimal1 lhs, Decimal2 rhs) noexcept
    -> std::enable_if_t<(detail::is_decimal_floating_point_v<Decimal1> &&
                         detail::is_decimal_floating_point_v<Decimal2>), bool>
{
    static_assert(!std::is_same<Decimal1, Decimal2>::value, "Equality of same type exists in simpler form");
    using Bigger_Decimal_Type = detail::mixed_comparison_type<Decimal1, Decimal2>;

    #ifndef BOOST_DECIMAL_FAST_MATH
    if (isnan(lhs) || isnan(rhs))
    {
        return false;
    }
    if (isinf(lhs) || isinf(rhs))
    {
        return isinf(lhs) && isinf(rhs) && lhs.isneg() == rhs.isneg();
    }
    #endif

    using sig_type = std::conditional_t<(detail::decimal_val_v<Bigger_Decimal_Type> < 64), std::uint32_t,
                     std::conditional_t<(detail::decimal_val_v<Bigger_Decimal_Type> < 128), std::uint64_t, int128::uint128_t>>;
    const auto lhs_sig {static_cast<sig_type>(lhs.full_significand())};
    const auto rhs_sig {static_cast<sig_type>(rhs.full_significand())};

    return equal_finite_parts<Bigger_Decimal_Type>(lhs_sig, static_cast<int>(lhs.biased_exponent()), lhs.isneg(),
                                                   rhs_sig, static_cast<int>(rhs.biased_exponent()), rhs.isneg());
}

template <BOOST_DECIMAL_DECIMAL_FLOATING_TYPE Decimal1, BOOST_DECIMAL_DECIMAL_FLOATING_TYPE Decimal2>
BOOST_DECIMAL_CUDA_CONSTEXPR auto operator==(Decimal1 lhs, Decimal2 rhs) noexcept
    -> std::enable_if_t<(detail::is_decimal_floating_point_v<Decimal1> &&
                         detail::is_decimal_floating_point_v<Decimal2>), bool>
{
    return mixed_decimal_equality_impl(lhs, rhs);
}

template <BOOST_DECIMAL_DECIMAL_FLOATING_TYPE Decimal1, BOOST_DECIMAL_DECIMAL_FLOATING_TYPE Decimal2>
BOOST_DECIMAL_CUDA_CONSTEXPR auto operator!=(Decimal1 lhs, Decimal2 rhs) noexcept
    -> std::enable_if_t<(detail::is_decimal_floating_point_v<Decimal1> &&
                         detail::is_decimal_floating_point_v<Decimal2>), bool>
{
    return !(mixed_decimal_equality_impl(lhs, rhs));
}

template <BOOST_DECIMAL_FAST_DECIMAL_FLOATING_TYPE DecimalType>
BOOST_DECIMAL_FORCE_INLINE constexpr auto fast_less_impl(const DecimalType& lhs, const DecimalType& rhs) noexcept -> bool
{
    // One test finds a zero or a value that is not finite in either operand, except in
    // two words, where the test costs more than the separate tests.
    using sig_type = decltype(lhs.significand_);
    constexpr auto max_sig {static_cast<sig_type>(detail::max_significand_v<DecimalType>)};
    if (detail::is_true(detail::decimal_val_v<DecimalType> >= 128) ||
        BOOST_DECIMAL_UNLIKELY((static_cast<unsigned>(static_cast<sig_type>(lhs.significand_ - 1U) >= max_sig) |
                                static_cast<unsigned>(static_cast<sig_type>(rhs.significand_ - 1U) >= max_sig)) != 0U))
    {
    #ifndef BOOST_DECIMAL_FAST_MATH
        if (not_finite(lhs) || not_finite(rhs))
        {
            if (isnan(lhs) || isnan(rhs) ||
                (!lhs.isneg() && rhs.isneg()))
            {
                return false;
            }
            else if (lhs.isneg() && !rhs.isneg())
            {
                return true;
            }
            else if (isfinite(lhs) && isinf(rhs))
            {
                return !signbit(rhs);
            }
            else if (isinf(lhs) && isfinite(rhs))
            {
                return signbit(rhs);
            }
        }
    #endif

        // Needed to correctly compare signed and unsigned zeros.
        // Every zero compares equal to every other zero, so neither of a pair of them is less
        // (IEEE 754-2008 5.11). Ordering -0 before +0 is the job of totalorder, not of operator<.
        if (lhs.significand_ == 0U || rhs.significand_ == 0U)
        {
            if (lhs.significand_ == 0U && rhs.significand_ == 0U)
            {
                return false;
            }
            return lhs.significand_ == 0U ? !rhs.sign_ : lhs.sign_;
        }
    }

    BOOST_DECIMAL_IF_CONSTEXPR (detail::is_true(detail::decimal_val_v<DecimalType> < 64 && sizeof(std::size_t) <= 4U))
    {
        // A target with words of 32 bits is faster with a key of one word. Each exponent has
        // 9 * 10^6 significands, and a target with longer words is faster with the shift below.
        constexpr auto max_sig32 {static_cast<std::uint32_t>(static_cast<std::uint64_t>(detail::max_significand_v<DecimalType>))};
        constexpr auto below_min_sig {max_sig32 / 10U};
        constexpr auto sig_count {static_cast<std::int32_t>(max_sig32 - below_min_sig)};
        static_assert(detail::decimal_val_v<DecimalType> >= 64 || detail::max_biased_exp_v<DecimalType> < INT32_MAX / sig_count, "The key is not a positive std::int32_t");
        const auto lhs_mask {-static_cast<std::int32_t>(lhs.sign_)};
        const auto rhs_mask {-static_cast<std::int32_t>(rhs.sign_)};
        const auto lhs_key {static_cast<std::int32_t>(lhs.exponent_) * sig_count + static_cast<std::int32_t>(static_cast<std::uint32_t>(static_cast<std::uint64_t>(lhs.significand_)) - below_min_sig)};
        const auto rhs_key {static_cast<std::int32_t>(rhs.exponent_) * sig_count + static_cast<std::int32_t>(static_cast<std::uint32_t>(static_cast<std::uint64_t>(rhs.significand_)) - below_min_sig)};
        return ((lhs_key ^ lhs_mask) - lhs_mask) < ((rhs_key ^ rhs_mask) - rhs_mask);
    }
    else BOOST_DECIMAL_IF_CONSTEXPR (detail::is_true(detail::decimal_val_v<DecimalType> < 128))
    {
        // Normalized values order by exponent and then significand, so one signed key holds the
        // whole order. The key of a nonzero value is positive, so its negation is less than it.
        constexpr auto below_min_sig {detail::pow10(static_cast<std::uint64_t>(detail::precision_v<DecimalType> > 16 ? 0 : detail::precision_v<DecimalType> - 1)) - 1U};
        // The significand part is from 1 to 9 * 10^(p - 1), and the exponent is in the bits above it
        constexpr int sig_bits {53};
        static_assert(9U * (below_min_sig + 1U) <= (UINT64_C(1) << sig_bits), "The significand part does not fit in the key");
        static_assert(detail::decimal_val_v<DecimalType> >= 128 || detail::max_biased_exp_v<DecimalType> < (1 << (63 - sig_bits)), "The key is not a positive std::int64_t");
        const auto lhs_mask {-static_cast<std::int64_t>(lhs.sign_)};
        const auto rhs_mask {-static_cast<std::int64_t>(rhs.sign_)};
        const auto lhs_key {(static_cast<std::int64_t>(lhs.exponent_) << sig_bits) + static_cast<std::int64_t>(static_cast<std::uint64_t>(lhs.significand_) - below_min_sig)};
        const auto rhs_key {(static_cast<std::int64_t>(rhs.exponent_) << sig_bits) + static_cast<std::int64_t>(static_cast<std::uint64_t>(rhs.significand_) - below_min_sig)};
        return ((lhs_key ^ lhs_mask) - lhs_mask) < ((rhs_key ^ rhs_mask) - rhs_mask);
    }
    else
    {
        // The same key in two words: the exponent sits above the 49 high bits of the significand
        constexpr int high_sig_bits {49};
        static_assert((int128::uint128_t{detail::max_significand_v<DecimalType>}.high >> high_sig_bits) == 0U, "The significand part does not fit in the key");
        static_assert(detail::decimal_val_v<DecimalType> < 128 || detail::max_biased_exp_v<DecimalType> < (1 << (64 - high_sig_bits)), "The exponent part does not fit in the key");
        const auto lhs_high {(static_cast<std::uint64_t>(lhs.exponent_) << high_sig_bits) + int128::uint128_t{lhs.significand_}.high};
        const auto rhs_high {(static_cast<std::uint64_t>(rhs.exponent_) << high_sig_bits) + int128::uint128_t{rhs.significand_}.high};
        const auto lhs_low {int128::uint128_t{lhs.significand_}.low};
        const auto rhs_low {int128::uint128_t{rhs.significand_}.low};
        const auto same_high {static_cast<unsigned>(lhs_high == rhs_high)};
        // Each value below is 0 or 1. The two magnitude values compare the keys before the signs apply.
        const auto magnitude_above {static_cast<unsigned>(lhs_high > rhs_high) | (same_high & static_cast<unsigned>(lhs_low > rhs_low))};
        const auto magnitude_below {static_cast<unsigned>(lhs_high < rhs_high) | (same_high & static_cast<unsigned>(lhs_low < rhs_low))};
        const auto lhs_negative {static_cast<unsigned>(lhs.sign_)};
        const auto signs_differ {lhs_negative ^ static_cast<unsigned>(rhs.sign_)};
        const auto less_for_equal_signs {(lhs_negative & magnitude_above) | (~lhs_negative & magnitude_below & 1U)};
        return detail::less_by_signs(lhs_negative, signs_differ, less_for_equal_signs);
    }
}

template <BOOST_DECIMAL_INTEGRAL T, BOOST_DECIMAL_INTEGRAL U>
BOOST_DECIMAL_FORCE_INLINE constexpr auto fast_type_less_parts_impl(T lhs_sig, U lhs_exp, bool lhs_sign,
                                                                    T rhs_sig, U rhs_exp, bool rhs_sign) noexcept -> bool
{
    // A pair of zeros compares equal no matter what their signs are (IEEE 754-2008 5.11)
    if (lhs_sig == 0U || rhs_sig == 0U)
    {
        if (lhs_sig == 0U && rhs_sig == 0U)
        {
            return false;
        }
        return lhs_sig == 0U ? !rhs_sign : lhs_sign;
    }

    if (lhs_sign != rhs_sign)
    {
        return lhs_sign;
    }

    if (lhs_exp != rhs_exp)
    {
        return lhs_sign ? lhs_exp > rhs_exp : lhs_exp < rhs_exp;
    }

    return lhs_sign ? lhs_sig > rhs_sig : lhs_sig < rhs_sig;
}

// The significands fit in the type of DecimalType, the type with more digits. They are
// references, because a copy of a 128-bit significand reloads it just after two 64-bit stores.
template <BOOST_DECIMAL_DECIMAL_FLOATING_TYPE DecimalType, typename SigType>
BOOST_DECIMAL_FORCE_INLINE BOOST_DECIMAL_CUDA_CONSTEXPR auto less_finite_parts(const SigType& lhs_sig, const int lhs_exp, const bool lhs_sign,
                                                                               const SigType& rhs_sig, const int rhs_exp, const bool rhs_sign) noexcept -> bool
{
    // Signed zeros: +0 == -0, neither is less.
    if (lhs_sig == 0U || rhs_sig == 0U)
    {
        if (lhs_sig == rhs_sig)
        {
            return false;
        }
        return lhs_sig == 0U ? !rhs_sign : lhs_sign;
    }

    const auto delta_exp {lhs_exp - rhs_exp};

    // A larger gap decides by the exponents alone. The narrow types scale the largest gaps whose
    // value (32 bits) or power (64 bits) fits a word, so a random gap costs no mispredicted branch.
    constexpr auto max_delta_diff {detail::decimal_val_v<DecimalType> < 64 ? 11 : (detail::decimal_val_v<DecimalType> < 128 ? 19 : 34)};

    if (BOOST_DECIMAL_UNLIKELY(delta_exp > max_delta_diff || delta_exp < -max_delta_diff))
    {
        if (lhs_sign != rhs_sign)
        {
            return lhs_sign;
        }
        return lhs_sign ? lhs_exp > rhs_exp : lhs_exp < rhs_exp;
    }

    BOOST_DECIMAL_IF_CONSTEXPR (detail::is_true(detail::decimal_val_v<DecimalType> < 64))
    {
        const auto lhs_scaled {static_cast<std::uint64_t>(lhs_sig) * detail::pow10(static_cast<std::uint64_t>(delta_exp > 0 ? delta_exp : 0))};
        const auto rhs_scaled {static_cast<std::uint64_t>(rhs_sig) * detail::pow10(static_cast<std::uint64_t>(delta_exp < 0 ? -delta_exp : 0))};
        const auto lhs_mask {-static_cast<std::int64_t>(lhs_sign)};
        const auto rhs_mask {-static_cast<std::int64_t>(rhs_sign)};
        return ((static_cast<std::int64_t>(lhs_scaled) ^ lhs_mask) - lhs_mask) < ((static_cast<std::int64_t>(rhs_scaled) ^ rhs_mask) - rhs_mask);
    }
    else
    {
        // One product of the significand with the larger exponent. Only the bits decide the order,
        // because random signs and exponents make a branch mispredict.
        const auto swapped {static_cast<unsigned>(delta_exp < 0)};
        const auto scale {swapped != 0U ? -delta_exp : delta_exp};
        // Each value below is 0 or 1. above and below compare the magnitude of the operand with the
        // larger exponent to the other magnitude, and flip says that above gives lhs < rhs.
        unsigned above {};
        unsigned below {};
        BOOST_DECIMAL_IF_CONSTEXPR (detail::is_true(detail::decimal_val_v<DecimalType> >= 128))
        {
            const int128::uint128_t smaller {detail::select_bits(swapped != 0U, lhs_sig, rhs_sig)};
            const int128::uint128_t larger {detail::select_bits(swapped != 0U, rhs_sig, lhs_sig)};
            const int128::uint128_t power {detail::pow10(static_cast<int128::uint128_t>(scale))};

            // The product of the two high words only says that the product does not fit
            const auto low {detail::umul128(larger.low, power.low)};
            const int128::uint128_t middle {detail::umul128(larger.low, power.high) + low.high};
            const int128::uint128_t upper {detail::umul128(larger.high, power.low) + middle.low};
            const int128::uint128_t low_product {upper.low, low.low};
            const auto overflow {static_cast<unsigned>((middle.high | upper.high) != 0U) |
                                 (static_cast<unsigned>(larger.high != 0U) & static_cast<unsigned>(power.high != 0U))};
            above = overflow | static_cast<unsigned>(low_product > smaller);
            below = static_cast<unsigned>(low_product < smaller) & (overflow ^ 1U);
        }
        else
        {
            const auto larger {static_cast<std::uint64_t>(swapped != 0U ? rhs_sig : lhs_sig)};
            const auto smaller {static_cast<std::uint64_t>(swapped != 0U ? lhs_sig : rhs_sig)};
            const auto product {detail::umul128(larger, detail::pow10(static_cast<std::uint64_t>(scale)))};
            above = static_cast<unsigned>(product.high != 0U) | static_cast<unsigned>(product.low > smaller);
            below = static_cast<unsigned>(product.high == 0U) & static_cast<unsigned>(product.low < smaller);
        }
        const auto flip {swapped ^ static_cast<unsigned>(lhs_sign)};
        const auto less_for_equal_signs {(flip & above) | (~flip & below & 1U)};
        return detail::less_by_signs(static_cast<unsigned>(lhs_sign), static_cast<unsigned>(lhs_sign != rhs_sign), less_for_equal_signs);
    }
}

template <BOOST_DECIMAL_DECIMAL_FLOATING_TYPE DecimalType>
BOOST_DECIMAL_FORCE_INLINE BOOST_DECIMAL_CUDA_CONSTEXPR auto sequential_less_impl(DecimalType lhs, DecimalType rhs) noexcept -> bool
{
    // Non-finite handling is performed by the callers. With NaN ruled out,
    // identical bit patterns encode the same finite value, so lhs < rhs is false.
    if (lhs.bits_ == rhs.bits_)
    {
        return false;
    }

    // One combination-field branch per operand instead of three.
    const auto lhs_components {lhs.to_components()};
    const auto rhs_components {rhs.to_components()};

    return less_finite_parts<DecimalType>(lhs_components.sig, static_cast<int>(lhs_components.exp), lhs_components.sign,
                                          rhs_components.sig, static_cast<int>(rhs_components.exp), rhs_components.sign);
}

template <BOOST_DECIMAL_DECIMAL_FLOATING_TYPE Decimal, BOOST_DECIMAL_INTEGRAL Integer>
BOOST_DECIMAL_CUDA_CONSTEXPR auto less_impl(Decimal lhs, Integer rhs) noexcept
    -> std::enable_if_t<(detail::is_decimal_floating_point_v<Decimal> && detail::is_integral_v<Integer>), bool>
{
    #ifndef BOOST_DECIMAL_FAST_MATH
    if (isnan(lhs))
    {
        return false;
    }
    else if (isinf(lhs))
    {
        return lhs.isneg();
    }
    #endif

    const auto lhs_significand {lhs.full_significand()};

    // A negative zero is not less than any integer that a positive zero is not less than,
    // so the sign shortcuts below have to stand aside when the decimal is a zero
    const bool lhs_is_zero {lhs_significand == 0U};

    bool lhs_sign {lhs.isneg()};
    bool rhs_sign {false};

    BOOST_DECIMAL_IF_CONSTEXPR (detail::is_signed_v<Integer>)
    {
        if (rhs < static_cast<Integer>(0))
        {
            rhs_sign = true;
        }

        if (!lhs_is_zero)
        {
            if (lhs_sign && !rhs_sign)
            {
                return true;
            }
            else if (!lhs_sign && rhs_sign)
            {
                return false;
            }
        }
    }
    else
    {
        if (lhs_sign && !lhs_is_zero)
        {
            return true;
        }
    }

    const auto rhs_significand {detail::make_positive_unsigned(rhs)};
    const auto magnitude {compare_integer_magnitude(lhs_significand, static_cast<int>(lhs.biased_exponent()), rhs_significand)};

    // The signs are equal here unless the decimal is a zero
    return rhs_sign ? magnitude > 0 : magnitude < 0;
}

template <BOOST_DECIMAL_DECIMAL_FLOATING_TYPE Decimal1, BOOST_DECIMAL_DECIMAL_FLOATING_TYPE Decimal2>
BOOST_DECIMAL_CUDA_CONSTEXPR auto mixed_decimal_less_impl(Decimal1 lhs, Decimal2 rhs) noexcept
    -> std::enable_if_t<(detail::is_decimal_floating_point_v<Decimal1> &&
                         detail::is_decimal_floating_point_v<Decimal2>), bool>
{
    using Bigger_Decimal_Type = detail::mixed_comparison_type<Decimal1, Decimal2>;

    #ifndef BOOST_DECIMAL_FAST_MATH
    if (!boost::decimal::isfinite(lhs) || !boost::decimal::isfinite(rhs))
    {
        if (isnan(lhs) || isnan(rhs))
        {
            return false;
        }
        if (lhs.isneg() != rhs.isneg())
        {
            return lhs.isneg();
        }

        // The types decode an infinity as different values
        return isinf(lhs) ? lhs.isneg() && !isinf(rhs) : !rhs.isneg();
    }
    #endif

    using sig_type = std::conditional_t<(detail::decimal_val_v<Bigger_Decimal_Type> < 128), std::uint64_t, int128::uint128_t>;
    const auto lhs_sig {static_cast<sig_type>(lhs.full_significand())};
    const auto rhs_sig {static_cast<sig_type>(rhs.full_significand())};

    return less_finite_parts<Bigger_Decimal_Type>(lhs_sig, static_cast<int>(lhs.biased_exponent()), lhs.isneg(),
                                                  rhs_sig, static_cast<int>(rhs.biased_exponent()), rhs.isneg());
}

template <BOOST_DECIMAL_DECIMAL_FLOATING_TYPE Decimal1, BOOST_DECIMAL_DECIMAL_FLOATING_TYPE Decimal2>
BOOST_DECIMAL_CUDA_CONSTEXPR auto operator<(Decimal1 lhs, Decimal2 rhs) noexcept
    -> std::enable_if_t<(detail::is_decimal_floating_point_v<Decimal1> &&
                         detail::is_decimal_floating_point_v<Decimal2>), bool>
{
    return mixed_decimal_less_impl(lhs, rhs);
}

template <BOOST_DECIMAL_DECIMAL_FLOATING_TYPE Decimal1, BOOST_DECIMAL_DECIMAL_FLOATING_TYPE Decimal2>
BOOST_DECIMAL_CUDA_CONSTEXPR auto operator<=(Decimal1 lhs, Decimal2 rhs) noexcept
    -> std::enable_if_t<(detail::is_decimal_floating_point_v<Decimal1> &&
                         detail::is_decimal_floating_point_v<Decimal2>), bool>
{
    #ifndef BOOST_DECIMAL_FAST_MATH
    if (isnan(lhs) || isnan(rhs))
    {
        return false;
    }
    #endif

    return !(rhs < lhs);
}

template <BOOST_DECIMAL_DECIMAL_FLOATING_TYPE Decimal1, BOOST_DECIMAL_DECIMAL_FLOATING_TYPE Decimal2>
BOOST_DECIMAL_CUDA_CONSTEXPR auto operator>(Decimal1 lhs, Decimal2 rhs) noexcept
    -> std::enable_if_t<(detail::is_decimal_floating_point_v<Decimal1> &&
                         detail::is_decimal_floating_point_v<Decimal2>), bool>
{
    return rhs < lhs;
}

template <BOOST_DECIMAL_DECIMAL_FLOATING_TYPE Decimal1, BOOST_DECIMAL_DECIMAL_FLOATING_TYPE Decimal2>
BOOST_DECIMAL_CUDA_CONSTEXPR auto operator>=(Decimal1 lhs, Decimal2 rhs) noexcept
    -> std::enable_if_t<(detail::is_decimal_floating_point_v<Decimal1> &&
                         detail::is_decimal_floating_point_v<Decimal2>), bool>
{
    #ifndef BOOST_DECIMAL_FAST_MATH
    if (isnan(lhs) || isnan(rhs))
    {
        return false;
    }
    #endif

    return !(lhs < rhs);
}

#ifdef BOOST_DECIMAL_HAS_SPACESHIP_OPERATOR

template <BOOST_DECIMAL_DECIMAL_FLOATING_TYPE Decimal1, BOOST_DECIMAL_DECIMAL_FLOATING_TYPE Decimal2>
BOOST_DECIMAL_CUDA_CONSTEXPR auto operator<=>(Decimal1 lhs, Decimal2 rhs) noexcept
    -> std::enable_if_t<(detail::is_decimal_floating_point_v<Decimal1> &&
                         detail::is_decimal_floating_point_v<Decimal2>), std::partial_ordering>
{
    if (lhs < rhs)
    {
        return std::partial_ordering::less;
    }
    else if (lhs > rhs)
    {
        return std::partial_ordering::greater;
    }
    else if (lhs == rhs)
    {
        return std::partial_ordering::equivalent;
    }

    return std::partial_ordering::unordered;
}

#endif

} //namespace decimal
} //namespace boost

#endif //BOOST_DECIMAL_DETAIL_COMPARISON_HPP
