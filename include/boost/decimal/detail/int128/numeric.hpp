// Copyright 2025 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_DECIMAL_DETAIL_INT128_NUMERIC_HPP
#define BOOST_DECIMAL_DETAIL_INT128_NUMERIC_HPP

#include <boost/decimal/detail/int128/bit.hpp>
#include <boost/decimal/detail/int128/cstdlib.hpp>
#include <boost/decimal/detail/int128/detail/traits.hpp>
#include <boost/decimal/detail/int128/utilities.hpp>

#ifndef BOOST_DECIMAL_DETAIL_INT128_BUILD_MODULE

#include <limits>
#include <utility>

#endif

namespace boost {
namespace int128 {

namespace detail {

template <typename IntegerType>
struct reduced_integers
{
    static constexpr bool value {std::is_same<IntegerType, signed char>::value ||
                                 std::is_same<IntegerType, unsigned char>::value ||
                                 std::is_same<IntegerType, signed short>::value ||
                                 std::is_same<IntegerType, unsigned short>::value ||
                                 std::is_same<IntegerType, signed int>::value ||
                                 std::is_same<IntegerType, unsigned int>::value ||
                                 std::is_same<IntegerType, signed long>::value ||
                                 std::is_same<IntegerType, unsigned long>::value ||
                                 std::is_same<IntegerType, signed long long>::value ||
                                 std::is_same<IntegerType, unsigned long long>::value ||
                                 std::is_same<IntegerType, int128>::value ||
                                 std::is_same<IntegerType, uint128>::value};
};

#if defined(BOOST_DECIMAL_DETAIL_INT128_HAS_INT128) || defined(BOOST_DECIMAL_DETAIL_INT128_HAS_MSVC_INT128)

template <typename IntegerType>
BOOST_DECIMAL_DETAIL_INT128_INLINE_CONSTEXPR bool is_reduced_integer_v {reduced_integers<IntegerType>::value ||
                                              std::is_same<IntegerType, detail::builtin_i128>::value ||
                                              std::is_same<IntegerType, detail::builtin_u128>::value};

#else

template <typename IntegerType>
BOOST_DECIMAL_DETAIL_INT128_INLINE_CONSTEXPR bool is_reduced_integer_v {reduced_integers<IntegerType>::value};

#endif // 128-bit

} // namespace detail

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128 saturating_add(const uint128 x, const uint128 y) noexcept
{
    const auto z {x + y};

    if (z < x)
    {
        return (std::numeric_limits<uint128>::max)();
    }

    return z;
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128 saturating_sub(const uint128 x, const uint128 y) noexcept
{
    const auto z {x - y};

    if (z > x)
    {
        return (std::numeric_limits<uint128>::min)();
    }

    return z;
}

#ifdef _MSC_VER
#  pragma warning(push)
#  pragma warning(disable : 4307) // Addition Overflow
#  pragma warning(disable : 4146) // Unary minus applied to unsigned type
#endif

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int128 saturating_add(const int128 x, const int128 y) noexcept
{
    // Detect overflow BEFORE the addition to avoid signed overflow UB.
    // When both are non-negative: overflow iff x > max - y (subtraction safe: max - non_negative >= 0)
    // When both are negative: overflow iff x < min - y (subtraction safe: min - negative > min)
    // Mixed signs: overflow is impossible.

    if (x.signed_high() >= 0 && y.signed_high() >= 0)
    {
        if (x > (std::numeric_limits<int128>::max)() - y)
        {
            return (std::numeric_limits<int128>::max)();
        }
    }
    else if (x.signed_high() < 0 && y.signed_high() < 0)
    {
        if (x < (std::numeric_limits<int128>::min)() - y)
        {
            return (std::numeric_limits<int128>::min)();
        }
    }

    return x + y;
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int128 saturating_sub(const int128 x, const int128 y) noexcept
{
    // Detect overflow BEFORE the subtraction to avoid signed overflow UB.
    // Positive overflow: x >= 0 and y < 0 and x > max + y (safe: max + negative < max)
    // Negative overflow: x < 0 and y >= 0 and x < min + y (safe: min + non_negative > min)
    // Same signs: overflow is impossible.

    if (x.signed_high() >= 0 && y.signed_high() < 0)
    {
        if (x > (std::numeric_limits<int128>::max)() + y)
        {
            return (std::numeric_limits<int128>::max)();
        }
    }
    else if (x.signed_high() < 0 && y.signed_high() >= 0)
    {
        if (x < (std::numeric_limits<int128>::min)() + y)
        {
            return (std::numeric_limits<int128>::min)();
        }
    }

    return x - y;
}

#ifdef _MSC_VER
#  pragma warning(pop)
#endif

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128 saturating_mul(const uint128 x, const uint128 y) noexcept
{
    uint128 res {};
    return ckd_mul(&res, x, y) ? (std::numeric_limits<uint128>::max)() : res;
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int128 saturating_mul(const int128 x, const int128 y) noexcept
{
    int128 res {};
    const auto overflowed {ckd_mul(&res, x, y)};

    if (overflowed)
    {
        return (x < 0) != (y < 0) ? (std::numeric_limits<int128>::min)() : (std::numeric_limits<int128>::max)();
    }

    return res;
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128 saturating_div(const uint128 x, const uint128 y) noexcept
{
    return x / y;
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int128 saturating_div(const int128 x, const int128 y) noexcept
{
    if (BOOST_DECIMAL_DETAIL_INT128_UNLIKELY(x == (std::numeric_limits<int128>::min)() && y == -1))
    {
        // This is the only possible case of overflow
        return (std::numeric_limits<int128>::max)();
    }

    return x / y;
}

#ifdef _MSC_VER
#  pragma warning(push)
#  pragma warning(disable: 4267)
#endif

BOOST_DECIMAL_DETAIL_INT128_EXPORT template <typename TargetType, std::enable_if_t<detail::is_reduced_integer_v<TargetType>, bool> = true>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr TargetType saturating_cast(const uint128 value) noexcept
{
    BOOST_DECIMAL_DETAIL_INT128_IF_CONSTEXPR (std::is_same<uint128, TargetType>::value)
    {
        return static_cast<TargetType>(value);
    }
    else
    {
        if (value > static_cast<uint128>((std::numeric_limits<TargetType>::max)()))
        {
            return (std::numeric_limits<TargetType>::max)();
        }

        return static_cast<TargetType>(value);
    }
}

#ifdef _MSC_VER
#  pragma warning(pop)
#endif

BOOST_DECIMAL_DETAIL_INT128_EXPORT template <typename TargetType, std::enable_if_t<detail::is_reduced_integer_v<TargetType>, bool> = true>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr TargetType saturating_cast(const int128 value) noexcept
{
    BOOST_DECIMAL_DETAIL_INT128_IF_CONSTEXPR (std::is_same<int128, TargetType>::value)
    {
        return static_cast<TargetType>(value);
    }
    #if defined(BOOST_DECIMAL_DETAIL_INT128_HAS_INT128) || defined(BOOST_DECIMAL_DETAIL_INT128_HAS_MSVC_INT128)
    else BOOST_DECIMAL_DETAIL_INT128_IF_CONSTEXPR (std::is_same<uint128, TargetType>::value || std::is_same<detail::builtin_u128, TargetType>::value)
    #else
    else BOOST_DECIMAL_DETAIL_INT128_IF_CONSTEXPR (std::is_same<uint128, TargetType>::value)
    #endif
    {
        // We can't possibly have overflow in this case
        return value < 0 ? static_cast<TargetType>(0) : static_cast<TargetType>(value);
    }
    else
    {
        if (value > static_cast<int128>((std::numeric_limits<TargetType>::max)()))
        {
            return (std::numeric_limits<TargetType>::max)();
        }
        else if (value < static_cast<int128>((std::numeric_limits<TargetType>::min)()))
        {
            return (std::numeric_limits<TargetType>::min)();
        }

        return static_cast<TargetType>(value);
    }
}

namespace detail {

BOOST_int128EST_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr std::uint64_t gcd64(std::uint64_t x, std::uint64_t y) noexcept
{
    if (x == 0)
    {
        return y;
    }
    if (y == 0)
    {
        return x;
    }

    const auto s {impl::countr_impl(x | y)};
    x >>= impl::countr_impl(x);

    do
    {
        y >>= impl::countr_impl(y);
        if (x > y)
        {
            const auto temp {x};
            x = y;
            y = temp;
        }

        y -= x;
    } while (y);

    return x << s;
}

} // namespace detail

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128 gcd(uint128 a, uint128 b) noexcept
{
    // Base case
    if (a == 0U)
    {
        return b;
    }
    if (b == 0U)
    {
        return a;
    }

    const auto a_zero {countr_zero(a)};
    const auto b_zero {countr_zero(b)};
    const auto shift {b_zero < a_zero ? b_zero : a_zero};
    a >>= shift;
    b >>= shift;

    do
    {
        b >>= countr_zero(b);

        if (a > b)
        {
            const uint128 temp {a};
            a = b;
            b = temp;
        }

        b -= a;
    } while (b != 0U && (a.high | b.high) > 0U);

    // Stop doing 128-bit math as soon as we can
    const auto g {detail::gcd64(a.low, b.low)};
    return uint128{0, g} << shift;
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int128 gcd(const int128 a, const int128 b) noexcept
{
    return static_cast<int128>(gcd(static_cast<uint128>(abs(a)), static_cast<uint128>(abs(b))));
}

// For unknown reasons this implementation fails for MSVC x86 only in release mode
// Directly calculating leads to the same failures, so unfortunately we have a viable,
// but very slow impl that we know works.
#if !(defined(_M_IX86) && !defined(_NDEBUG))

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128 lcm(const uint128 a, const uint128 b) noexcept
{
    if (a == 0U || b == 0U)
    {
        return static_cast<uint128>(0);
    }

    // Calculate GCD first
    const auto g {gcd(a, b)};

    // Compute LCM avoiding overflow: (a/gcd) * b
    return (a / g) * b;
}

#else

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128 lcm(uint128 a, uint128 b) noexcept
{
    if (a == 0U || b == 0U)
    {
        return uint128{0};
    }


    unsigned shift{};
    while ((a & 1U) == 0U && (b & 1U) == 0U) 
    {
        a >>= 1U;
        b >>= 1U;
        shift++;
    }

    // Ensure a >= b
    if (a < b)
    {
        std::swap(a, b);
    }

    uint128 lcm{a};

    while (lcm % b != 0U) 
    {
        lcm += a;
    }

    return lcm << shift;
}

#endif

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int128 lcm(const int128 a, const int128 b) noexcept
{
    return static_cast<int128>(lcm(static_cast<uint128>(abs(a)), static_cast<uint128>(abs(b))));
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128 midpoint(const uint128 a, const uint128 b) noexcept
{
    // Bit manipulation formula works for unsigned integers
    auto mid {(a & b) + ((a ^ b) >> 1)};

    // std::midpoint rounds towards the first parameter
    if ((a ^ b) & 1U && a > b)
    {
        ++mid;
    }

    return mid;
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int128 midpoint(const int128 a, const int128 b) noexcept
{
    // For signed integers, we use a + (b - a) / 2 or a - (a - b) / 2
    // The subtraction is done in unsigned arithmetic to handle overflow correctly
    // Integer division automatically rounds toward the first argument
    //
    // Use direct field access for both the uint128 construction and the
    // comparison to avoid NVCC host compiler issues with operator<= and
    // static_cast on int128 for large-magnitude values

    const uint128 ua {a.high, a.low};
    const uint128 ub {b.high, b.low};

    const bool a_le_b {a.high == b.high ? a.low <= b.low : a.signed_high() < b.signed_high()};

    if (a_le_b)
    {
        // diff = b - a (computed in unsigned, handles wrap-around correctly)
        const auto diff {ub - ua};
        return a + static_cast<int128>(diff / 2U);
    }
    else
    {
        // diff = a - b (computed in unsigned, handles wrap-around correctly)
        const auto diff {ua - ub};
        return a - static_cast<int128>(diff / 2U);
    }
}

// Quotient and remainder of a single division, following the div_result<T> proposed for the
// standard library by P3724 (Integer division)
BOOST_DECIMAL_DETAIL_INT128_EXPORT template <typename T>
struct div_result
{
    T quotient;
    T remainder;
};

BOOST_DECIMAL_DETAIL_INT128_EXPORT template <typename T>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr bool operator==(const div_result<T>& lhs, const div_result<T>& rhs) noexcept
{
    return lhs.quotient == rhs.quotient && lhs.remainder == rhs.remainder;
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT template <typename T>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr bool operator!=(const div_result<T>& lhs, const div_result<T>& rhs) noexcept
{
    return !(lhs == rhs);
}

#ifdef BOOST_DECIMAL_DETAIL_INT128_HAS_SPACESHIP_OPERATOR

BOOST_DECIMAL_DETAIL_INT128_EXPORT template <typename T>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr std::strong_ordering operator<=>(const div_result<T>& lhs, const div_result<T>& rhs) noexcept
{
    const auto quotient_order {lhs.quotient <=> rhs.quotient};
    return quotient_order != std::strong_ordering::equal ? quotient_order : lhs.remainder <=> rhs.remainder;
}

#endif // BOOST_DECIMAL_DETAIL_INT128_HAS_SPACESHIP_OPERATOR

namespace detail {

// -1 when the exact quotient of x / y is negative, and 1 otherwise
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int quotient_sign(const int128 x, const int128 y) noexcept
{
    return (x < 0) != (y < 0) ? -1 : 1;
}

// Applies the quotient offset d (-1, 0, or 1) to a truncated division result, and returns the
// remainder matching the adjusted quotient. The remainder is evaluated in unsigned arithmetic
// so that the d * y term cannot overflow when y is INT128_MIN.
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr div_result<int128> offset_quotient(const i128div_t truncated, const int128 y, const int d) noexcept
{
    const uint128 unsigned_rem {truncated.rem.high, truncated.rem.low};
    const uint128 unsigned_y {y.high, y.low};

    uint128 rem {unsigned_rem};

    if (d > 0)
    {
        rem = unsigned_rem - unsigned_y;
    }
    else if (d < 0)
    {
        rem = unsigned_rem + unsigned_y;
    }

    return div_result<int128>{truncated.quot + d, static_cast<int128>(rem)};
}

// An unsigned quotient is never rounded down, so the only offsets are 0 and 1. The remainder
// of an incremented quotient is negative, and is returned reduced modulo 2^128.
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr div_result<uint128> offset_quotient(const u128div_t truncated, const uint128 y, const bool increment) noexcept
{
    return div_result<uint128>{increment ? truncated.quot + 1U : truncated.quot,
                                 increment ? truncated.rem - y : truncated.rem};
}

// Round-to-nearest comparison shared by the ties functions: the truncated quotient grows in
// magnitude when the remainder is more than half the divisor. truncate_ties selects the strict
// form, which both breaks an exact tie towards zero and recovers the bit that abs(y) / 2 drops
// when y is odd.
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr bool nearest_increment(const uint128 abs_rem, const uint128 abs_half_y, const bool truncate_ties) noexcept
{
    return truncate_ties ? abs_rem > abs_half_y : abs_rem >= abs_half_y;
}

// Magnitude of the remainder of a truncated signed division. The magnitude is always less
// than abs(y), so it is representable for every valid divisor.
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128 abs_remainder(const i128div_t truncated) noexcept
{
    return static_cast<uint128>(abs(truncated.rem));
}

// floor(abs(y) / 2), exact for every y including INT128_MIN
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128 abs_half_divisor(const int128 y) noexcept
{
    return static_cast<uint128>(abs(y)) >> 1U;
}

// An odd divisor cannot produce an exact tie, so every ties function truncates on it
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr bool is_odd(const int128 x) noexcept
{
    return (x.low & 1U) != 0U;
}

BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr bool is_odd(const uint128 x) noexcept
{
    return (x.low & 1U) != 0U;
}

} // namespace detail

// Rounds towards zero, which is what operator/ already does
BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr div_result<uint128> div_rem_to_zero(const uint128 x, const uint128 y) noexcept
{
    const auto truncated {div(x, y)};
    return div_result<uint128>{truncated.quot, truncated.rem};
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr div_result<int128> div_rem_to_zero(const int128 x, const int128 y) noexcept
{
    const auto truncated {div(x, y)};
    return div_result<int128>{truncated.quot, truncated.rem};
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128 div_to_zero(const uint128 x, const uint128 y) noexcept
{
    return x / y;
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int128 div_to_zero(const int128 x, const int128 y) noexcept
{
    return x / y;
}

// Rounds away from zero, so the quotient grows in magnitude unless the division is exact
BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr div_result<uint128> div_rem_away_zero(const uint128 x, const uint128 y) noexcept
{
    const auto truncated {div(x, y)};
    return detail::offset_quotient(truncated, y, truncated.rem != 0U);
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr div_result<int128> div_rem_away_zero(const int128 x, const int128 y) noexcept
{
    const auto truncated {div(x, y)};
    return detail::offset_quotient(truncated, y, truncated.rem != 0 ? detail::quotient_sign(x, y) : 0);
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128 div_away_zero(const uint128 x, const uint128 y) noexcept
{
    return div_rem_away_zero(x, y).quotient;
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int128 div_away_zero(const int128 x, const int128 y) noexcept
{
    return div_rem_away_zero(x, y).quotient;
}

// Rounds towards positive infinity, which for an unsigned quotient is away from zero
BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr div_result<uint128> div_rem_to_pos_inf(const uint128 x, const uint128 y) noexcept
{
    const auto truncated {div(x, y)};
    return detail::offset_quotient(truncated, y, truncated.rem != 0U);
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr div_result<int128> div_rem_to_pos_inf(const int128 x, const int128 y) noexcept
{
    const auto truncated {div(x, y)};
    const auto adjust {truncated.rem != 0 && detail::quotient_sign(x, y) > 0};
    return detail::offset_quotient(truncated, y, adjust ? 1 : 0);
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128 div_to_pos_inf(const uint128 x, const uint128 y) noexcept
{
    return div_rem_to_pos_inf(x, y).quotient;
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int128 div_to_pos_inf(const int128 x, const int128 y) noexcept
{
    return div_rem_to_pos_inf(x, y).quotient;
}

// Rounds towards negative infinity, which for an unsigned quotient is truncation
BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr div_result<uint128> div_rem_to_neg_inf(const uint128 x, const uint128 y) noexcept
{
    const auto truncated {div(x, y)};
    return div_result<uint128>{truncated.quot, truncated.rem};
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr div_result<int128> div_rem_to_neg_inf(const int128 x, const int128 y) noexcept
{
    const auto truncated {div(x, y)};
    const auto adjust {truncated.rem != 0 && detail::quotient_sign(x, y) < 0};
    return detail::offset_quotient(truncated, y, adjust ? -1 : 0);
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128 div_to_neg_inf(const uint128 x, const uint128 y) noexcept
{
    return x / y;
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int128 div_to_neg_inf(const int128 x, const int128 y) noexcept
{
    return div_rem_to_neg_inf(x, y).quotient;
}

// Euclidean division, whose remainder is always in [0, abs(y)). Only a negative remainder
// needs fixing, and growing the quotient magnitude by one makes the remainder positive.
BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr div_result<uint128> div_rem_euclid(const uint128 x, const uint128 y) noexcept
{
    const auto truncated {div(x, y)};
    return div_result<uint128>{truncated.quot, truncated.rem};
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr div_result<int128> div_rem_euclid(const int128 x, const int128 y) noexcept
{
    const auto truncated {div(x, y)};
    return detail::offset_quotient(truncated, y, truncated.rem < 0 ? detail::quotient_sign(x, y) : 0);
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128 div_euclid(const uint128 x, const uint128 y) noexcept
{
    return x / y;
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int128 div_euclid(const int128 x, const int128 y) noexcept
{
    return div_rem_euclid(x, y).quotient;
}

// Rounds to nearest, breaking an exact tie towards zero
BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr div_result<uint128> div_rem_ties_to_zero(const uint128 x, const uint128 y) noexcept
{
    const auto truncated {div(x, y)};
    return detail::offset_quotient(truncated, y, detail::nearest_increment(truncated.rem, y >> 1U, true));
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr div_result<int128> div_rem_ties_to_zero(const int128 x, const int128 y) noexcept
{
    const auto truncated {div(x, y)};
    const auto increment {detail::nearest_increment(detail::abs_remainder(truncated), detail::abs_half_divisor(y), true)};
    return detail::offset_quotient(truncated, y, increment ? detail::quotient_sign(x, y) : 0);
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128 div_ties_to_zero(const uint128 x, const uint128 y) noexcept
{
    return div_rem_ties_to_zero(x, y).quotient;
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int128 div_ties_to_zero(const int128 x, const int128 y) noexcept
{
    return div_rem_ties_to_zero(x, y).quotient;
}

// Rounds to nearest, breaking an exact tie away from zero
BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr div_result<uint128> div_rem_ties_away_zero(const uint128 x, const uint128 y) noexcept
{
    const auto truncated {div(x, y)};
    return detail::offset_quotient(truncated, y, detail::nearest_increment(truncated.rem, y >> 1U, detail::is_odd(y)));
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr div_result<int128> div_rem_ties_away_zero(const int128 x, const int128 y) noexcept
{
    const auto truncated {div(x, y)};
    const auto increment {detail::nearest_increment(detail::abs_remainder(truncated), detail::abs_half_divisor(y), detail::is_odd(y))};
    return detail::offset_quotient(truncated, y, increment ? detail::quotient_sign(x, y) : 0);
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128 div_ties_away_zero(const uint128 x, const uint128 y) noexcept
{
    return div_rem_ties_away_zero(x, y).quotient;
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int128 div_ties_away_zero(const int128 x, const int128 y) noexcept
{
    return div_rem_ties_away_zero(x, y).quotient;
}

// Rounds to nearest, breaking an exact tie towards positive infinity. A tie only grows the
// magnitude when the quotient is positive.
BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr div_result<uint128> div_rem_ties_to_pos_inf(const uint128 x, const uint128 y) noexcept
{
    const auto truncated {div(x, y)};
    return detail::offset_quotient(truncated, y, detail::nearest_increment(truncated.rem, y >> 1U, detail::is_odd(y)));
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr div_result<int128> div_rem_ties_to_pos_inf(const int128 x, const int128 y) noexcept
{
    const auto truncated {div(x, y)};
    const auto sign {detail::quotient_sign(x, y)};
    const auto increment {detail::nearest_increment(detail::abs_remainder(truncated), detail::abs_half_divisor(y), detail::is_odd(y) || sign < 0)};
    return detail::offset_quotient(truncated, y, increment ? sign : 0);
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128 div_ties_to_pos_inf(const uint128 x, const uint128 y) noexcept
{
    return div_rem_ties_to_pos_inf(x, y).quotient;
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int128 div_ties_to_pos_inf(const int128 x, const int128 y) noexcept
{
    return div_rem_ties_to_pos_inf(x, y).quotient;
}

// Rounds to nearest, breaking an exact tie towards negative infinity. A tie only grows the
// magnitude when the quotient is negative, so an unsigned tie always truncates.
BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr div_result<uint128> div_rem_ties_to_neg_inf(const uint128 x, const uint128 y) noexcept
{
    const auto truncated {div(x, y)};
    return detail::offset_quotient(truncated, y, detail::nearest_increment(truncated.rem, y >> 1U, true));
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr div_result<int128> div_rem_ties_to_neg_inf(const int128 x, const int128 y) noexcept
{
    const auto truncated {div(x, y)};
    const auto sign {detail::quotient_sign(x, y)};
    const auto increment {detail::nearest_increment(detail::abs_remainder(truncated), detail::abs_half_divisor(y), detail::is_odd(y) || sign > 0)};
    return detail::offset_quotient(truncated, y, increment ? sign : 0);
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128 div_ties_to_neg_inf(const uint128 x, const uint128 y) noexcept
{
    return div_rem_ties_to_neg_inf(x, y).quotient;
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int128 div_ties_to_neg_inf(const int128 x, const int128 y) noexcept
{
    return div_rem_ties_to_neg_inf(x, y).quotient;
}

// Rounds to nearest, breaking an exact tie to the odd quotient, so a tie only grows the
// magnitude when truncation would have produced an even quotient
BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr div_result<uint128> div_rem_ties_to_odd(const uint128 x, const uint128 y) noexcept
{
    const auto truncated {div(x, y)};
    const auto increment {detail::nearest_increment(truncated.rem, y >> 1U, detail::is_odd(y) || detail::is_odd(truncated.quot))};
    return detail::offset_quotient(truncated, y, increment);
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr div_result<int128> div_rem_ties_to_odd(const int128 x, const int128 y) noexcept
{
    const auto truncated {div(x, y)};
    const auto truncate_ties {detail::is_odd(y) || detail::is_odd(truncated.quot)};
    const auto increment {detail::nearest_increment(detail::abs_remainder(truncated), detail::abs_half_divisor(y), truncate_ties)};
    return detail::offset_quotient(truncated, y, increment ? detail::quotient_sign(x, y) : 0);
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128 div_ties_to_odd(const uint128 x, const uint128 y) noexcept
{
    return div_rem_ties_to_odd(x, y).quotient;
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int128 div_ties_to_odd(const int128 x, const int128 y) noexcept
{
    return div_rem_ties_to_odd(x, y).quotient;
}

// Rounds to nearest, breaking an exact tie to the even quotient, so a tie only grows the
// magnitude when truncation would have produced an odd quotient
BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr div_result<uint128> div_rem_ties_to_even(const uint128 x, const uint128 y) noexcept
{
    const auto truncated {div(x, y)};
    const auto increment {detail::nearest_increment(truncated.rem, y >> 1U, detail::is_odd(y) || !detail::is_odd(truncated.quot))};
    return detail::offset_quotient(truncated, y, increment);
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr div_result<int128> div_rem_ties_to_even(const int128 x, const int128 y) noexcept
{
    const auto truncated {div(x, y)};
    const auto truncate_ties {detail::is_odd(y) || !detail::is_odd(truncated.quot)};
    const auto increment {detail::nearest_increment(detail::abs_remainder(truncated), detail::abs_half_divisor(y), truncate_ties)};
    return detail::offset_quotient(truncated, y, increment ? detail::quotient_sign(x, y) : 0);
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128 div_ties_to_even(const uint128 x, const uint128 y) noexcept
{
    return div_rem_ties_to_even(x, y).quotient;
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int128 div_ties_to_even(const int128 x, const int128 y) noexcept
{
    return div_rem_ties_to_even(x, y).quotient;
}

// The Euclidean remainder, which is always in [0, abs(y)). Only a negative remainder needs
// fixing, and abs(y) is added in unsigned arithmetic so that INT128_MIN is handled.
BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128 rem_euclid(const uint128 x, const uint128 y) noexcept
{
    return x % y;
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int128 rem_euclid(const int128 x, const int128 y) noexcept
{
    const auto rem {x % y};

    if (rem < 0)
    {
        const uint128 unsigned_rem {rem.high, rem.low};
        return static_cast<int128>(unsigned_rem + static_cast<uint128>(abs(y)));
    }

    return rem;
}

} // namespace int128
} // namespace boost

#endif // BOOST_DECIMAL_DETAIL_INT128_NUMERIC_HPP
