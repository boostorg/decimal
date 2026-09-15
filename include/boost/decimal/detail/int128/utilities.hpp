// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_DECIMAL_DETAIL_INT128_UTILITIES_HPP
#define BOOST_DECIMAL_DETAIL_INT128_UTILITIES_HPP

#include <boost/decimal/detail/int128/int128.hpp>
#include <boost/decimal/detail/int128/bit.hpp>
#include <boost/decimal/detail/int128/detail/config.hpp>

#ifndef BOOST_DECIMAL_DETAIL_INT128_BUILD_MODULE

#include <cstddef>
#include <cstdint>
#include <limits>
#include <type_traits>

#endif

namespace boost {
namespace int128 {

namespace detail {

// Modular addition for 128-bit operands assuming 0 <= a, b < m
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128 addmod(const uint128 a, const uint128 b, const uint128 m) noexcept
{
    const uint128 s {a + b};

    if (s < a || s >= m)
    {
        return s - m;
    }

    return s;
}

// Modular multiplication via shift-and-add for the full 128-bit modulus case
BOOST_int128EST_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128 mulmod_shift(uint128 a, uint128 b, const uint128 m) noexcept
{
    uint128 result {0};

    while (b != 0U)
    {
        if (static_cast<bool>(b.low & 1U))
        {
            result = addmod(result, a, m);
        }

        a = addmod(a, a, m);
        b >>= 1;
    }

    return result;
}

// Modular multiplication when the modulus fits in 64 bits
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr std::uint64_t mulmod_word(const std::uint64_t a, const std::uint64_t b, const std::uint64_t m) noexcept
{
    return ((uint128{a} * uint128{b}) % uint128{m}).low;
}

} // namespace detail

// Computes (base ^ exp) mod m using fast modular exponentiation with
// optimizations specific to the boost::int128 library types
BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128 powm(uint128 base, uint128 exp, const uint128 m) noexcept
{
    if (BOOST_DECIMAL_DETAIL_INT128_UNLIKELY(m == 0U))
    {
        return uint128{0};
    }

    if (m == 1U)
    {
        return uint128{0};
    }

    if (exp == 0U)
    {
        return uint128{1};
    }

    base %= m;

    if (base == 0U)
    {
        return uint128{0};
    }

    // Power-of-two modulus: reduction is just a bitmask.
    if (has_single_bit(m))
    {
        const uint128 mask {m - 1U};
        uint128 result {1};

        while (exp != 0U)
        {
            if (static_cast<bool>(exp.low & 1U))
            {
                result = (result * base) & mask;
            }

            base = (base * base) & mask;
            exp >>= 1;
        }

        return result;
    }

    // Modulus fits in 64 bits: stay in 64-bit lanes.
    if (m.high == 0U)
    {
        const auto mm {m.low};
        std::uint64_t result {1};
        auto b {base.low};

        while (exp != 0U)
        {
            if (static_cast<bool>(exp.low & 1U))
            {
                result = detail::mulmod_word(result, b, mm);
            }

            b = detail::mulmod_word(b, b, mm);
            exp >>= 1;
        }

        return uint128{result};
    }

    // General 128-bit modulus: shift-and-add for each squaring keeps every
    // intermediate strictly below m without needing a 256-bit product.
    uint128 result {1};

    while (exp != 0U)
    {
        if (static_cast<bool>(exp.low & 1U))
        {
            result = detail::mulmod_shift(result, base, m);
        }

        base = detail::mulmod_shift(base, base, m);
        exp >>= 1;
    }

    return result;
}

// Signed overload. Returns the non-negative residue in [0, m)
BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int128 powm(const int128 base, const int128 exp, const int128 m) noexcept
{
    if (BOOST_DECIMAL_DETAIL_INT128_UNLIKELY(m <= 0 || exp < 0))
    {
        return int128{0};
    }

    const uint128 um {static_cast<uint128>(m)};

    uint128 ub {};

    if (base.signed_high() < 0)
    {
        const uint128 magnitude {static_cast<uint128>(abs(base))};
        const uint128 r {magnitude % um};
        ub = r == 0U ? uint128{0} : um - r;
    }
    else
    {
        ub = static_cast<uint128>(base) % um;
    }

    return static_cast<int128>(powm(ub, static_cast<uint128>(exp), um));
}

// Computes base^exp using exponentiation by squaring. The result is reduced
// modulo 2^128, mirroring the wrap-around behavior of operator*.
BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128 ipow(uint128 base, std::uint64_t exp) noexcept
{
    uint128 result {1};

    while (exp != 0U)
    {
        if (static_cast<bool>(exp & 1U))
        {
            result *= base;
        }

        exp >>= 1;

        if (exp != 0U)
        {
            base *= base;
        }
    }

    return result;
}

// Signed overload. Wraps modulo 2^128 on overflow, matching operator*.
BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int128 ipow(int128 base, std::uint64_t exp) noexcept
{
    int128 result {1};

    while (exp != 0U)
    {
        if (static_cast<bool>(exp & 1U))
        {
            result *= base;
        }

        exp >>= 1;

        if (exp != 0U)
        {
            base *= base;
        }
    }

    return result;
}

// Integer square root: returns floor(sqrt(n)).
BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128 isqrt(const uint128 n) noexcept
{
    if (n < 2U)
    {
        return n;
    }

    // 2^ceil(bit_width(n)/2) is the smallest power of two whose square exceeds n.
    uint128 x {uint128{1} << ((bit_width(n) + 1) / 2)};

    while (true)
    {
        const uint128 y {(x + n / x) >> 1};

        if (y >= x)
        {
            return x;
        }

        x = y;
    }
}

// Signed overload. Negative inputs are documented to return 0.
BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int128 isqrt(const int128 n) noexcept
{
    if (BOOST_DECIMAL_DETAIL_INT128_UNLIKELY(n < 0))
    {
        return int128{0};
    }

    return static_cast<int128>(isqrt(static_cast<uint128>(n)));
}

namespace detail {

// The C23 checked integer macros accept any integer type for their operands
// except bool, plain char, enumerated types, and bit-precise (_BitInt) types.
template <typename T>
struct valid_checked_type : std::integral_constant<bool, std::is_integral<T>::value &&
                                                         !std::is_same<T, bool>::value &&
                                                         !std::is_same<T, char>::value> {};

template <>
struct valid_checked_type<int128> : std::true_type {};

template <>
struct valid_checked_type<uint128> : std::true_type {};

// Widen an integer operand to its 128-bit two's complement bit pattern, returned as a uint128
template <typename T>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128 ckd_widen(const T value) noexcept
{
    BOOST_DECIMAL_DETAIL_INT128_IF_CONSTEXPR (std::numeric_limits<T>::is_signed)
    {
        return static_cast<uint128>(static_cast<int128>(value));
    }
    else
    {
        return static_cast<uint128>(value);
    }
}

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4324)
#endif

// Sign and magnitude of an operand together with its 128-bit two's complement
// image. magnitude is the absolute value; negative records the sign.
struct ckd_operand
{
    uint128 raw;
    uint128 magnitude;
    bool negative;
};

// Exact signed sum of two operands given as (magnitude, sign). carry marks a
// 129th bit, which no 128-bit or narrower target can represent.
struct ckd_sum_result
{
    uint128 magnitude;
    bool negative;
    bool carry;
};

#ifdef _MSC_VER
#pragma warning(pop)
#endif

template <typename T>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr ckd_operand ckd_decompose(const T value) noexcept
{
    const uint128 raw {ckd_widen(value)};
    const bool negative {std::numeric_limits<T>::is_signed && ((raw >> 127) != 0U)};
    return ckd_operand{raw, negative ? uint128{0} - raw : raw, negative};
}

BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr ckd_sum_result ckd_signed_sum(const uint128 a_magnitude, const bool a_negative,
                                                                 const uint128 b_magnitude, const bool b_negative) noexcept
{
    if (a_negative == b_negative)
    {
        // Equal signs: magnitudes add and may overflow into a 129th bit.
        const uint128 magnitude {a_magnitude + b_magnitude};
        return ckd_sum_result{magnitude, a_negative, magnitude < a_magnitude};
    }

    // Opposite signs: the smaller magnitude is subtracted and never carries.
    if (a_magnitude >= b_magnitude)
    {
        return ckd_sum_result{a_magnitude - b_magnitude, a_negative, false};
    }

    return ckd_sum_result{b_magnitude - a_magnitude, b_negative, false};
}

// Whether a result of the given sign and magnitude fits in T1. exceeds_width
// forces overflow when the true magnitude does not even fit in 128 bits.
template <typename T1>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr bool ckd_overflows(const uint128 magnitude, const bool negative, const bool exceeds_width) noexcept
{
    if (exceeds_width)
    {
        return true;
    }

    const uint128 max_magnitude {static_cast<uint128>((std::numeric_limits<T1>::max)())};

    if (negative)
    {
        const uint128 min_magnitude {std::numeric_limits<T1>::is_signed ? max_magnitude + uint128{1} : uint128{0}};
        return magnitude > min_magnitude;
    }

    return magnitude > max_magnitude;
}

} // namespace detail

// Checked addition following the C23 <stdckdint.h> ckd_add contract.
//
// Computes a + b as if both operands were represented in a signed integer
// type of infinite range and then converts that exact result to the type
// pointed to by result. *result always receives the exact result wrapped
// around to the width of *result. Returns false when *result represents the
// exact mathematical sum, and true when the sum did not fit and wrap-around
// occurred.
BOOST_DECIMAL_DETAIL_INT128_EXPORT template <typename T1, typename T2, typename T3>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr bool ckd_add(T1* result, const T2 a, const T3 b) noexcept
{
    static_assert(detail::valid_checked_type<T1>::value &&
                  detail::valid_checked_type<T2>::value &&
                  detail::valid_checked_type<T3>::value,
                  "ckd_add operands must be integer types other than bool and plain char.");

    const auto op_a {detail::ckd_decompose(a)};
    const auto op_b {detail::ckd_decompose(b)};

    // The modular sum of the widened images is the exact sum mod 2^128, which
    // is all the wrapped result needs for any target no wider than 128 bits.
    *result = static_cast<T1>(op_a.raw + op_b.raw);

    const auto sum {detail::ckd_signed_sum(op_a.magnitude, op_a.negative, op_b.magnitude, op_b.negative)};
    return detail::ckd_overflows<T1>(sum.magnitude, sum.negative, sum.carry);
}

// Checked subtraction following the C23 <stdckdint.h> ckd_sub contract.
//
// Behaves as ckd_add for a - b: *result receives the exact difference wrapped
// to its width, and the return value reports whether that difference did not
// fit.
BOOST_DECIMAL_DETAIL_INT128_EXPORT template <typename T1, typename T2, typename T3>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr bool ckd_sub(T1* result, const T2 a, const T3 b) noexcept
{
    static_assert(detail::valid_checked_type<T1>::value &&
                  detail::valid_checked_type<T2>::value &&
                  detail::valid_checked_type<T3>::value,
                  "ckd_sub operands must be integer types other than bool and plain char.");

    const auto op_a {detail::ckd_decompose(a)};
    const auto op_b {detail::ckd_decompose(b)};

    *result = static_cast<T1>(op_a.raw - op_b.raw);

    // a - b is a + (-b): negating b flips its sign while keeping its magnitude.
    const auto difference {detail::ckd_signed_sum(op_a.magnitude, op_a.negative, op_b.magnitude, !op_b.negative)};
    return detail::ckd_overflows<T1>(difference.magnitude, difference.negative, difference.carry);
}

// Checked multiplication following the C23 <stdckdint.h> ckd_mul contract.
//
// Computes a * b as if both operands had infinite range, stores the result
// wrapped to the width of *result, and returns true when the exact product did
// not fit.
BOOST_DECIMAL_DETAIL_INT128_EXPORT template <typename T1, typename T2, typename T3>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr bool ckd_mul(T1* result, const T2 a, const T3 b) noexcept
{
    static_assert(detail::valid_checked_type<T1>::value &&
                  detail::valid_checked_type<T2>::value &&
                  detail::valid_checked_type<T3>::value,
                  "ckd_mul operands must be integer types other than bool and plain char.");

    const auto op_a {detail::ckd_decompose(a)};
    const auto op_b {detail::ckd_decompose(b)};

    *result = static_cast<T1>(op_a.raw * op_b.raw);

    // The product magnitude needs more than 128 bits exactly when it exceeds
    // UINT128_MAX. Dividing the maximum by one magnitude tests that without
    // forming a 256-bit product.
    const bool exceeds_width {op_a.magnitude != 0U &&
                              op_b.magnitude > ((std::numeric_limits<uint128>::max)() / op_a.magnitude)};

    const uint128 product_magnitude {op_a.magnitude * op_b.magnitude};
    const bool product_negative {op_a.negative != op_b.negative};

    return detail::ckd_overflows<T1>(product_magnitude, product_negative, exceeds_width);
}

namespace detail {

// See: https://eel.is/c++draft/utility.intcmp
// [Note 1: These function templates cannot be used to compare byte, char, char8_t, char16_t, char32_t, wchar_t, and bool. end note]
template <typename T>
struct valid_comparison_type
{
    static constexpr bool value = std::is_integral<T>::value &&
                                  !std::is_same<T, char>::value &&
                                      !std::is_same<T, char16_t>::value &&
                                          !std::is_same<T, char32_t>::value &&
                                              !std::is_same<T, wchar_t>::value &&
                                                  !std::is_same<T, bool>::value
                                                    #if defined(__cpp_char8_t)
                                                    && !std::is_same<T, char8_t>::value
                                                    #endif
                                                    #if defined(__cpp_lib_byte) && __cpp_lib_byte >= 201603L
                                                    && !std::is_same<T, std::byte>::value
                                                    #endif
    ;
};

template <typename T>
BOOST_DECIMAL_DETAIL_INT128_INLINE_CONSTEXPR bool is_valid_comparison_type_v = valid_comparison_type<T>::value;

// Allow the builtins to be used when available
template <typename T>
BOOST_DECIMAL_DETAIL_INT128_INLINE_CONSTEXPR bool is_int128_type_v = std::is_same<T, int128>::value ||
                                                      std::is_same<T, uint128>::value
    #if defined(BOOST_DECIMAL_DETAIL_INT128_HAS_INT128) || defined(BOOST_DECIMAL_DETAIL_INT128_HAS_MSVC_INT128)
                                                      || std::is_same<T, builtin_i128>::value
                                                      || std::is_same<T, builtin_u128>::value
    #endif
                                                      ;

template <typename T>
BOOST_DECIMAL_DETAIL_INT128_INLINE_CONSTEXPR bool is_valid_comparison_operand_v = is_valid_comparison_type_v<T> ||
                                                                   is_int128_type_v<T>;

// Maps the builtin 128-bit types onto the library equivalents
template <typename T>
struct comparison_canonical
{
    using type = T;
};

#if defined(BOOST_DECIMAL_DETAIL_INT128_HAS_INT128) || defined(BOOST_DECIMAL_DETAIL_INT128_HAS_MSVC_INT128)

template <>
struct comparison_canonical<builtin_i128>
{
    using type = int128;
};

template <>
struct comparison_canonical<builtin_u128>
{
    using type = uint128;
};

#endif

template <typename T>
using comparison_canonical_t = typename comparison_canonical<T>::type;

template <typename T>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr comparison_canonical_t<T> canonical_comparison_operand(const T value) noexcept
{
    return static_cast<comparison_canonical_t<T>>(value);
}

// Mathematical equality of two integers regardless of their signedness, via the
// same (sign, magnitude) decomposition.
template <typename T, typename U>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr bool cmp_equal_impl(const T lhs, const U rhs) noexcept
{
    const auto a {ckd_decompose(canonical_comparison_operand(lhs))};
    const auto b {ckd_decompose(canonical_comparison_operand(rhs))};

    return (a.negative == b.negative) && (a.magnitude == b.magnitude);
}

// Mathematical less-than of two integers regardless of their signedness, via the
// same (sign, magnitude) decomposition.
template <typename T, typename U>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr bool cmp_less_impl(const T lhs, const U rhs) noexcept
{
    const auto a {ckd_decompose(canonical_comparison_operand(lhs))};
    const auto b {ckd_decompose(canonical_comparison_operand(rhs))};

    if (a.negative != b.negative)
    {
        return a.negative;
    }

    return a.negative ? (a.magnitude > b.magnitude) : (a.magnitude < b.magnitude);
}

template <typename T, typename U>
BOOST_DECIMAL_DETAIL_INT128_INLINE_CONSTEXPR bool enable_comparison_v = is_valid_comparison_operand_v<T> &&
                                                        is_valid_comparison_operand_v<U> &&
                                                        (is_int128_type_v<T> || is_int128_type_v<U>);

} // namespace detail

// C++26 integer comparison functions (https://eel.is/c++draft/utility.intcmp)
// extended to the library and builtin 128-bit types and available from C++14.

BOOST_DECIMAL_DETAIL_INT128_EXPORT template <typename T, typename U, std::enable_if_t<detail::enable_comparison_v<T, U>, bool> = true>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr bool cmp_equal(const T lhs, const U rhs) noexcept
{
    return detail::cmp_equal_impl(lhs, rhs);
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT template <typename T, typename U, std::enable_if_t<detail::enable_comparison_v<T, U>, bool> = true>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr bool cmp_not_equal(const T lhs, const U rhs) noexcept
{
    return !detail::cmp_equal_impl(lhs, rhs);
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT template <typename T, typename U, std::enable_if_t<detail::enable_comparison_v<T, U>, bool> = true>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr bool cmp_less(const T lhs, const U rhs) noexcept
{
    return detail::cmp_less_impl(lhs, rhs);
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT template <typename T, typename U, std::enable_if_t<detail::enable_comparison_v<T, U>, bool> = true>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr bool cmp_greater(const T lhs, const U rhs) noexcept
{
    return detail::cmp_less_impl(rhs, lhs);
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT template <typename T, typename U, std::enable_if_t<detail::enable_comparison_v<T, U>, bool> = true>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr bool cmp_less_equal(const T lhs, const U rhs) noexcept
{
    return !detail::cmp_less_impl(rhs, lhs);
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT template <typename T, typename U, std::enable_if_t<detail::enable_comparison_v<T, U>, bool> = true>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr bool cmp_greater_equal(const T lhs, const U rhs) noexcept
{
    return !detail::cmp_less_impl(lhs, rhs);
}

// Whether t is representable in the target type R.
BOOST_DECIMAL_DETAIL_INT128_EXPORT template <typename R, typename T, std::enable_if_t<detail::enable_comparison_v<R, T>, bool> = true>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr bool in_range(const T t) noexcept
{
    using limits = std::numeric_limits<detail::comparison_canonical_t<R>>;

    return !detail::cmp_less_impl(t, (limits::min)()) &&
           !detail::cmp_less_impl((limits::max)(), t);
}

} // namespace int128
} // namespace boost

#endif // BOOST_DECIMAL_DETAIL_INT128_UTILITIES_HPP
