// Copyright 2025 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_DECIMAL_DETAIL_INT128_DETAIL_UINT128_IMP_HPP
#define BOOST_DECIMAL_DETAIL_INT128_DETAIL_UINT128_IMP_HPP

#include <boost/decimal/detail/int128/detail/fwd.hpp>
#include <boost/decimal/detail/int128/detail/config.hpp>
#include <boost/decimal/detail/int128/detail/traits.hpp>
#include <boost/decimal/detail/int128/detail/constants.hpp>
#include <boost/decimal/detail/int128/detail/clz.hpp>
#include <boost/decimal/detail/int128/detail/common_mul.hpp>
#include <boost/decimal/detail/int128/detail/common_div.hpp>
#include <boost/decimal/detail/int128/detail/float_conversion.hpp>

#ifndef BOOST_DECIMAL_DETAIL_INT128_BUILD_MODULE

#include <cstdint>
#include <cstring>
#include <climits>

#endif

namespace boost {
namespace int128 {

BOOST_DECIMAL_DETAIL_INT128_EXPORT struct
    #if (defined(BOOST_DECIMAL_DETAIL_INT128_HAS_INT128) || defined(BOOST_DECIMAL_DETAIL_INT128_HAS_MSVC_INT128)) && !defined(_M_IX86)
    alignas(alignof(detail::builtin_u128))
    #endif
uint128
{
    #if BOOST_DECIMAL_DETAIL_INT128_ENDIAN_LITTLE_BYTE
    std::uint64_t low {};
    std::uint64_t high {};
    #else

    #ifdef __GNUC__
    #  pragma GCC diagnostic push
    #  pragma GCC diagnostic ignored "-Wreorder"
    #endif

    std::uint64_t high {};
    std::uint64_t low {};

    #ifdef __GNUC__
    #  pragma GCC diagnostic pop
    #endif

    #endif // BOOST_DECIMAL_DETAIL_INT128_ENDIAN_LITTLE_BYTE

    // Defaulted basic construction
    constexpr uint128() noexcept = default;
    constexpr uint128(const uint128&) noexcept = default;
    constexpr uint128(uint128&&) noexcept = default;
    constexpr uint128& operator=(const uint128&) noexcept = default;
    constexpr uint128& operator=(uint128&&) noexcept = default;

    // Requires a conversion file to be implemented
    BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128(const int128& v) noexcept;

    // Construct from integral types
    #if BOOST_DECIMAL_DETAIL_INT128_ENDIAN_LITTLE_BYTE

    BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128(const std::uint64_t hi, const std::uint64_t lo) noexcept : low {lo}, high {hi} {}

    template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_SIGNED_INTEGER_CONCEPT>
    BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128(const SignedInteger v) noexcept : low {static_cast<std::uint64_t>(v)}, high {v < 0 ? UINT64_MAX : UINT64_C(0)} {}

    template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_UNSIGNED_INTEGER_CONCEPT>
    BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128(const UnsignedInteger v) noexcept : low {static_cast<std::uint64_t>(v)}, high {} {}

    #if defined(BOOST_DECIMAL_DETAIL_INT128_HAS_INT128) || defined(BOOST_DECIMAL_DETAIL_INT128_HAS_MSVC_INT128)

    BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_BUILTIN_CONSTEXPR uint128(const detail::builtin_i128 v) noexcept :
        low {static_cast<std::uint64_t>(v)},
        high {static_cast<std::uint64_t>(static_cast<detail::builtin_u128>(v) >> static_cast<detail::builtin_u128>(64U))} {}

    BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_BUILTIN_CONSTEXPR uint128(const detail::builtin_u128 v) noexcept :
        low {static_cast<std::uint64_t>(v)},
        high {static_cast<std::uint64_t>(v >> static_cast<detail::builtin_i128>(64U))} {}

    #endif // BOOST_DECIMAL_DETAIL_INT128_HAS_INT128

    #else // Big endian

    BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128(const std::uint64_t hi, const std::uint64_t lo) noexcept : high {hi}, low {lo} {}

    template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_SIGNED_INTEGER_CONCEPT>
    BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128(const SignedInteger v) noexcept : high {v < 0 ? UINT64_MAX : UINT64_C(0)}, low {static_cast<std::uint64_t>(v)} {}

    template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_UNSIGNED_INTEGER_CONCEPT>
    BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128(const UnsignedInteger v) noexcept : high {}, low {static_cast<std::uint64_t>(v)} {}

    #if defined(BOOST_DECIMAL_DETAIL_INT128_HAS_INT128) || defined(BOOST_DECIMAL_DETAIL_INT128_HAS_MSVC_INT128)

    BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_BUILTIN_CONSTEXPR uint128(const detail::builtin_i128 v) noexcept :
        high {static_cast<std::uint64_t>(static_cast<detail::builtin_u128>(v) >> 64U)},
        low {static_cast<std::uint64_t>(v)} {}

    BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_BUILTIN_CONSTEXPR uint128(const detail::builtin_u128 v) noexcept :
        high {static_cast<std::uint64_t>(v >> 64U)},
        low {static_cast<std::uint64_t>(v)} {}

    #endif // BOOST_DECIMAL_DETAIL_INT128_HAS_INT128

    #endif // BOOST_DECIMAL_DETAIL_INT128_ENDIAN_LITTLE_BYTE

    // Construct from floating-point types
    template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_FLOATING_POINT_CONCEPT>
    BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128(Float f) noexcept;

    // Integer conversion operators
    BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE explicit constexpr operator bool() const noexcept {return low || high; }

    template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_SIGNED_INTEGER_CONCEPT>
    BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr operator SignedInteger() const noexcept { return static_cast<SignedInteger>(low); }

    #ifdef _MSC_VER
    #  pragma warning(push)
    #  pragma warning(disable:4127)
    #endif

    template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_UNSIGNED_INTEGER_CONCEPT>
    BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr operator UnsignedInteger() const noexcept
    {
        BOOST_DECIMAL_DETAIL_INT128_IF_CONSTEXPR (std::is_same<UnsignedInteger, bool>::value)
        {
            return low || high;
        }
        else
        {
            return static_cast<UnsignedInteger>(low);
        }
    }

    #ifdef _MSC_VER
    #  pragma warning(pop)
    #endif

    #if defined(BOOST_DECIMAL_DETAIL_INT128_HAS_INT128) || defined(BOOST_DECIMAL_DETAIL_INT128_HAS_MSVC_INT128)

    BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_BUILTIN_CONSTEXPR operator detail::builtin_i128() const noexcept { return static_cast<detail::builtin_i128>(static_cast<detail::builtin_u128>(high) << static_cast<detail::builtin_u128>(64)) | static_cast<detail::builtin_i128>(low); }

    BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_BUILTIN_CONSTEXPR operator detail::builtin_u128() const noexcept { return (static_cast<detail::builtin_u128>(high) << static_cast<detail::builtin_u128>(64)) | static_cast<detail::builtin_u128>(low); }

    #endif // BOOST_DECIMAL_DETAIL_INT128_HAS_INT128

    // Conversion to float
    // Uses the builtin 128-bit conversion where one exists, and otherwise composes
    // the words as high * 2^64 + low with an exact 2^64 constant, which is the value
    // ldexp(static_cast<T>(high), 64) + static_cast<T>(low) computes
    BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr operator float() const noexcept;
    BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr operator double() const noexcept;

    // long doubles do not exist on the CUDA or SYCL (spir64) device
    #if !defined(BOOST_DECIMAL_DETAIL_INT128_HAS_GPU_SUPPORT)
    constexpr operator long double() const noexcept;
    #endif

    // Compound OR
    template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_INTEGER_CONCEPT>
    BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128& operator|=(Integer rhs) noexcept;

    BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128& operator|=(uint128 rhs) noexcept;

    #ifdef BOOST_DECIMAL_DETAIL_INT128_HAS_MSVC_INT128

    template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_128BIT_INTEGER_CONCEPT>
    BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE inline uint128& operator|=(Integer rhs) noexcept;

    #endif // BOOST_DECIMAL_DETAIL_INT128_HAS_MSVC_INT128

    // Compound AND
    template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_INTEGER_CONCEPT>
    BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128& operator&=(Integer rhs) noexcept;

    BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128& operator&=(uint128 rhs) noexcept;

    #ifdef BOOST_DECIMAL_DETAIL_INT128_HAS_MSVC_INT128

    template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_128BIT_INTEGER_CONCEPT>
    BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE inline uint128& operator&=(Integer rhs) noexcept;

    #endif // BOOST_DECIMAL_DETAIL_INT128_HAS_MSVC_INT128

    // Compound XOR
    template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_INTEGER_CONCEPT>
    BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128& operator^=(Integer rhs) noexcept;

    BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128& operator^=(uint128 rhs) noexcept;

    #ifdef BOOST_DECIMAL_DETAIL_INT128_HAS_MSVC_INT128

    template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_128BIT_INTEGER_CONCEPT>
    BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE inline uint128& operator^=(Integer rhs) noexcept;

    #endif // BOOST_DECIMAL_DETAIL_INT128_HAS_MSVC_INT128

    // Compound Left Shift
    template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_INTEGER_CONCEPT>
    BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128& operator<<=(Integer rhs) noexcept;

    BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128& operator<<=(uint128 rhs) noexcept;

    #ifdef BOOST_DECIMAL_DETAIL_INT128_HAS_MSVC_INT128

    template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_128BIT_INTEGER_CONCEPT>
    BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE inline uint128& operator<<=(Integer rhs) noexcept;

    #endif // BOOST_DECIMAL_DETAIL_INT128_HAS_MSVC_INT128

    // Compound Right Shift
    template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_INTEGER_CONCEPT>
    BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128& operator>>=(Integer rhs) noexcept;

    BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128& operator>>=(uint128 rhs) noexcept;

    #ifdef BOOST_DECIMAL_DETAIL_INT128_HAS_MSVC_INT128

    template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_128BIT_INTEGER_CONCEPT>
    BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE inline uint128& operator>>=(Integer rhs) noexcept;

    #endif // BOOST_DECIMAL_DETAIL_INT128_HAS_MSVC_INT128

    BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128& operator++() noexcept;
    BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128 operator++(int) noexcept;
    BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128& operator--() noexcept;
    BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128 operator--(int) noexcept;

    // Compound Addition
    template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_INTEGER_CONCEPT>
    BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128& operator+=(Integer rhs) noexcept;

    BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128& operator+=(uint128 rhs) noexcept;

    #ifdef BOOST_DECIMAL_DETAIL_INT128_HAS_MSVC_INT128

    template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_128BIT_INTEGER_CONCEPT>
    BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE inline uint128& operator+=(Integer rhs) noexcept;

    #endif // BOOST_DECIMAL_DETAIL_INT128_HAS_MSVC_INT128

    // Compound Subtraction
    template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_INTEGER_CONCEPT>
    BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128& operator-=(Integer rhs) noexcept;

    BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128& operator-=(uint128 rhs) noexcept;

    #ifdef BOOST_DECIMAL_DETAIL_INT128_HAS_MSVC_INT128

    template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_128BIT_INTEGER_CONCEPT>
    BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE inline uint128& operator-=(Integer rhs) noexcept;

    #endif // BOOST_DECIMAL_DETAIL_INT128_HAS_MSVC_INT128

    // Compound Multiplication
    template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_INTEGER_CONCEPT>
    BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128& operator*=(Integer rhs) noexcept;

    BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128& operator*=(uint128 rhs) noexcept;

    #ifdef BOOST_DECIMAL_DETAIL_INT128_HAS_MSVC_INT128

    template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_128BIT_INTEGER_CONCEPT>
    BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE inline uint128& operator*=(Integer rhs) noexcept;

    #endif // BOOST_DECIMAL_DETAIL_INT128_HAS_MSVC_INT128

    // Compound Division
    template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_INTEGER_CONCEPT>
    BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128& operator/=(Integer rhs) noexcept;

    BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128& operator/=(uint128 rhs) noexcept;

    #ifdef BOOST_DECIMAL_DETAIL_INT128_HAS_MSVC_INT128

    template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_128BIT_INTEGER_CONCEPT>
    BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE inline uint128& operator/=(Integer rhs) noexcept;

    #endif // BOOST_DECIMAL_DETAIL_INT128_HAS_MSVC_INT128

    // Compound modulo
    template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_INTEGER_CONCEPT>
    BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128& operator%=(Integer rhs) noexcept;

    BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128& operator%=(uint128 rhs) noexcept;

    #ifdef BOOST_DECIMAL_DETAIL_INT128_HAS_MSVC_INT128

    template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_128BIT_INTEGER_CONCEPT>
    BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE inline uint128& operator%=(Integer rhs) noexcept;

    #endif // BOOST_DECIMAL_DETAIL_INT128_HAS_MSVC_INT128

    // Compound assignment with floating point types.
    // Matches the builtin: this value is converted to Float, the operation is applied in
    // floating point, and the result is converted back, truncating toward zero.
    template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_FLOATING_POINT_CONCEPT>
    BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128& operator+=(Float rhs) noexcept;

    template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_FLOATING_POINT_CONCEPT>
    BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128& operator-=(Float rhs) noexcept;

    template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_FLOATING_POINT_CONCEPT>
    BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128& operator*=(Float rhs) noexcept;

    template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_FLOATING_POINT_CONCEPT>
    BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128& operator/=(Float rhs) noexcept;

    // The builtin does not allow a floating point operand for these, so neither do we.
    // Without these the implicit floating point constructor would silently truncate rhs
    template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_FLOATING_POINT_CONCEPT>
    BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE uint128& operator%=(Float rhs) = delete;

    template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_FLOATING_POINT_CONCEPT>
    BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE uint128& operator&=(Float rhs) = delete;

    template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_FLOATING_POINT_CONCEPT>
    BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE uint128& operator|=(Float rhs) = delete;

    template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_FLOATING_POINT_CONCEPT>
    BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE uint128& operator^=(Float rhs) = delete;

    template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_FLOATING_POINT_CONCEPT>
    BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE uint128& operator<<=(Float rhs) = delete;

    template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_FLOATING_POINT_CONCEPT>
    BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE uint128& operator>>=(Float rhs) = delete;
};

//=====================================
// Absolute Value function
//=====================================

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128 abs(const uint128 value) noexcept
{
    return value;
}

//=====================================
// Float Conversion Operators
//=====================================

// When the builtin 128-bit type exists we convert through it since the compiler
// runtime (__floatuntisf and friends) is correctly rounded. The portable fallback
// composes the words as high * 2^64 + low; see detail/float_conversion.hpp

BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128::operator float() const noexcept
{
    #if defined(BOOST_DECIMAL_DETAIL_INT128_HAS_INT128) && !defined(BOOST_DECIMAL_DETAIL_INT128_HAS_GPU_SUPPORT)

    return static_cast<float>(static_cast<detail::builtin_u128>(*this));

    #else

    return detail::unsigned_words_to_float<float>(high, low);

    #endif
}

BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128::operator double() const noexcept
{
    #if defined(BOOST_DECIMAL_DETAIL_INT128_HAS_INT128) && !defined(BOOST_DECIMAL_DETAIL_INT128_HAS_GPU_SUPPORT)

    return static_cast<double>(static_cast<detail::builtin_u128>(*this));

    #else

    return detail::unsigned_words_to_float<double>(high, low);

    #endif
}

#if !defined(BOOST_DECIMAL_DETAIL_INT128_HAS_GPU_SUPPORT)

constexpr uint128::operator long double() const noexcept
{
    #if defined(BOOST_DECIMAL_DETAIL_INT128_HAS_INT128)

    return static_cast<long double>(static_cast<detail::builtin_u128>(*this));

    #else

    return detail::unsigned_words_to_float<long double>(high, low);

    #endif
}

#endif // BOOST_DECIMAL_DETAIL_INT128_HAS_GPU_SUPPORT

//=====================================
// Float Construction
//=====================================

// Inverse of operator(Float): decompose f into (high, low) by dividing by 2^64.
// NaN/negative -> 0
// overflow -> UINT128_MAX.
template <BOOST_DECIMAL_DETAIL_INT128_FLOATING_POINT_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128::uint128(Float f) noexcept
{
    constexpr Float two_32 {static_cast<Float>(UINT64_C(1) << 32)};
    constexpr Float two_64 {two_32 * two_32};

    // !(f >= 0) catches both NaN and negative values without using <cmath>
    if (!(f >= Float{0}))
    {
        return;
    }

    // Overflow test: f >= 2^128 iff f / 2^64 >= 2^64. Comparing scaled values
    // avoids materializing 2^128 as a Float, which overflows to +infinity for
    // `float` and is therefore not constant-evaluable on older compilers.
    const Float scaled {f / two_64};
    if (scaled >= two_64)
    {
        high = UINT64_MAX;
        low = UINT64_MAX;
        return;
    }

    high = detail::float_to_uint64(scaled);
    const Float remainder {f - static_cast<Float>(high) * two_64};
    low = detail::float_to_uint64(remainder);
}

//=====================================
// Unary Operators
//=====================================

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128 operator+(const uint128 value) noexcept
{
    return value;
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128 operator-(const uint128 value) noexcept
{
    return {~value.high + static_cast<std::uint64_t>(value.low == UINT64_C(0)), ~value.low + UINT64_C(1)};
}

//=====================================
// Equality Operators
//=====================================

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr bool operator==(const uint128 lhs, const bool rhs) noexcept
{
    return lhs.high == UINT64_C(0) && lhs.low == static_cast<std::uint64_t>(rhs);
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr bool operator==(const bool lhs, const uint128 rhs) noexcept
{
    return rhs.high == UINT64_C(0) && rhs.low == static_cast<std::uint64_t>(lhs);
}

#if defined(__clang__)
#  pragma clang diagnostic push
#  pragma clang diagnostic ignored "-Wsign-conversion"
#elif defined(__GNUC__)
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wsign-conversion"
#endif

BOOST_DECIMAL_DETAIL_INT128_EXPORT template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_SIGNED_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr bool operator==(const uint128 lhs, const SignedInteger rhs) noexcept
{
    const uint128 rhs_u {rhs};
    return lhs.high == rhs_u.high && lhs.low == rhs_u.low;
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_SIGNED_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr bool operator==(const SignedInteger lhs, const uint128 rhs) noexcept
{
    const uint128 lhs_u {lhs};
    return lhs_u.high == rhs.high && lhs_u.low == rhs.low;
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_UNSIGNED_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr bool operator==(const uint128 lhs, const UnsignedInteger rhs) noexcept
{
    return lhs.high == UINT64_C(0) && lhs.low == static_cast<std::uint64_t>(rhs);
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_UNSIGNED_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr bool operator==(const UnsignedInteger lhs, const uint128 rhs) noexcept
{
    return rhs.high == UINT64_C(0) && rhs.low == static_cast<std::uint64_t>(lhs);
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr bool operator==(const uint128 lhs, const uint128 rhs) noexcept
{
    #if defined(__aarch64__) || defined(_M_ARM64) || defined(_M_AMD64)

    return lhs.low == rhs.low && lhs.high == rhs.high;

    #elif defined(__x86_64__) && defined(BOOST_DECIMAL_DETAIL_INT128_HAS_INT128)

    return static_cast<detail::builtin_u128>(lhs) == static_cast<detail::builtin_u128>(rhs);

    #elif (defined(__i386__) || defined(_M_IX86) || defined(_M_AMD64)) && !defined(BOOST_DECIMAL_DETAIL_INT128_NO_CONSTEVAL_DETECTION) && defined(__SSE2__)

    if (BOOST_DECIMAL_DETAIL_INT128_IS_CONSTANT_EVALUATED(lhs))
    {
        return lhs.low == rhs.low && lhs.high == rhs.high;
    }
    else
    {
        __m128i a = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&lhs));
        __m128i b = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&rhs));
        __m128i cmp = _mm_cmpeq_epi32(a, b);

        return _mm_movemask_ps(_mm_castsi128_ps(cmp)) == 0xF;
    }

    #else

    return lhs.high == rhs.high && lhs.low == rhs.low;

    #endif
}

#if defined(BOOST_DECIMAL_DETAIL_INT128_HAS_INT128) || defined(BOOST_DECIMAL_DETAIL_INT128_HAS_MSVC_INT128)

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_BUILTIN_CONSTEXPR bool operator==(const uint128 lhs, const detail::builtin_i128 rhs) noexcept
{
    return lhs == static_cast<uint128>(rhs);
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_BUILTIN_CONSTEXPR bool operator==(const detail::builtin_i128 lhs, const uint128 rhs) noexcept
{
    return static_cast<uint128>(lhs) == rhs;
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_BUILTIN_CONSTEXPR bool operator==(const uint128 lhs, const detail::builtin_u128 rhs) noexcept
{
    return lhs == static_cast<uint128>(rhs);
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_BUILTIN_CONSTEXPR bool operator==(const detail::builtin_u128 lhs, const uint128 rhs) noexcept
{
    return static_cast<uint128>(lhs) == rhs;
}

#endif // BOOST_DECIMAL_DETAIL_INT128_HAS_INT128

//=====================================
// Inequality Operators
//=====================================

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr bool operator!=(const uint128 lhs, const bool rhs) noexcept
{
    return lhs.high != UINT64_C(0) || lhs.low != static_cast<std::uint64_t>(rhs);
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr bool operator!=(const bool lhs, const uint128 rhs) noexcept
{
    return rhs.high != UINT64_C(0) || rhs.low != static_cast<std::uint64_t>(lhs);
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_SIGNED_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr bool operator!=(const uint128 lhs, const SignedInteger rhs) noexcept
{
    const uint128 rhs_u {rhs};
    return lhs.high != rhs_u.high || lhs.low != rhs_u.low;
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_SIGNED_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr bool operator!=(const SignedInteger lhs, const uint128 rhs) noexcept
{
    const uint128 lhs_u {lhs};
    return lhs_u.high != rhs.high || lhs_u.low != rhs.low;
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_UNSIGNED_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr bool operator!=(const uint128 lhs, const UnsignedInteger rhs) noexcept
{
    return lhs.high != UINT64_C(0) || lhs.low != static_cast<std::uint64_t>(rhs);
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_UNSIGNED_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr bool operator!=(const UnsignedInteger lhs, const uint128 rhs) noexcept
{
    return rhs.high != UINT64_C(0) || rhs.low != static_cast<std::uint64_t>(lhs);
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr bool operator!=(const uint128 lhs, const uint128 rhs) noexcept
{
    #if defined(__aarch64__) || defined(_M_ARM64) || defined(_M_AMD64)

    return lhs.low != rhs.low || lhs.high != rhs.high;

    #elif defined(__x86_64__) && defined(BOOST_DECIMAL_DETAIL_INT128_HAS_INT128)

    return static_cast<detail::builtin_u128>(lhs) != static_cast<detail::builtin_u128>(rhs);

    #elif (defined(__i386__) || defined(_M_IX86)) && !defined(BOOST_DECIMAL_DETAIL_INT128_NO_CONSTEVAL_DETECTION) && defined(__SSE2__)

    if (BOOST_DECIMAL_DETAIL_INT128_IS_CONSTANT_EVALUATED(lhs))
    {
        return lhs.low != rhs.low || lhs.high != rhs.high;
    }
    else
    {
        __m128i a = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&lhs));
        __m128i b = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&rhs));
        __m128i cmp = _mm_cmpeq_epi32(a, b);

        return _mm_movemask_ps(_mm_castsi128_ps(cmp)) != 0xF;
    }

    #else

    return lhs.high != rhs.high || lhs.low != rhs.low;

    #endif
}

#if defined(BOOST_DECIMAL_DETAIL_INT128_HAS_INT128) || defined(BOOST_DECIMAL_DETAIL_INT128_BUILTIN_CONSTEXPR)


BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_BUILTIN_CONSTEXPR bool operator!=(const uint128 lhs, const detail::builtin_i128 rhs) noexcept
{
    return lhs != static_cast<uint128>(rhs);
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_BUILTIN_CONSTEXPR bool operator!=(const detail::builtin_i128 lhs, const uint128 rhs) noexcept
{
    return static_cast<uint128>(lhs) != rhs;
}


BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_BUILTIN_CONSTEXPR bool operator!=(const uint128 lhs, const detail::builtin_u128 rhs) noexcept
{
    return lhs != static_cast<uint128>(rhs);
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_BUILTIN_CONSTEXPR bool operator!=(const detail::builtin_u128 lhs, const uint128 rhs) noexcept
{
    return static_cast<uint128>(lhs) != rhs;
}

#endif // BOOST_DECIMAL_DETAIL_INT128_HAS_INT128

//=====================================
// Less than Operators
//=====================================

BOOST_DECIMAL_DETAIL_INT128_EXPORT template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_SIGNED_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr bool operator<(const uint128 lhs, const SignedInteger rhs) noexcept
{
    const uint128 rhs_u {rhs};
    return lhs.high == rhs_u.high ? lhs.low < rhs_u.low : lhs.high < rhs_u.high;
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_SIGNED_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr bool operator<(const SignedInteger lhs, const uint128 rhs) noexcept
{
    const uint128 lhs_u {lhs};
    return lhs_u.high == rhs.high ? lhs_u.low < rhs.low : lhs_u.high < rhs.high;
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_UNSIGNED_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr bool operator<(const uint128 lhs, const UnsignedInteger rhs) noexcept
{
    return lhs.high == UINT64_C(0) && lhs.low < static_cast<std::uint64_t>(rhs);
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_UNSIGNED_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr bool operator<(const UnsignedInteger lhs, const uint128 rhs) noexcept
{
    return rhs.high > UINT64_C(0) || static_cast<std::uint64_t>(lhs) < rhs.low;
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr bool operator<(const uint128 lhs, const uint128 rhs) noexcept
{
    // On ARM macs only with the clang compiler is casting to unsigned __int128 uniformly better (and seemingly cost free)
    #if defined(__clang__) && defined(BOOST_DECIMAL_DETAIL_INT128_HAS_INT128)

    return static_cast<detail::builtin_u128>(lhs) < static_cast<detail::builtin_u128>(rhs);

    #elif defined(__x86_64__) && !defined(BOOST_DECIMAL_DETAIL_INT128_NO_CONSTEVAL_DETECTION) && defined(__GNUC__) && !defined(__clang__) && defined(BOOST_DECIMAL_DETAIL_INT128_HAS_INT128)

    if (BOOST_DECIMAL_DETAIL_INT128_IS_CONSTANT_EVALUATED(lhs))
    {
        return lhs.high == rhs.high ? lhs.low < rhs.low : lhs.high < rhs.high; // LCOV_EXCL_LINE
    }
    else
    {
        detail::builtin_u128 builtin_lhs {};
        detail::builtin_u128 builtin_rhs {};

        std::memcpy(&builtin_lhs, &lhs, sizeof(builtin_lhs));
        std::memcpy(&builtin_rhs, &rhs, sizeof(builtin_rhs));

        return builtin_lhs < builtin_rhs;
    }

    #elif (defined(__i386__) || defined(_M_IX86) || defined(__arm__)) && !defined(BOOST_DECIMAL_DETAIL_INT128_NO_CONSTEVAL_DETECTION)

    if (BOOST_DECIMAL_DETAIL_INT128_IS_CONSTANT_EVALUATED(lhs))
    {
        return lhs.high == rhs.high ? lhs.low < rhs.low : lhs.high < rhs.high; // LCOV_EXCL_LINE
    }
    else
    {
        std::uint32_t l[4] {};
        std::uint32_t r[4] {};
        std::memcpy(l, &lhs, sizeof(lhs));
        std::memcpy(r, &rhs, sizeof(rhs));

        if (l[3] != r[3])
        {
            return l[3] < r[3];
        }
        else if (l[2] != r[2])
        {
            return l[2] < r[2];
        }
        else if (l[1] != r[1])
        {
            return l[1] < r[1];
        }
        else
        {
            return l[0] < r[0];
        }
    }

    #else

    return lhs.high == rhs.high ? lhs.low < rhs.low : lhs.high < rhs.high;

    #endif
}

#if defined(BOOST_DECIMAL_DETAIL_INT128_HAS_INT128) || defined(BOOST_DECIMAL_DETAIL_INT128_HAS_MSVC_INT128)


BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_BUILTIN_CONSTEXPR bool operator<(const uint128 lhs, const detail::builtin_i128 rhs) noexcept
{
    return lhs < static_cast<uint128>(rhs);
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_BUILTIN_CONSTEXPR bool operator<(const detail::builtin_i128 lhs, const uint128 rhs) noexcept
{
    return static_cast<uint128>(lhs) < rhs;
}


BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_BUILTIN_CONSTEXPR bool operator<(const uint128 lhs, const detail::builtin_u128 rhs) noexcept
{
    return lhs < static_cast<uint128>(rhs);
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_BUILTIN_CONSTEXPR bool operator<(const detail::builtin_u128 lhs, const uint128 rhs) noexcept
{
    return static_cast<uint128>(lhs) < rhs;
}

#endif // BOOST_DECIMAL_DETAIL_INT128_HAS_INT128

//=====================================
// Less Equal Operators
//=====================================

BOOST_DECIMAL_DETAIL_INT128_EXPORT template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_SIGNED_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr bool operator<=(const uint128 lhs, const SignedInteger rhs) noexcept
{
    const uint128 rhs_u {rhs};
    return lhs.high == rhs_u.high ? lhs.low <= rhs_u.low : lhs.high < rhs_u.high;
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_SIGNED_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr bool operator<=(const SignedInteger lhs, const uint128 rhs) noexcept
{
    const uint128 lhs_u {lhs};
    return lhs_u.high == rhs.high ? lhs_u.low <= rhs.low : lhs_u.high < rhs.high;
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_UNSIGNED_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr bool operator<=(const uint128 lhs, const UnsignedInteger rhs) noexcept
{
    return lhs.high == UINT64_C(0) && lhs.low <= static_cast<std::uint64_t>(rhs);
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_UNSIGNED_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr bool operator<=(const UnsignedInteger lhs, const uint128 rhs) noexcept
{
    return rhs.high > UINT64_C(0) || static_cast<std::uint64_t>(lhs) <= rhs.low;
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr bool operator<=(const uint128 lhs, const uint128 rhs) noexcept
{
    #if defined(__clang__) && defined(BOOST_DECIMAL_DETAIL_INT128_HAS_INT128)

    return static_cast<detail::builtin_u128>(lhs) <= static_cast<detail::builtin_u128>(rhs);

    #elif defined(__x86_64__) && !defined(BOOST_DECIMAL_DETAIL_INT128_NO_CONSTEVAL_DETECTION) && defined(__GNUC__) && !defined(__clang__) && defined(BOOST_DECIMAL_DETAIL_INT128_HAS_INT128)

    if (BOOST_DECIMAL_DETAIL_INT128_IS_CONSTANT_EVALUATED(lhs))
    {
        return lhs.high == rhs.high ? lhs.low <= rhs.low : lhs.high <= rhs.high; // LCOV_EXCL_LINE
    }
    else
    {
        detail::builtin_u128 builtin_lhs {};
        detail::builtin_u128 builtin_rhs {};

        std::memcpy(&builtin_lhs, &lhs, sizeof(builtin_lhs));
        std::memcpy(&builtin_rhs, &rhs, sizeof(builtin_rhs));

        return builtin_lhs <= builtin_rhs;
    }

    #elif (defined(__i386__) || defined(_M_IX86) || defined(__arm__)) && !defined(BOOST_DECIMAL_DETAIL_INT128_NO_CONSTEVAL_DETECTION)

    if (BOOST_DECIMAL_DETAIL_INT128_IS_CONSTANT_EVALUATED(lhs))
    {
        return lhs.high == rhs.high ? lhs.low <= rhs.low : lhs.high <= rhs.high; // LCOV_EXCL_LINE
    }
    else
    {
        std::uint32_t l[4] {};
        std::uint32_t r[4] {};
        std::memcpy(l, &lhs, sizeof(lhs));
        std::memcpy(r, &rhs, sizeof(rhs));

        if (l[3] != r[3])
        {
            return l[3] < r[3];
        }
        else if (l[2] != r[2])
        {
            return l[2] < r[2];
        }
        else if (l[1] != r[1])
        {
            return l[1] < r[1];
        }
        else
        {
            return l[0] <= r[0];
        }
    }

    #else

    return lhs.high == rhs.high ? lhs.low <= rhs.low : lhs.high <= rhs.high;

    #endif
}

#if defined(BOOST_DECIMAL_DETAIL_INT128_HAS_INT128) || defined(BOOST_DECIMAL_DETAIL_INT128_HAS_MSVC_INT128)


BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_BUILTIN_CONSTEXPR bool operator<=(const uint128 lhs, const detail::builtin_i128 rhs) noexcept
{
    return lhs <= static_cast<uint128>(rhs);
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_BUILTIN_CONSTEXPR bool operator<=(const detail::builtin_i128 lhs, const uint128 rhs) noexcept
{
    return static_cast<uint128>(lhs) <= rhs;
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_BUILTIN_CONSTEXPR bool operator<=(const uint128 lhs, const detail::builtin_u128 rhs) noexcept
{
    return lhs <= static_cast<uint128>(rhs);
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_BUILTIN_CONSTEXPR bool operator<=(const detail::builtin_u128 lhs, const uint128 rhs) noexcept
{
    return static_cast<uint128>(lhs) <= rhs;
}


#endif // BOOST_DECIMAL_DETAIL_INT128_HAS_INT128

//=====================================
// Greater Than Operators
//=====================================

BOOST_DECIMAL_DETAIL_INT128_EXPORT template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_SIGNED_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr bool operator>(const uint128 lhs, const SignedInteger rhs) noexcept
{
    const uint128 rhs_u {rhs};
    return lhs.high == rhs_u.high ? lhs.low > rhs_u.low : lhs.high > rhs_u.high;
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_SIGNED_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr bool operator>(const SignedInteger lhs, const uint128 rhs) noexcept
{
    const uint128 lhs_u {lhs};
    return lhs_u.high == rhs.high ? lhs_u.low > rhs.low : lhs_u.high > rhs.high;
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_UNSIGNED_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr bool operator>(const uint128 lhs, const UnsignedInteger rhs) noexcept
{
    return lhs.high > UINT64_C(0) || lhs.low > static_cast<std::uint64_t>(rhs);
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_UNSIGNED_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr bool operator>(const UnsignedInteger lhs, const uint128 rhs) noexcept
{
    return rhs.high == UINT64_C(0) && static_cast<std::uint64_t>(lhs) > rhs.low;
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr bool operator>(const uint128 lhs, const uint128 rhs) noexcept
{
    #if defined(__clang__) && defined(BOOST_DECIMAL_DETAIL_INT128_HAS_INT128)

    return static_cast<detail::builtin_u128>(lhs) > static_cast<detail::builtin_u128>(rhs);

    #elif defined(__x86_64__) && !defined(BOOST_DECIMAL_DETAIL_INT128_NO_CONSTEVAL_DETECTION) && defined(__GNUC__) && !defined(__clang__) && defined(BOOST_DECIMAL_DETAIL_INT128_HAS_INT128)

    if (BOOST_DECIMAL_DETAIL_INT128_IS_CONSTANT_EVALUATED(lhs))
    {
        return lhs.high == rhs.high ? rhs.low < lhs.low : rhs.high < lhs.high; // LCOV_EXCL_LINE
    }
    else
    {
        detail::builtin_u128 builtin_lhs {};
        detail::builtin_u128 builtin_rhs {};

        std::memcpy(&builtin_lhs, &lhs, sizeof(builtin_lhs));
        std::memcpy(&builtin_rhs, &rhs, sizeof(builtin_rhs));

        return builtin_lhs > builtin_rhs;
    }

    #elif (defined(__i386__) || defined(_M_IX86) || defined(__arm__)) && !defined(BOOST_DECIMAL_DETAIL_INT128_NO_CONSTEVAL_DETECTION)

    if (BOOST_DECIMAL_DETAIL_INT128_IS_CONSTANT_EVALUATED(lhs))
    {
        return lhs.high == rhs.high ? rhs.low < lhs.low : rhs.high < lhs.high; // LCOV_EXCL_LINE
    }
    else
    {
        std::uint32_t l[4] {};
        std::uint32_t r[4] {};
        std::memcpy(l, &lhs, sizeof(lhs));
        std::memcpy(r, &rhs, sizeof(rhs));

        if (l[3] != r[3])
        {
            return l[3] > r[3];
        }
        else if (l[2] != r[2])
        {
            return l[2] > r[2];
        }
        else if (l[1] != r[1])
        {
            return l[1] > r[1];
        }
        else
        {
            return l[0] > r[0];
        }
    }

    #else

    return lhs.high == rhs.high ? rhs.low < lhs.low : rhs.high < lhs.high;

    #endif
}

#if defined(BOOST_DECIMAL_DETAIL_INT128_HAS_INT128) || defined(BOOST_DECIMAL_DETAIL_INT128_HAS_MSVC_INT128)


BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_BUILTIN_CONSTEXPR bool operator>(const uint128 lhs, const detail::builtin_i128 rhs) noexcept
{
    return lhs > static_cast<uint128>(rhs);
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_BUILTIN_CONSTEXPR bool operator>(const detail::builtin_i128 lhs, const uint128 rhs) noexcept
{
    return static_cast<uint128>(lhs) > rhs;
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_BUILTIN_CONSTEXPR bool operator>(const uint128 lhs, const detail::builtin_u128 rhs) noexcept
{
    return lhs > static_cast<uint128>(rhs);
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_BUILTIN_CONSTEXPR bool operator>(const detail::builtin_u128 lhs, const uint128 rhs) noexcept
{
    return static_cast<uint128>(lhs) > rhs;
}


#endif // BOOST_DECIMAL_DETAIL_INT128_HAS_INT128

//=====================================
// Greater-equal Operators
//=====================================

BOOST_DECIMAL_DETAIL_INT128_EXPORT template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_SIGNED_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr bool operator>=(const uint128 lhs, const SignedInteger rhs) noexcept
{
    const uint128 rhs_u {rhs};
    return lhs.high == rhs_u.high ? lhs.low >= rhs_u.low : lhs.high > rhs_u.high;
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_SIGNED_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr bool operator>=(const SignedInteger lhs, const uint128 rhs) noexcept
{
    const uint128 lhs_u {lhs};
    return lhs_u.high == rhs.high ? lhs_u.low >= rhs.low : lhs_u.high > rhs.high;
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_UNSIGNED_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr bool operator>=(const uint128 lhs, const UnsignedInteger rhs) noexcept
{
    return lhs.high > UINT64_C(0) || lhs.low >= static_cast<std::uint64_t>(rhs);
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_UNSIGNED_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr bool operator>=(const UnsignedInteger lhs, const uint128 rhs) noexcept
{
    return rhs.high == UINT64_C(0) && static_cast<std::uint64_t>(lhs) >= rhs.low;
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr bool operator>=(const uint128 lhs, const uint128 rhs) noexcept
{
    #if defined(__clang__) && defined(BOOST_DECIMAL_DETAIL_INT128_HAS_INT128)

    return static_cast<detail::builtin_u128>(lhs) >= static_cast<detail::builtin_u128>(rhs);

    #elif defined(__x86_64__) && !defined(BOOST_DECIMAL_DETAIL_INT128_NO_CONSTEVAL_DETECTION) && defined(__GNUC__) && !defined(__clang__) && defined(BOOST_DECIMAL_DETAIL_INT128_HAS_INT128)

    if (BOOST_DECIMAL_DETAIL_INT128_IS_CONSTANT_EVALUATED(lhs))
    {
        return lhs.high == rhs.high ? rhs.low <= lhs.low : rhs.high <= lhs.high; // LCOV_EXCL_LINE
    }
    else
    {
        detail::builtin_u128 builtin_lhs {};
        detail::builtin_u128 builtin_rhs {};

        std::memcpy(&builtin_lhs, &lhs, sizeof(builtin_lhs));
        std::memcpy(&builtin_rhs, &rhs, sizeof(builtin_rhs));

        return builtin_lhs >= builtin_rhs;
    }

    #elif (defined(__i386__) || defined(_M_IX86) || defined(__arm__)) && !defined(BOOST_DECIMAL_DETAIL_INT128_NO_CONSTEVAL_DETECTION)

    if (BOOST_DECIMAL_DETAIL_INT128_IS_CONSTANT_EVALUATED(lhs))
    {
        return lhs.high == rhs.high ? rhs.low <= lhs.low : rhs.high <= lhs.high; // LCOV_EXCL_LINE
    }
    else
    {
        std::uint32_t l[4] {};
        std::uint32_t r[4] {};
        std::memcpy(l, &lhs, sizeof(lhs));
        std::memcpy(r, &rhs, sizeof(rhs));

        if (l[3] != r[3])
        {
            return l[3] > r[3];
        }
        else if (l[2] != r[2])
        {
            return l[2] > r[2];
        }
        else if (l[1] != r[1])
        {
            return l[1] > r[1];
        }
        else
        {
            return l[0] >= r[0];
        }
    }

    #else

    return lhs.high == rhs.high ? rhs.low <= lhs.low : rhs.high <= lhs.high;

    #endif
}

#if defined(BOOST_DECIMAL_DETAIL_INT128_HAS_INT128) || defined(BOOST_DECIMAL_DETAIL_INT128_HAS_MSVC_INT128)

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_BUILTIN_CONSTEXPR bool operator>=(const uint128 lhs, const detail::builtin_i128 rhs) noexcept
{
    return lhs >= static_cast<uint128>(rhs);
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_BUILTIN_CONSTEXPR bool operator>=(const detail::builtin_i128 lhs, const uint128 rhs) noexcept
{
    return static_cast<uint128>(lhs) >= rhs;
}


BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_BUILTIN_CONSTEXPR bool operator>=(const uint128 lhs, const detail::builtin_u128 rhs) noexcept
{
    return lhs >= static_cast<uint128>(rhs);
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_BUILTIN_CONSTEXPR bool operator>=(const detail::builtin_u128 lhs, const uint128 rhs) noexcept
{
    return static_cast<uint128>(lhs) >= rhs;
}


#endif // BOOST_DECIMAL_DETAIL_INT128_HAS_INT128

//=====================================
// Spaceship Operator
//=====================================

#ifdef BOOST_DECIMAL_DETAIL_INT128_HAS_SPACESHIP_OPERATOR

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr std::strong_ordering operator<=>(const uint128 lhs, const uint128 rhs) noexcept
{
    if (lhs < rhs)
    {
        return std::strong_ordering::less;
    }
    else if (lhs == rhs)
    {
        return std::strong_ordering::equivalent;
    }
    else
    {
        return std::strong_ordering::greater;
    }
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_UNSIGNED_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr std::strong_ordering operator<=>(const uint128 lhs, const UnsignedInteger rhs) noexcept
{
    if (lhs < rhs)
    {
        return std::strong_ordering::less;
    }
    else if (lhs == rhs)
    {
        return std::strong_ordering::equivalent;
    }
    else
    {
        return std::strong_ordering::greater;
    }
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_UNSIGNED_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr std::strong_ordering operator<=>(const UnsignedInteger lhs, const uint128 rhs) noexcept
{
    if (lhs < rhs)
    {
        return std::strong_ordering::less;
    }
    else if (lhs == rhs)
    {
        return std::strong_ordering::equivalent;
    }
    else
    {
        return std::strong_ordering::greater;
    }
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_SIGNED_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr std::strong_ordering operator<=>(const SignedInteger lhs, const uint128 rhs) noexcept
{
    if (lhs < rhs)
    {
        return std::strong_ordering::less;
    }
    else if (lhs == rhs)
    {
        return std::strong_ordering::equivalent;
    }
    else
    {
        return std::strong_ordering::greater;
    }
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_SIGNED_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr std::strong_ordering operator<=>(const uint128 lhs, const SignedInteger rhs) noexcept
{
    if (lhs < rhs)
    {
        return std::strong_ordering::less;
    }
    else if (lhs == rhs)
    {
        return std::strong_ordering::equivalent;
    }
    else
    {
        return std::strong_ordering::greater;
    }
}

#endif

//=====================================
// Not Operator
//=====================================

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128 operator~(const uint128 rhs) noexcept
{
    return {~rhs.high, ~rhs.low};
}

//=====================================
// OR Operator
//=====================================

BOOST_DECIMAL_DETAIL_INT128_EXPORT template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_SIGNED_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128 operator|(const uint128 lhs, const SignedInteger rhs) noexcept
{
    return {lhs.high | (rhs < 0 ? ~UINT64_C(0) : UINT64_C(0)), lhs.low | static_cast<std::uint64_t>(rhs)};
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_SIGNED_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128 operator|(const SignedInteger lhs, const uint128 rhs) noexcept
{
    return {rhs.high | (lhs < 0 ? ~UINT64_C(0) : UINT64_C(0)), rhs.low | static_cast<std::uint64_t>(lhs)};
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_UNSIGNED_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128 operator|(const uint128 lhs, const UnsignedInteger rhs) noexcept
{
    return {lhs.high, lhs.low | static_cast<std::uint64_t>(rhs)};
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_UNSIGNED_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128 operator|(const UnsignedInteger lhs, const uint128 rhs) noexcept
{
    return {rhs.high, rhs.low | static_cast<std::uint64_t>(lhs)};
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128 operator|(const uint128 lhs, const uint128 rhs) noexcept
{
    return {lhs.high | rhs.high, lhs.low | rhs.low};
}

#if defined(BOOST_DECIMAL_DETAIL_INT128_HAS_INT128) || defined(BOOST_DECIMAL_DETAIL_INT128_HAS_MSVC_INT128)


BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_BUILTIN_CONSTEXPR uint128 operator|(const uint128 lhs, const detail::builtin_i128 rhs) noexcept
{
    return lhs | static_cast<uint128>(rhs);
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_BUILTIN_CONSTEXPR uint128 operator|(const detail::builtin_i128 lhs, const uint128 rhs) noexcept
{
    return static_cast<uint128>(lhs) | rhs;
}


BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_BUILTIN_CONSTEXPR uint128 operator|(const uint128 lhs, const detail::builtin_u128 rhs) noexcept
{
    return lhs | static_cast<uint128>(rhs);
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_BUILTIN_CONSTEXPR uint128 operator|(const detail::builtin_u128 lhs, const uint128 rhs) noexcept
{
    return static_cast<uint128>(lhs) | rhs;
}

#endif // BOOST_DECIMAL_DETAIL_INT128_HAS_INT128

template <BOOST_DECIMAL_DETAIL_INT128_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128& uint128::operator|=(const Integer rhs) noexcept
{
    *this = *this | rhs;
    return *this;
}
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128& uint128::operator|=(const uint128 rhs) noexcept
{
    *this = *this | rhs;
    return *this;
}

#ifdef BOOST_DECIMAL_DETAIL_INT128_HAS_MSVC_INT128

template <BOOST_DECIMAL_DETAIL_INT128_128BIT_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE inline uint128& uint128::operator|=(const Integer rhs) noexcept
{
    *this = *this | rhs;
    return *this;
}

#endif

//=====================================
// AND Operator
//=====================================

BOOST_DECIMAL_DETAIL_INT128_EXPORT template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_SIGNED_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128 operator&(const uint128 lhs, const SignedInteger rhs) noexcept
{
    return {lhs.high & (rhs < 0 ? ~UINT64_C(0) : UINT64_C(0)), lhs.low & static_cast<std::uint64_t>(rhs)};
}

template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_SIGNED_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128 operator&(const SignedInteger lhs, const uint128 rhs) noexcept
{
    return {rhs.high & (lhs < 0 ? ~UINT64_C(0) : UINT64_C(0)), rhs.low & static_cast<std::uint64_t>(lhs)};
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_UNSIGNED_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128 operator&(const uint128 lhs, const UnsignedInteger rhs) noexcept
{
    return {UINT64_C(0), lhs.low & static_cast<std::uint64_t>(rhs)};
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_UNSIGNED_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128 operator&(const UnsignedInteger lhs, const uint128 rhs) noexcept
{
    return {UINT64_C(0), rhs.low & static_cast<std::uint64_t>(lhs)};
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128 operator&(const uint128 lhs, const uint128 rhs) noexcept
{
    return {lhs.high & rhs.high, lhs.low & rhs.low};
}

#if defined(BOOST_DECIMAL_DETAIL_INT128_HAS_INT128) || defined(BOOST_DECIMAL_DETAIL_INT128_HAS_MSVC_INT128)


BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_BUILTIN_CONSTEXPR uint128 operator&(const uint128 lhs, const detail::builtin_i128 rhs) noexcept
{
    return lhs & static_cast<uint128>(rhs);
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_BUILTIN_CONSTEXPR uint128 operator&(const detail::builtin_i128 lhs, const uint128 rhs) noexcept
{
    return static_cast<uint128>(lhs) & rhs;
}


BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_BUILTIN_CONSTEXPR uint128 operator&(const uint128 lhs, const detail::builtin_u128 rhs) noexcept
{
    return lhs & static_cast<uint128>(rhs);
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_BUILTIN_CONSTEXPR uint128 operator&(const detail::builtin_u128 lhs, const uint128 rhs) noexcept
{
    return static_cast<uint128>(lhs) & rhs;
}

#endif // BOOST_DECIMAL_DETAIL_INT128_HAS_INT128

template <BOOST_DECIMAL_DETAIL_INT128_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128& uint128::operator&=(const Integer rhs) noexcept
{
    *this = *this & rhs;
    return *this;
}

BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128& uint128::operator&=(const uint128 rhs) noexcept
{
    *this = *this & rhs;
    return *this;
}

#ifdef BOOST_DECIMAL_DETAIL_INT128_HAS_MSVC_INT128

template <BOOST_DECIMAL_DETAIL_INT128_128BIT_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE inline uint128& uint128::operator&=(Integer rhs) noexcept
{
    *this = *this & rhs;
    return *this;
}

#endif // BOOST_DECIMAL_DETAIL_INT128_HAS_MSVC_INT128


//=====================================
// XOR Operator
//=====================================

BOOST_DECIMAL_DETAIL_INT128_EXPORT template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_SIGNED_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128 operator^(const uint128 lhs, const SignedInteger rhs) noexcept
{
    return {lhs.high ^ (rhs < 0 ? ~UINT64_C(0) : UINT64_C(0)), lhs.low ^ static_cast<std::uint64_t>(rhs)};
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_SIGNED_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128 operator^(const SignedInteger lhs, const uint128 rhs) noexcept
{
    return {rhs.high ^ (lhs < 0 ? ~UINT64_C(0) : UINT64_C(0)), rhs.low ^ static_cast<std::uint64_t>(lhs)};
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_UNSIGNED_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128 operator^(const uint128 lhs, const UnsignedInteger rhs) noexcept
{
    return {lhs.high, lhs.low ^ static_cast<std::uint64_t>(rhs)};
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_UNSIGNED_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128 operator^(const UnsignedInteger lhs, const uint128 rhs) noexcept
{
    return {rhs.high, rhs.low ^ static_cast<std::uint64_t>(lhs)};
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128 operator^(const uint128 lhs, const uint128 rhs) noexcept
{
    return {lhs.high ^ rhs.high, lhs.low ^ rhs.low};
}

#if defined(BOOST_DECIMAL_DETAIL_INT128_HAS_INT128) || defined(BOOST_DECIMAL_DETAIL_INT128_HAS_MSVC_INT128)


BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_BUILTIN_CONSTEXPR uint128 operator^(const uint128 lhs, const detail::builtin_i128 rhs) noexcept
{
    return lhs ^ static_cast<uint128>(rhs);
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_BUILTIN_CONSTEXPR uint128 operator^(const detail::builtin_i128 lhs, const uint128 rhs) noexcept
{
    return static_cast<uint128>(lhs) ^ rhs;
}


BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_BUILTIN_CONSTEXPR uint128 operator^(const uint128 lhs, const detail::builtin_u128 rhs) noexcept
{
    return lhs ^ static_cast<uint128>(rhs);
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_BUILTIN_CONSTEXPR uint128 operator^(const detail::builtin_u128 lhs, const uint128 rhs) noexcept
{
    return static_cast<uint128>(lhs) ^ rhs;
}

#endif // BOOST_DECIMAL_DETAIL_INT128_HAS_INT128

template <BOOST_DECIMAL_DETAIL_INT128_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128& uint128::operator^=(const Integer rhs) noexcept
{
    *this = *this ^ rhs;
    return *this;
}

BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128& uint128::operator^=(const uint128 rhs) noexcept
{
    *this = *this ^ rhs;
    return *this;
}

#ifdef BOOST_DECIMAL_DETAIL_INT128_HAS_MSVC_INT128

template <BOOST_DECIMAL_DETAIL_INT128_128BIT_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE inline uint128& uint128::operator^=(Integer rhs) noexcept
{
    *this = *this ^ rhs;
    return *this;
}

#endif // BOOST_DECIMAL_DETAIL_INT128_HAS_MSVC_INT128

//=====================================
// Left Shift Operator
//=====================================

namespace detail {

template <typename Integer>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128 default_ls_impl(const uint128 lhs, const Integer rhs) noexcept
{
    static_assert(std::is_integral<Integer>::value, "Needs to be a builtin type");

    // A shift by a negative amount or by an amount >= 128 (the operand width) is
    // undefined behavior, exactly as for the built-in shift operators. In a
    // constant expression the compiler diagnoses it; at runtime it is unspecified.
    if (rhs == 0)
    {
        return lhs;
    }

    if (rhs == 64)
    {
        return {lhs.low, 0};
    }

    if (rhs > 64)
    {
        return {lhs.low << (rhs - 64), 0};
    }

    return {
        (lhs.high << rhs) | (lhs.low >> (64 - rhs)),
        lhs.low << rhs
    };
}

template <typename T>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE uint128 intrinsic_ls_impl(const uint128 lhs, const T rhs) noexcept
{
    // A shift by a negative amount or by an amount >= 128 (the operand width) is
    // undefined behavior, exactly as for the built-in shift operators; delegate
    // straight to the native type so we produce identical results.
    #ifdef BOOST_DECIMAL_DETAIL_INT128_HAS_INT128

    #  if defined(__aarch64__)

        #if defined(__GNUC__) && __GNUC__ >= 8
        #  pragma GCC diagnostic push
        #  pragma GCC diagnostic ignored "-Wclass-memaccess"
        #endif

        builtin_u128 value;
        std::memcpy(&value, &lhs, sizeof(builtin_u128));
        const auto res {value << rhs};

        uint128 return_value;
        std::memcpy(&return_value, &res, sizeof(uint128));
        return return_value;

        #if defined(__GNUC__) && __GNUC__ >= 8
        #  pragma GCC diagnostic pop
        #endif

    #  else

        return static_cast<builtin_u128>(lhs) << rhs;

    #  endif

    #else

    if (rhs == 0)
    {
        return lhs;
    }

    if (rhs == 64)
    {
        return {lhs.low, 0};
    }

    if (rhs > 64)
    {
        return {lhs.low << (rhs - 64), 0};
    }

    return {
        (lhs.high << rhs) | (lhs.low >> (64 - rhs)),
        lhs.low << rhs
    };

    #endif
}

} // namespace detail

BOOST_DECIMAL_DETAIL_INT128_EXPORT template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128 operator<<(const uint128 lhs, const Integer rhs) noexcept
{
    #ifndef BOOST_DECIMAL_DETAIL_INT128_NO_CONSTEVAL_DETECTION

    if (BOOST_DECIMAL_DETAIL_INT128_IS_CONSTANT_EVALUATED(lhs))
    {
        return detail::default_ls_impl(lhs, rhs); // LCOV_EXCL_LINE
    }
    else
    {
        return detail::intrinsic_ls_impl(lhs, rhs);
    }

    #else

    return detail::default_ls_impl(lhs, rhs);

    #endif
}

// A number of different overloads to ensure that we return the same type as the builtins would

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128 operator<<(const uint128 lhs, const uint128 rhs) noexcept
{
    // Out-of-range counts (>= 128 or with the high word set) are undefined,
    // matching the built-in operators; forward the count to the scalar overload.
    return lhs << rhs.low;
}

#if defined(BOOST_DECIMAL_DETAIL_INT128_HAS_INT128) || defined(BOOST_DECIMAL_DETAIL_INT128_HAS_MSVC_INT128)

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_BUILTIN_CONSTEXPR detail::builtin_u128 operator<<(const detail::builtin_u128 lhs, const uint128 rhs) noexcept
{
    // Out-of-range counts are undefined, matching the built-in operators.
    return lhs << static_cast<detail::builtin_u128>(rhs.low);
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_BUILTIN_CONSTEXPR detail::builtin_i128 operator<<(const detail::builtin_i128 lhs, const uint128 rhs) noexcept
{
    // Out-of-range counts are undefined, matching the built-in operators.
    return lhs << static_cast<detail::builtin_u128>(rhs.low);
}

#endif

// A shift takes its value and its result type from the left operand after integral promotion,
// and only the count from the right, exactly as the builtin does

BOOST_DECIMAL_DETAIL_INT128_EXPORT template <typename Integer, std::enable_if_t<detail::is_any_integer_v<Integer> && (sizeof(Integer) * 8 <= 64), bool> = true>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr detail::promoted_t<Integer> operator<<(const Integer lhs, const uint128 rhs) noexcept
{
    // Out-of-range counts are undefined, matching the built-in operators.
    return static_cast<detail::promoted_t<Integer>>(lhs) << rhs.low;
}

template <BOOST_DECIMAL_DETAIL_INT128_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128& uint128::operator<<=(const Integer rhs) noexcept
{
    *this = *this << rhs;
    return *this;
}

BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128& uint128::operator<<=(const uint128 rhs) noexcept
{
    *this = *this << rhs;
    return *this;
}

#ifdef BOOST_DECIMAL_DETAIL_INT128_HAS_MSVC_INT128

template <BOOST_DECIMAL_DETAIL_INT128_128BIT_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE inline uint128& uint128::operator<<=(Integer rhs) noexcept
{
    *this = *this << rhs;
    return *this;
}

#endif

//=====================================
// Right Shift Operator
//=====================================

namespace detail {

template <typename Integer>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128 default_rs_impl(const uint128 lhs, const Integer rhs) noexcept
{
    // A shift by a negative amount or by an amount >= 128 (the operand width) is
    // undefined behavior, exactly as for the built-in shift operators. In a
    // constant expression the compiler diagnoses it; at runtime it is unspecified.
    if (rhs == 0)
    {
        return lhs;
    }

    if (rhs == 64)
    {
        return {0, lhs.high};
    }

    if (rhs > 64)
    {
        return {0, lhs.high >> (rhs - 64)};
    }

    return {
        lhs.high >> rhs,
        (lhs.low >> rhs) | (lhs.high << (64 - rhs))
    };
}

template <typename Integer>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE uint128 intrinsic_rs_impl(const uint128 lhs, const Integer rhs) noexcept
{
    // A shift by a negative amount or by an amount >= 128 (the operand width) is
    // undefined behavior, exactly as for the built-in shift operators; delegate
    // straight to the native type so we produce identical results.
    #ifdef BOOST_DECIMAL_DETAIL_INT128_HAS_INT128

    #  ifdef __aarch64__

        #if defined(__GNUC__) && __GNUC__ >= 8
        #  pragma GCC diagnostic push
        #  pragma GCC diagnostic ignored "-Wclass-memaccess"
        #endif

        builtin_u128 value;
        std::memcpy(&value, &lhs, sizeof(builtin_u128));
        const auto res {value >> rhs};

        uint128 return_value;
        std::memcpy(&return_value, &res, sizeof(uint128));
        return return_value;

        #if defined(__GNUC__) && __GNUC__ >= 8
        #  pragma GCC diagnostic pop
        #endif

    #  else
        return static_cast<builtin_u128>(lhs) >> rhs;
    #  endif

    #else

    if (rhs == 0)
    {
        return lhs;
    }

    if (rhs == 64)
    {
        return {0, lhs.high};
    }

    if (rhs < 64)
    {
        const auto result_low {(lhs.low >> rhs) | (lhs.high << (64 - rhs))};
        const auto result_high {lhs.high >> rhs};
        return {result_high, result_low};
    }

    return {0, lhs.high >> (rhs - 64)};

    #endif
}

} // namespace detail

BOOST_DECIMAL_DETAIL_INT128_EXPORT template <typename Integer, std::enable_if_t<std::is_integral<Integer>::value, bool> = true>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128 operator>>(const uint128 lhs, const Integer rhs) noexcept
{
    #ifndef BOOST_DECIMAL_DETAIL_INT128_NO_CONSTEVAL_DETECTION

    if (BOOST_DECIMAL_DETAIL_INT128_IS_CONSTANT_EVALUATED(lhs))
    {
        return detail::default_rs_impl(lhs, rhs); // LCOV_EXCL_LINE
    }
    else
    {
        return detail::intrinsic_rs_impl(lhs, rhs);
    }

    #else

    return detail::default_rs_impl(lhs, rhs);

    #endif
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128 operator>>(const uint128 lhs, const uint128 rhs) noexcept
{
    // Out-of-range counts (>= 128 or with the high word set) are undefined,
    // matching the built-in operators; forward the count to the scalar overload.
    return lhs >> rhs.low;
}

#if defined(BOOST_DECIMAL_DETAIL_INT128_HAS_INT128) || defined(BOOST_DECIMAL_DETAIL_INT128_HAS_MSVC_INT128)

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_BUILTIN_CONSTEXPR detail::builtin_u128 operator>>(const detail::builtin_u128 lhs, const uint128 rhs) noexcept
{
    // Out-of-range counts are undefined, matching the built-in operators.
    return lhs >> static_cast<detail::builtin_u128>(rhs.low);
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_BUILTIN_CONSTEXPR detail::builtin_i128 operator>>(const detail::builtin_i128 lhs, const uint128 rhs) noexcept
{
    // Out-of-range counts are undefined, matching the built-in operators.
    return lhs >> static_cast<detail::builtin_u128>(rhs.low);
}

#endif

// A shift takes its value and its result type from the left operand after integral promotion,
// and only the count from the right, exactly as the builtin does

BOOST_DECIMAL_DETAIL_INT128_EXPORT template <typename Integer, std::enable_if_t<detail::is_any_integer_v<Integer> && (sizeof(Integer) * 8 <= 64), bool> = true>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr detail::promoted_t<Integer> operator>>(const Integer lhs, const uint128 rhs) noexcept
{
    // Out-of-range counts are undefined, matching the built-in operators.
    return static_cast<detail::promoted_t<Integer>>(lhs) >> rhs.low;
}

template <BOOST_DECIMAL_DETAIL_INT128_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128& uint128::operator>>=(const Integer rhs) noexcept
{
    *this = *this >> rhs;
    return *this;
}

BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128& uint128::operator>>=(const uint128 rhs) noexcept
{
    *this = *this >> rhs;
    return *this;
}

#ifdef BOOST_DECIMAL_DETAIL_INT128_HAS_MSVC_INT128

template <BOOST_DECIMAL_DETAIL_INT128_128BIT_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE inline uint128& uint128::operator>>=(Integer rhs) noexcept
{
    *this = *this >> rhs;
    return *this;
}

#endif

//=====================================
// Increment Operator
//=====================================

BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128& uint128::operator++() noexcept
{
    if (++low == UINT64_C(0))
    {
        ++high;
    }

    return *this;
}

BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128 uint128::operator++(int) noexcept
{
    const auto temp {*this};
    ++(*this);
    return temp;
}

//=====================================
// Decrement Operator
//=====================================

BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128& uint128::operator--() noexcept
{
    if (--low == UINT64_MAX)
    {
        --high;
    }

    return *this;
}

BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128 uint128::operator--(int) noexcept
{
    const auto temp {*this};
    --(*this);
    return temp;
}

//=====================================
// Addition Operator
//=====================================

namespace impl {

BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_FORCE_INLINE constexpr uint128 default_add(const uint128 lhs, const uint128 rhs) noexcept
{
    #if defined(BOOST_DECIMAL_DETAIL_INT128_HAS_BUILTIN_ADD_OVERFLOW) && (defined(__i386__) || (defined(__aarch64__) && !defined(__APPLE__)) || defined(__arm__) || (defined(__s390__) || defined(__s390x__)))

    uint128 res {};
    res.high = lhs.high + rhs.high + __builtin_add_overflow(lhs.low, rhs.low, &res.low);

    return res;

    #elif (defined(__x86_64__) || (defined(__aarch64__) && !defined(__APPLE__))) && !defined(_MSC_VER) && defined(BOOST_DECIMAL_DETAIL_INT128_HAS_INT128)

    return static_cast<uint128>(static_cast<detail::builtin_u128>(lhs) + static_cast<detail::builtin_u128>(rhs));

    #else

    uint128 temp {lhs.high + rhs.high, lhs.low + rhs.low};

    if (temp.low < lhs.low)
    {
        ++temp.high;
    }

    return temp;

    #endif
}

BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_FORCE_INLINE constexpr uint128 default_add(const uint128 lhs, const std::uint64_t rhs) noexcept
{
    #if defined(BOOST_DECIMAL_DETAIL_INT128_HAS_BUILTIN_ADD_OVERFLOW) && (defined(__i386__) || (defined(__aarch64__) && !defined(__APPLE__)) || defined(__arm__) || (defined(__s390__) || defined(__s390x__)))

    uint128 res {};
    res.high = lhs.high + __builtin_add_overflow(lhs.low, rhs, &res.low);

    return res;

    #else

    uint128 temp {lhs.high, lhs.low + rhs};

    if (temp.low < lhs.low)
    {
        ++temp.high;
    }

    return temp;

    #endif
}

BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_FORCE_INLINE constexpr uint128 default_sub(const uint128 lhs, const uint128 rhs) noexcept
{
    #if defined(BOOST_DECIMAL_DETAIL_INT128_HAS_BUILTIN_SUB_OVERFLOW) && (defined(__i386__) || defined(__arm__) || (defined(__s390__) || defined(__s390x__)))

    uint128 res {};
    res.high = lhs.high - rhs.high - __builtin_sub_overflow(lhs.low, rhs.low, &res.low);

    return res;

    #elif (defined(__x86_64__) || (defined(__aarch64__) && !defined(__APPLE__))) && !defined(_MSC_VER) && defined(BOOST_DECIMAL_DETAIL_INT128_HAS_INT128)

    return static_cast<uint128>(static_cast<detail::builtin_u128>(lhs) - static_cast<detail::builtin_u128>(rhs));

    #else

    uint128 temp {lhs.high - rhs.high, lhs.low - rhs.low};

    // Check for carry
    if (lhs.low < rhs.low)
    {
        --temp.high;
    }

    return temp;

    #endif
}

BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_FORCE_INLINE constexpr uint128 default_sub(const uint128 lhs, const std::uint64_t rhs) noexcept
{
    #if defined(BOOST_DECIMAL_DETAIL_INT128_HAS_BUILTIN_SUB_OVERFLOW) && (defined(__i386__) || (defined(__aarch64__) && !defined(__APPLE__)) || defined(__arm__) || (defined(__s390__) || defined(__s390x__)))

    uint128 res {};
    res.high = lhs.high - __builtin_sub_overflow(lhs.low, rhs, &res.low);

    return res;

    #else

    uint128 temp {lhs.high, lhs.low - rhs};

    // Check for carry
    if (lhs.low < rhs)
    {
        --temp.high;
    }

    return temp;

    #endif
}

} // namespace impl

#ifdef _MSC_VER
#  pragma warning(push)
#  pragma warning(disable : 4146)
#endif

BOOST_DECIMAL_DETAIL_INT128_EXPORT template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_SIGNED_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128 operator+(const uint128 lhs, const SignedInteger rhs) noexcept
{
    return rhs < 0 ? impl::default_sub(lhs, -static_cast<std::uint64_t>(rhs)) :
                     impl::default_add(lhs, static_cast<std::uint64_t>(rhs));
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_SIGNED_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128 operator+(const SignedInteger lhs, const uint128 rhs) noexcept
{
    return lhs < 0 ? impl::default_sub(rhs, -static_cast<std::uint64_t>(lhs)) :
                     impl::default_add(rhs, static_cast<std::uint64_t>(lhs));
}

#ifdef _MSC_VER
#  pragma warning(pop)
#endif

BOOST_DECIMAL_DETAIL_INT128_EXPORT template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_UNSIGNED_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128 operator+(const uint128 lhs, const UnsignedInteger rhs) noexcept
{
    return impl::default_add(lhs, static_cast<std::uint64_t>(rhs));
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_UNSIGNED_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128 operator+(const UnsignedInteger lhs, const uint128 rhs) noexcept
{
    return impl::default_add(rhs, static_cast<std::uint64_t>(lhs));
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128 operator+(const uint128 lhs, const uint128 rhs) noexcept
{
    return impl::default_add(lhs, rhs);
}

#if defined(BOOST_DECIMAL_DETAIL_INT128_HAS_INT128) || defined(BOOST_DECIMAL_DETAIL_INT128_HAS_MSVC_INT128)


BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_BUILTIN_CONSTEXPR uint128 operator+(const uint128 lhs, const detail::builtin_i128 rhs) noexcept
{
    return impl::default_add(lhs, static_cast<uint128>(rhs));
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_BUILTIN_CONSTEXPR uint128 operator+(const detail::builtin_i128 lhs, const uint128 rhs) noexcept
{
    return impl::default_add(static_cast<uint128>(lhs), rhs);
}


BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_BUILTIN_CONSTEXPR uint128 operator+(const uint128 lhs, const detail::builtin_u128 rhs) noexcept
{
    return impl::default_add(lhs, static_cast<uint128>(rhs));
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_BUILTIN_CONSTEXPR uint128 operator+(const detail::builtin_u128 lhs, const uint128 rhs) noexcept
{
    return impl::default_add(static_cast<uint128>(lhs), rhs);
}

#endif // BOOST_DECIMAL_DETAIL_INT128_HAS_INT128

template <BOOST_DECIMAL_DETAIL_INT128_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128& uint128::operator+=(const Integer rhs) noexcept
{
    *this = *this + rhs;
    return *this;
}

BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128& uint128::operator+=(const uint128 rhs) noexcept
{
    *this = *this + rhs;
    return *this;
}

#ifdef BOOST_DECIMAL_DETAIL_INT128_HAS_MSVC_INT128

template <BOOST_DECIMAL_DETAIL_INT128_128BIT_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE inline uint128& uint128::operator+=(const Integer rhs) noexcept
{
    *this = *this + rhs;
    return *this;
}

#endif // BOOST_DECIMAL_DETAIL_INT128_HAS_MSVC_INT128


//=====================================
// Subtraction Operator
//=====================================

#ifdef _MSC_VER
#  pragma warning(push)
#  pragma warning(disable : 4146)
#endif

BOOST_DECIMAL_DETAIL_INT128_EXPORT template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_SIGNED_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128 operator-(const uint128 lhs, const SignedInteger rhs) noexcept
{
    return rhs < 0 ? impl::default_add(lhs, -static_cast<std::uint64_t>(rhs)) :
                     impl::default_sub(lhs, static_cast<std::uint64_t>(rhs));
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_SIGNED_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128 operator-(const SignedInteger lhs, const uint128 rhs) noexcept
{
    return lhs < 0 ? impl::default_sub(-rhs, -static_cast<std::uint64_t>(lhs)) :
                     impl::default_add(-rhs, static_cast<std::uint64_t>(lhs));
}

#ifdef _MSC_VER
#  pragma warning(pop)
#endif

BOOST_DECIMAL_DETAIL_INT128_EXPORT template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_UNSIGNED_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128 operator-(const uint128 lhs, const UnsignedInteger rhs) noexcept
{
    return impl::default_sub(lhs, static_cast<std::uint64_t>(rhs));
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_UNSIGNED_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128 operator-(const UnsignedInteger lhs, const uint128 rhs) noexcept
{
    return impl::default_add(-rhs, static_cast<std::uint64_t>(lhs));
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128 operator-(const uint128 lhs, const uint128 rhs) noexcept
{
    return impl::default_sub(lhs, rhs);
}

#if defined(BOOST_DECIMAL_DETAIL_INT128_HAS_INT128) || defined(BOOST_DECIMAL_DETAIL_INT128_HAS_MSVC_INT128)


BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_BUILTIN_CONSTEXPR uint128 operator-(const uint128 lhs, const detail::builtin_i128 rhs) noexcept
{
    return lhs - static_cast<uint128>(rhs);
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_BUILTIN_CONSTEXPR uint128 operator-(const detail::builtin_i128 lhs, const uint128 rhs) noexcept
{
    return static_cast<uint128>(lhs) - rhs;
}


BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_BUILTIN_CONSTEXPR uint128 operator-(const uint128 lhs, const detail::builtin_u128 rhs) noexcept
{
    return lhs - static_cast<uint128>(rhs);
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_BUILTIN_CONSTEXPR uint128 operator-(const detail::builtin_u128 lhs, const uint128 rhs) noexcept
{
    return static_cast<uint128>(lhs) - rhs;
}

#endif // BOOST_DECIMAL_DETAIL_INT128_HAS_INT128

template <BOOST_DECIMAL_DETAIL_INT128_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128& uint128::operator-=(const Integer rhs) noexcept
{
    *this = *this - rhs;
    return *this;
}

BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128& uint128::operator-=(const uint128 rhs) noexcept
{
    *this = *this - rhs;
    return *this;
}

#ifdef BOOST_DECIMAL_DETAIL_INT128_HAS_MSVC_INT128

template <BOOST_DECIMAL_DETAIL_INT128_128BIT_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE inline uint128& uint128::operator-=(const Integer rhs) noexcept
{
    *this = *this - rhs;
    return *this;
}

#endif // BOOST_DECIMAL_DETAIL_INT128_HAS_MSVC_INT128

//=====================================
// Multiplication Operator
//=====================================

#if defined(__GNUC__) && __GNUC__ >= 8
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wclass-memaccess"
#endif

namespace detail {

#if defined(_M_AMD64) && !defined(__GNUC__)

BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_FORCE_INLINE uint128 msvc_mul(const uint128 lhs, const uint128 rhs) noexcept
{
    uint128 result {};
    result.low = _umul128(lhs.low, rhs.low, &result.high);
    result.high += lhs.low * rhs.high;
    result.high += lhs.high * rhs.low;

    return result;
}

BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_FORCE_INLINE uint128 msvc_mul(const uint128 lhs, const std::uint64_t rhs) noexcept
{
    uint128 result {};
    result.low = _umul128(lhs.low, rhs, &result.high);
    result.high += lhs.high * rhs;

    return result;
}

BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_FORCE_INLINE uint128 msvc_mul(const uint128 lhs, const std::uint32_t rhs) noexcept
{
    uint128 result {};
    result.low = _umul128(lhs.low, static_cast<std::uint64_t>(rhs), &result.high);
    result.high += lhs.high * static_cast<std::uint64_t>(rhs);

    return result;
}

#elif defined(_M_ARM64) && !defined(__GNUC__)

BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_FORCE_INLINE uint128 msvc_mul(const uint128 lhs, const uint128 rhs) noexcept
{
    const auto low_low{lhs.low * rhs.low};
    const auto high_low_low{__umulh(lhs.low, rhs.low)};

    const auto low_high{lhs.low * rhs.high};
    const auto high_low{lhs.high * rhs.low};

    const auto high{high_low + low_high + high_low_low};

    return {high, low_low};
}

BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_FORCE_INLINE uint128 msvc_mul(const uint128 lhs, const std::uint64_t rhs) noexcept
{
    const auto low{lhs.low * rhs};
    const auto high{__umulh(lhs.low, rhs) + (lhs.high * rhs)};

    return {high, low};
}

BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_FORCE_INLINE uint128 msvc_mul(const uint128 lhs, const std::uint32_t rhs) noexcept
{
    const auto low{lhs.low * rhs};
    const auto high{__umulh(lhs.low, static_cast<std::uint64_t>(rhs)) + (lhs.high * rhs)};

    return {high, low};
}

#endif // MSVC implementations

template <typename UnsignedInteger>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_FORCE_INLINE constexpr uint128 default_mul(const uint128 lhs, const UnsignedInteger rhs) noexcept
{
    #if (defined(__aarch64__) || defined(__x86_64__) || defined(__PPC__) || defined(__powerpc__)) && defined(__GNUC__) && defined(BOOST_DECIMAL_DETAIL_INT128_HAS_INT128)

    #  if !defined(BOOST_DECIMAL_DETAIL_INT128_NO_CONSTEVAL_DETECTION)

    if (!BOOST_DECIMAL_DETAIL_INT128_IS_CONSTANT_EVALUATED(lhs))
    {
        detail::builtin_u128 new_lhs {};
        detail::builtin_u128 new_rhs {};

        std::memcpy(&new_lhs, &lhs, sizeof(uint128));
        std::memcpy(&new_rhs, &rhs, sizeof(UnsignedInteger));

        const auto res {new_lhs * new_rhs};

        uint128 library_res {};

        std::memcpy(&library_res, &res, sizeof(uint128));

        return library_res;
    }

    #  elif defined(BOOST_DECIMAL_DETAIL_INT128_HAS_INT128)
    #    define BOOST_DECIMAL_DETAIL_INT128_HIDE_MUL

        return static_cast<uint128>(static_cast<detail::builtin_u128>(lhs) * static_cast<detail::builtin_u128>(rhs));

    #  endif

    // s390x intentionally falls through to the synthetic low_word_mul below. Casting to builtin_u128
    // makes GCC reconstruct the value through a vector-unit stack round-trip that is several times
    // slower, and the memcpy path is unsafe for the narrow (scalar rhs) overloads on big-endian.
    #elif (defined(_M_AMD64) || defined(_M_ARM64)) && !defined(__GNUC__) && !defined(BOOST_DECIMAL_DETAIL_INT128_NO_CONSTEVAL_DETECTION)

    if (!BOOST_DECIMAL_DETAIL_INT128_IS_CONSTANT_EVALUATED(lhs))
    {
        return msvc_mul(lhs, rhs);
    }

    #elif defined(BOOST_DECIMAL_DETAIL_INT128_HAS_INT128) && !defined(__s390__) && !defined(__s390x__)
    #  define BOOST_DECIMAL_DETAIL_INT128_HIDE_MUL

    return static_cast<uint128>(static_cast<detail::builtin_u128>(lhs) * static_cast<detail::builtin_u128>(rhs));

    #endif

    // We need to hide this if we use a non-const eval method above to avoid a litany of cross-platform warnings
    #ifndef BOOST_DECIMAL_DETAIL_INT128_HIDE_MUL

    return low_word_mul<uint128>(lhs, rhs);

    #else
    #undef BOOST_DECIMAL_DETAIL_INT128_HIDE_MUL
    #endif //BOOST_DECIMAL_DETAIL_INT128_HIDE_MUL
}

} // namespace detail

#if defined(__GNUC__) && __GNUC__ >= 8
#  pragma GCC diagnostic pop
#endif

#ifdef _MSC_VER
#  pragma warning(push)
#  pragma warning(disable : 4146)
#endif

BOOST_DECIMAL_DETAIL_INT128_EXPORT template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_SIGNED_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128 operator*(const uint128 lhs, const SignedInteger rhs) noexcept
{
    using eval_type = detail::evaluation_type_t<SignedInteger>;

    const auto abs_rhs {rhs < 0 ? -static_cast<eval_type>(rhs) : static_cast<eval_type>(rhs)};
    const auto res {detail::default_mul(lhs, abs_rhs)};

    return rhs < 0 ? -res : res;
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_SIGNED_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128 operator*(const SignedInteger lhs, const uint128 rhs) noexcept
{
    using eval_type = detail::evaluation_type_t<SignedInteger>;

    const auto abs_lhs {lhs < 0 ? -static_cast<eval_type>(lhs) : static_cast<eval_type>(lhs)};
    const auto res {detail::default_mul(rhs, abs_lhs)};

    return lhs < 0 ? -res : res;
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_UNSIGNED_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128 operator*(const uint128 lhs, const UnsignedInteger rhs) noexcept
{
    return detail::default_mul(lhs, static_cast<std::uint64_t>(rhs));
}

#ifdef _MSC_VER
#  pragma warning(pop)
#endif

BOOST_DECIMAL_DETAIL_INT128_EXPORT template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_UNSIGNED_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128 operator*(const UnsignedInteger lhs, const uint128 rhs) noexcept
{
    return detail::default_mul(rhs, static_cast<std::uint64_t>(lhs));
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128 operator*(const uint128 lhs, const uint128 rhs) noexcept
{
    return detail::default_mul(lhs, rhs);
}

#if defined(BOOST_DECIMAL_DETAIL_INT128_HAS_INT128) || defined(BOOST_DECIMAL_DETAIL_INT128_HAS_MSVC_INT128)


BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_BUILTIN_CONSTEXPR uint128 operator*(const uint128 lhs, const detail::builtin_i128 rhs) noexcept
{
    const detail::builtin_u128 rhs_bits {static_cast<detail::builtin_u128>(rhs)};
    const bool rhs_negative {static_cast<std::int64_t>(static_cast<std::uint64_t>(rhs_bits >> static_cast<detail::builtin_u128>(64U))) < 0};
    const uint128 rhs_u {rhs_bits};
    const uint128 abs_rhs {rhs_negative ? -rhs_u : rhs_u};
    const uint128 res {lhs * abs_rhs};

    return rhs_negative ? -res : res;
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_BUILTIN_CONSTEXPR uint128 operator*(const detail::builtin_i128 lhs, const uint128 rhs) noexcept
{
    const detail::builtin_u128 lhs_bits {static_cast<detail::builtin_u128>(lhs)};
    const bool lhs_negative {static_cast<std::int64_t>(static_cast<std::uint64_t>(lhs_bits >> static_cast<detail::builtin_u128>(64U))) < 0};
    const uint128 lhs_u {lhs_bits};
    const uint128 abs_lhs {lhs_negative ? -lhs_u : lhs_u};
    const uint128 res {abs_lhs * rhs};

    return lhs_negative ? -res : res;
}


BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_BUILTIN_CONSTEXPR uint128 operator*(const uint128 lhs, const detail::builtin_u128 rhs) noexcept
{
    return lhs * static_cast<uint128>(rhs);
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_BUILTIN_CONSTEXPR uint128 operator*(const detail::builtin_u128 lhs, const uint128 rhs) noexcept
{
    return static_cast<uint128>(lhs) * rhs;
}

#endif // BOOST_DECIMAL_DETAIL_INT128_HAS_INT128

template <BOOST_DECIMAL_DETAIL_INT128_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128& uint128::operator*=(const Integer rhs) noexcept
{
    *this = *this * rhs;
    return *this;
}

BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128& uint128::operator*=(const uint128 rhs) noexcept
{
    *this = *this * rhs;
    return *this;
}

#ifdef BOOST_DECIMAL_DETAIL_INT128_HAS_MSVC_INT128

template <BOOST_DECIMAL_DETAIL_INT128_128BIT_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE inline uint128& uint128::operator*=(const Integer rhs) noexcept
{
    *this = *this * rhs;
    return *this;
}

#endif // BOOST_DECIMAL_DETAIL_INT128_HAS_MSVC_INT128

//=====================================
// Division Operator
//=====================================

// For div we need forward declarations since we mix and match the arguments
BOOST_DECIMAL_DETAIL_INT128_EXPORT template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_SIGNED_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128 operator/(uint128 lhs, SignedInteger rhs) noexcept;

BOOST_DECIMAL_DETAIL_INT128_EXPORT template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_SIGNED_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128 operator/(SignedInteger lhs, uint128 rhs) noexcept;

BOOST_DECIMAL_DETAIL_INT128_EXPORT template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_UNSIGNED_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128 operator/(uint128 lhs, UnsignedInteger rhs) noexcept;

BOOST_DECIMAL_DETAIL_INT128_EXPORT template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_UNSIGNED_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128 operator/(UnsignedInteger lhs, uint128 rhs) noexcept;

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128 operator/(uint128 lhs, uint128 rhs) noexcept;

template <BOOST_DECIMAL_DETAIL_INT128_SIGNED_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128 operator/(const uint128 lhs, const SignedInteger rhs) noexcept
{
    using eval_type = detail::evaluation_type_t<SignedInteger>;
    return rhs < 0 ? lhs / static_cast<uint128>(rhs) : lhs / static_cast<eval_type>(rhs);
}

template <BOOST_DECIMAL_DETAIL_INT128_SIGNED_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128 operator/(const SignedInteger lhs, const uint128 rhs) noexcept
{
    using eval_type = detail::evaluation_type_t<SignedInteger>;
    return lhs < 0 ? static_cast<uint128>(lhs) / rhs : static_cast<eval_type>(lhs) / rhs;
}

template <BOOST_DECIMAL_DETAIL_INT128_UNSIGNED_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128 operator/(const uint128 lhs, const UnsignedInteger rhs) noexcept
{
    using eval_type = detail::evaluation_type_t<UnsignedInteger>;

    if (BOOST_DECIMAL_DETAIL_INT128_UNLIKELY(rhs == 0U))
    {
        // Division or remainder by zero is undefined behavior for the builtin __int128 types (a hardware trap). We match that: marking it unreachable keeps codegen branch-free and vectorizable.
        BOOST_DECIMAL_DETAIL_INT128_UNREACHABLE;
    }

    if (lhs < rhs)
    {
        return {0, 0};
    }

    uint128 quotient {};

    detail::one_word_div(lhs, static_cast<eval_type>(rhs), quotient);

    return quotient;
}

template <BOOST_DECIMAL_DETAIL_INT128_UNSIGNED_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128 operator/(const UnsignedInteger lhs, const uint128 rhs) noexcept
{
    using eval_type = detail::evaluation_type_t<UnsignedInteger>;

    if (BOOST_DECIMAL_DETAIL_INT128_UNLIKELY(rhs == 0U))
    {
        // Division or remainder by zero is undefined behavior for the builtin __int128 types (a hardware trap). We match that: marking it unreachable keeps codegen branch-free and vectorizable.
        BOOST_DECIMAL_DETAIL_INT128_UNREACHABLE;
    }

    if (lhs < rhs)
    {
        return {0, 0};
    }

    return {0, static_cast<eval_type>(lhs) / rhs.low};
}

BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128 operator/(const uint128 lhs, const uint128 rhs) noexcept
{
    if (BOOST_DECIMAL_DETAIL_INT128_UNLIKELY(rhs == 0U))
    {
        // Division or remainder by zero is undefined behavior for the builtin __int128 types (a hardware trap). We match that: marking it unreachable keeps codegen branch-free and vectorizable.
        BOOST_DECIMAL_DETAIL_INT128_UNREACHABLE;
    }

    if (lhs < rhs)
    {
        return {0, 0};
    }

    // A divisor that fits in 64 bits is handled by the hardware-accelerated narrow path. This
    // beats the native 128/128 divide for this common case on every platform (it avoids the
    // out-of-line __udivti3 call on GCC/Clang and uses divq / _udiv128 directly where present).
    if (rhs.high == 0U)
    {
        if (lhs.high == 0U)
        {
            return {0, lhs.low / rhs.low};
        }

        uint128 quotient {};
        detail::one_word_div(lhs, rhs.low, quotient);
        return quotient;
    }

    #if defined(BOOST_DECIMAL_DETAIL_INT128_HAS_INT128) && !defined(__s390__) && !defined(__s390x__)

    #if defined(BOOST_DECIMAL_DETAIL_INT128_HAS_X86_64_DIVQ) && !defined(BOOST_DECIMAL_DETAIL_INT128_NO_CONSTEVAL_DETECTION)
    // Decimal-local: at runtime the divq based knuth_div beats the out-of-line __udivti3 call
    if (!BOOST_DECIMAL_DETAIL_INT128_IS_CONSTANT_EVALUATED(lhs))
    {
        return detail::knuth_div(lhs, rhs);
    }
    #endif

    return static_cast<uint128>(static_cast<detail::builtin_u128>(lhs) / static_cast<detail::builtin_u128>(rhs));

    #else

    return detail::knuth_div(lhs, rhs);

    #endif
}

#if defined(BOOST_DECIMAL_DETAIL_INT128_HAS_INT128) || defined(BOOST_DECIMAL_DETAIL_INT128_HAS_MSVC_INT128)

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_BUILTIN_CONSTEXPR uint128 operator/(const uint128 lhs, const detail::builtin_u128 rhs) noexcept
{
    return lhs / static_cast<uint128>(rhs);
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_BUILTIN_CONSTEXPR uint128 operator/(const detail::builtin_u128 lhs, const uint128 rhs) noexcept
{
    return static_cast<uint128>(lhs) / rhs;
}


BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_BUILTIN_CONSTEXPR uint128 operator/(const uint128 lhs, const detail::builtin_i128 rhs) noexcept
{
    return lhs / static_cast<uint128>(rhs);
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_BUILTIN_CONSTEXPR uint128 operator/(const detail::builtin_i128 lhs, const uint128 rhs) noexcept
{
    return static_cast<uint128>(lhs) / rhs;
}


#endif // BOOST_DECIMAL_DETAIL_INT128_HAS_INT128

template <BOOST_DECIMAL_DETAIL_INT128_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128& uint128::operator/=(const Integer rhs) noexcept
{
    *this = *this / rhs;
    return *this;
}

BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128& uint128::operator/=(const uint128 rhs) noexcept
{
    *this = *this / rhs;
    return *this;
}

#ifdef BOOST_DECIMAL_DETAIL_INT128_HAS_MSVC_INT128

template <BOOST_DECIMAL_DETAIL_INT128_128BIT_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE inline uint128& uint128::operator/=(const Integer rhs) noexcept
{
    *this = *this / rhs;
    return *this;
}

#endif // BOOST_DECIMAL_DETAIL_INT128_HAS_MSVC_INT128

#if defined(__clang__)
#  pragma clang diagnostic pop
#elif defined(__GNUC__)
#  pragma GCC diagnostic pop
#endif

//=====================================
// Modulo Operator
//=====================================

// For div we need forward declarations since we mix and match the arguments
BOOST_DECIMAL_DETAIL_INT128_EXPORT template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_SIGNED_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128 operator%(uint128 lhs, SignedInteger rhs) noexcept;

BOOST_DECIMAL_DETAIL_INT128_EXPORT template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_SIGNED_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128 operator%(SignedInteger lhs, uint128 rhs) noexcept;

BOOST_DECIMAL_DETAIL_INT128_EXPORT template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_UNSIGNED_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128 operator%(uint128 lhs, UnsignedInteger rhs) noexcept;

BOOST_DECIMAL_DETAIL_INT128_EXPORT template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_UNSIGNED_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128 operator%(UnsignedInteger lhs, uint128 rhs) noexcept;

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128 operator%(uint128 lhs, uint128 rhs) noexcept;

template <BOOST_DECIMAL_DETAIL_INT128_SIGNED_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128 operator%(const uint128 lhs, const SignedInteger rhs) noexcept
{
    using eval_type = detail::evaluation_type_t<SignedInteger>;
    return rhs < 0 ? lhs % static_cast<uint128>(rhs) : lhs % static_cast<eval_type>(rhs);
}

template <BOOST_DECIMAL_DETAIL_INT128_SIGNED_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128 operator%(const SignedInteger lhs, const uint128 rhs) noexcept
{
    using eval_type = detail::evaluation_type_t<SignedInteger>;
    return lhs < 0 ? static_cast<uint128>(lhs) % rhs : static_cast<eval_type>(lhs) % rhs;
}

template <BOOST_DECIMAL_DETAIL_INT128_UNSIGNED_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128 operator%(const uint128 lhs, const UnsignedInteger rhs) noexcept
{
    using eval_type = detail::evaluation_type_t<UnsignedInteger>;

    if (BOOST_DECIMAL_DETAIL_INT128_UNLIKELY(rhs == 0U))
    {
        // Division or remainder by zero is undefined behavior for the builtin __int128 types (a hardware trap). We match that: marking it unreachable keeps codegen branch-free and vectorizable.
        BOOST_DECIMAL_DETAIL_INT128_UNREACHABLE;
    }

    if (lhs.high != 0)
    {
        uint128 quotient {};
        uint128 remainder {};

        detail::one_word_div(lhs, static_cast<eval_type>(rhs), quotient, remainder);

        return remainder;
    }
    else
    {
        return {0, lhs.low % rhs};
    }
}

template <BOOST_DECIMAL_DETAIL_INT128_UNSIGNED_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128 operator%(const UnsignedInteger lhs, const uint128 rhs) noexcept
{
    using eval_type = detail::evaluation_type_t<UnsignedInteger>;

    if (BOOST_DECIMAL_DETAIL_INT128_UNLIKELY(rhs == 0U))
    {
        // Division or remainder by zero is undefined behavior for the builtin __int128 types (a hardware trap). We match that: marking it unreachable keeps codegen branch-free and vectorizable.
        BOOST_DECIMAL_DETAIL_INT128_UNREACHABLE;
    }
    else if (rhs > lhs)
    {
        return lhs;
    }

    return {0, static_cast<eval_type>(lhs) % rhs.low};
}

BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128 operator%(const uint128 lhs, const uint128 rhs) noexcept
{
    if (BOOST_DECIMAL_DETAIL_INT128_UNLIKELY(rhs == 0U))
    {
        // Division or remainder by zero is undefined behavior for the builtin __int128 types (a hardware trap). We match that: marking it unreachable keeps codegen branch-free and vectorizable.
        BOOST_DECIMAL_DETAIL_INT128_UNREACHABLE;
    }
    if (rhs > lhs)
    {
        return lhs;
    }

    // A divisor that fits in 64 bits is handled by the hardware-accelerated narrow path, which
    // beats the native 128/128 divide for this common case on every platform.
    if (rhs.high == 0U)
    {
        if (lhs.high == 0U)
        {
            return {0, lhs.low % rhs.low};
        }

        uint128 quotient {};
        uint128 remainder {};
        detail::one_word_div(lhs, rhs.low, quotient, remainder);
        return remainder;
    }

    #if defined(BOOST_DECIMAL_DETAIL_INT128_HAS_INT128) && !defined(__s390__) && !defined(__s390x__)

    #if defined(BOOST_DECIMAL_DETAIL_INT128_HAS_X86_64_DIVQ) && !defined(BOOST_DECIMAL_DETAIL_INT128_NO_CONSTEVAL_DETECTION)
    // Decimal-local: at runtime the divq based knuth_div beats the out-of-line __udivti3 call
    if (!BOOST_DECIMAL_DETAIL_INT128_IS_CONSTANT_EVALUATED(lhs))
    {
        uint128 remainder {};
        detail::knuth_div(lhs, rhs, remainder);
        return remainder;
    }
    #endif

    return static_cast<uint128>(static_cast<detail::builtin_u128>(lhs) % static_cast<detail::builtin_u128>(rhs));

    #else

    uint128 remainder {};
    detail::knuth_div(lhs, rhs, remainder);
    return remainder;

    #endif
}

#if defined(BOOST_DECIMAL_DETAIL_INT128_HAS_INT128) || defined(BOOST_DECIMAL_DETAIL_INT128_HAS_MSVC_INT128)

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_BUILTIN_CONSTEXPR uint128 operator%(const uint128 lhs, const detail::builtin_u128 rhs) noexcept
{
    return lhs % static_cast<uint128>(rhs);
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_BUILTIN_CONSTEXPR uint128 operator%(const detail::builtin_u128 lhs, const uint128 rhs) noexcept
{
    return static_cast<uint128>(lhs) % rhs;
}


BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_BUILTIN_CONSTEXPR uint128 operator%(const uint128 lhs, const detail::builtin_i128 rhs) noexcept
{
    return lhs % static_cast<uint128>(rhs);
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_BUILTIN_CONSTEXPR uint128 operator%(const detail::builtin_i128 lhs, const uint128 rhs) noexcept
{
    return static_cast<uint128>(lhs) % rhs;
}


#endif // BOOST_DECIMAL_DETAIL_INT128_HAS_INT128

template <BOOST_DECIMAL_DETAIL_INT128_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128& uint128::operator%=(const Integer rhs) noexcept
{
    *this = *this % rhs;
    return *this;
}

BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128& uint128::operator%=(const uint128 rhs) noexcept
{
    *this = *this % rhs;
    return *this;
}

#ifdef BOOST_DECIMAL_DETAIL_INT128_HAS_MSVC_INT128

template <BOOST_DECIMAL_DETAIL_INT128_128BIT_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE inline uint128& uint128::operator%=(const Integer rhs) noexcept
{
    * this = *this % rhs;
    return *this;
}

#endif // BOOST_DECIMAL_DETAIL_INT128_HAS_MSVC_INT128

//=====================================
// Built-in Integer Compound Assignment
//=====================================

// Compound assignment with a built-in integer on the left.
// The builtin applies the operation to the common type of the two operands and converts
// the result back to the type of the left operand, so each of these is the binary operator
// above followed by that conversion, which matches what the builtin 128-bit integer does.
// detail/traits.hpp defines which types Integer may be

#ifdef _MSC_VER
#  pragma warning(push)
#  pragma warning(disable : 4804) // Unsafe use of type bool in operation
#endif

#define BOOST_DECIMAL_DETAIL_INT128_DETAIL_U128_INTEGER_COMPOUND_OP(op, compound_op)                                          \
    BOOST_DECIMAL_DETAIL_INT128_EXPORT template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_INTEGER_CONCEPT>                                      \
    BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr Integer& operator compound_op(Integer& lhs, const uint128 rhs) noexcept \
    {                                                                                                          \
        lhs = static_cast<Integer>(lhs op rhs);                                                                \
        return lhs;                                                                                            \
    }

BOOST_DECIMAL_DETAIL_INT128_DETAIL_U128_INTEGER_COMPOUND_OP(|, |=)
BOOST_DECIMAL_DETAIL_INT128_DETAIL_U128_INTEGER_COMPOUND_OP(&, &=)
BOOST_DECIMAL_DETAIL_INT128_DETAIL_U128_INTEGER_COMPOUND_OP(^, ^=)
BOOST_DECIMAL_DETAIL_INT128_DETAIL_U128_INTEGER_COMPOUND_OP(+, +=)
BOOST_DECIMAL_DETAIL_INT128_DETAIL_U128_INTEGER_COMPOUND_OP(-, -=)
BOOST_DECIMAL_DETAIL_INT128_DETAIL_U128_INTEGER_COMPOUND_OP(*, *=)
BOOST_DECIMAL_DETAIL_INT128_DETAIL_U128_INTEGER_COMPOUND_OP(/, /=)
BOOST_DECIMAL_DETAIL_INT128_DETAIL_U128_INTEGER_COMPOUND_OP(%, %=)

// The shifts take the value from the left operand alone, so only the count comes from rhs
BOOST_DECIMAL_DETAIL_INT128_DETAIL_U128_INTEGER_COMPOUND_OP(<<, <<=)
BOOST_DECIMAL_DETAIL_INT128_DETAIL_U128_INTEGER_COMPOUND_OP(>>, >>=)

#undef BOOST_DECIMAL_DETAIL_INT128_DETAIL_U128_INTEGER_COMPOUND_OP

#ifdef _MSC_VER
#  pragma warning(pop)
#endif

//=====================================
// Floating Point Operators
//=====================================

// The usual arithmetic conversions convert the integer operand to the floating point type
// before the operation is applied, so each of these computes exactly what the builtin
// 128-bit integer computes for the same expression.
// detail/traits.hpp defines which types Float may be

#ifdef __GNUC__
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wfloat-equal"
#elif defined(__clang__)
#  pragma clang diagnostic push
#  pragma clang diagnostic ignored "-Wfloat-equal"
#endif

#define BOOST_DECIMAL_DETAIL_INT128_DETAIL_U128_FLOAT_BINARY_OP(op, return_type)                                       \
    BOOST_DECIMAL_DETAIL_INT128_EXPORT template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_FLOATING_POINT_CONCEPT>                         \
    BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr return_type operator op(const uint128 lhs, const Float rhs) noexcept \
    {                                                                                                    \
        return static_cast<Float>(lhs) op rhs;                                                           \
    }                                                                                                    \
                                                                                                         \
    BOOST_DECIMAL_DETAIL_INT128_EXPORT template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_FLOATING_POINT_CONCEPT>                         \
    BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr return_type operator op(const Float lhs, const uint128 rhs) noexcept \
    {                                                                                                    \
        return lhs op static_cast<Float>(rhs);                                                           \
    }

BOOST_DECIMAL_DETAIL_INT128_DETAIL_U128_FLOAT_BINARY_OP(+, Float)
BOOST_DECIMAL_DETAIL_INT128_DETAIL_U128_FLOAT_BINARY_OP(-, Float)
BOOST_DECIMAL_DETAIL_INT128_DETAIL_U128_FLOAT_BINARY_OP(*, Float)
BOOST_DECIMAL_DETAIL_INT128_DETAIL_U128_FLOAT_BINARY_OP(/, Float)

BOOST_DECIMAL_DETAIL_INT128_DETAIL_U128_FLOAT_BINARY_OP(==, bool)
BOOST_DECIMAL_DETAIL_INT128_DETAIL_U128_FLOAT_BINARY_OP(!=, bool)
BOOST_DECIMAL_DETAIL_INT128_DETAIL_U128_FLOAT_BINARY_OP(<, bool)
BOOST_DECIMAL_DETAIL_INT128_DETAIL_U128_FLOAT_BINARY_OP(<=, bool)
BOOST_DECIMAL_DETAIL_INT128_DETAIL_U128_FLOAT_BINARY_OP(>, bool)
BOOST_DECIMAL_DETAIL_INT128_DETAIL_U128_FLOAT_BINARY_OP(>=, bool)

// Mixing an integer and a floating point type yields a partial ordering because of NaN
#ifdef BOOST_DECIMAL_DETAIL_INT128_HAS_SPACESHIP_OPERATOR

BOOST_DECIMAL_DETAIL_INT128_DETAIL_U128_FLOAT_BINARY_OP(<=>, std::partial_ordering)

#endif // BOOST_DECIMAL_DETAIL_INT128_HAS_SPACESHIP_OPERATOR

#undef BOOST_DECIMAL_DETAIL_INT128_DETAIL_U128_FLOAT_BINARY_OP

// Compound assignment converts the result back to uint128, truncating toward zero.
// A result that is NaN or outside the range of the type saturates as the floating point
// constructor does, rather than being undefined as it is for the builtin

#define BOOST_DECIMAL_DETAIL_INT128_DETAIL_U128_FLOAT_COMPOUND_OP(op, compound_op)                                     \
    template <BOOST_DECIMAL_DETAIL_INT128_FLOATING_POINT_CONCEPT>                                                       \
    BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128& uint128::operator compound_op(const Float rhs) noexcept   \
    {                                                                                                    \
        *this = static_cast<uint128>(static_cast<Float>(*this) op rhs);                                   \
        return *this;                                                                                     \
    }                                                                                                    \
                                                                                                         \
    BOOST_DECIMAL_DETAIL_INT128_EXPORT template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_FLOATING_POINT_CONCEPT>                         \
    BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr Float& operator compound_op(Float& lhs, const uint128 rhs) noexcept \
    {                                                                                                    \
        lhs compound_op static_cast<Float>(rhs);                                                          \
        return lhs;                                                                                       \
    }

BOOST_DECIMAL_DETAIL_INT128_DETAIL_U128_FLOAT_COMPOUND_OP(+, +=)
BOOST_DECIMAL_DETAIL_INT128_DETAIL_U128_FLOAT_COMPOUND_OP(-, -=)
BOOST_DECIMAL_DETAIL_INT128_DETAIL_U128_FLOAT_COMPOUND_OP(*, *=)
BOOST_DECIMAL_DETAIL_INT128_DETAIL_U128_FLOAT_COMPOUND_OP(/, /=)

#undef BOOST_DECIMAL_DETAIL_INT128_DETAIL_U128_FLOAT_COMPOUND_OP

#ifdef __GNUC__
#  pragma GCC diagnostic pop
#elif defined(__clang__)
#  pragma clang diagnostic pop
#endif

// The builtin allows no floating point operand for the modulo, bitwise and shift operators.
// Deleting them keeps that a compile error here, rather than letting the implicit floating
// point constructor silently truncate the operand

#define BOOST_DECIMAL_DETAIL_INT128_DETAIL_U128_FLOAT_DELETED_OP(op)                                                   \
    BOOST_DECIMAL_DETAIL_INT128_EXPORT template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_FLOATING_POINT_CONCEPT>                         \
    BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE uint128 operator op(uint128 lhs, Float rhs) = delete;                        \
                                                                                                         \
    BOOST_DECIMAL_DETAIL_INT128_EXPORT template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_FLOATING_POINT_CONCEPT>                         \
    BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE uint128 operator op(Float lhs, uint128 rhs) = delete;

BOOST_DECIMAL_DETAIL_INT128_DETAIL_U128_FLOAT_DELETED_OP(%)
BOOST_DECIMAL_DETAIL_INT128_DETAIL_U128_FLOAT_DELETED_OP(&)
BOOST_DECIMAL_DETAIL_INT128_DETAIL_U128_FLOAT_DELETED_OP(|)
BOOST_DECIMAL_DETAIL_INT128_DETAIL_U128_FLOAT_DELETED_OP(^)
BOOST_DECIMAL_DETAIL_INT128_DETAIL_U128_FLOAT_DELETED_OP(<<)
BOOST_DECIMAL_DETAIL_INT128_DETAIL_U128_FLOAT_DELETED_OP(>>)

#undef BOOST_DECIMAL_DETAIL_INT128_DETAIL_U128_FLOAT_DELETED_OP

namespace detail {

template <bool>
class numeric_limits_impl_u128
{
public:

        // Member constants
    static constexpr bool is_specialized = true;
    static constexpr bool is_signed = false;
    static constexpr bool is_integer = true;
    static constexpr bool is_exact = true;
    static constexpr bool has_infinity = false;
    static constexpr bool has_quiet_NaN = false;
    static constexpr bool has_signaling_NaN = false;

    // C++23 deprecated the following two members
    #if defined(__GNUC__) && __cplusplus > 202002L
    #  pragma GCC diagnostic push
    #  pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    #elif defined(_MSC_VER)
    #  pragma warning(push)
    #  pragma warning(disable:4996)
    #endif

    static constexpr std::float_denorm_style has_denorm = std::denorm_absent;
    static constexpr bool has_denorm_loss = false;

    #if defined(__GNUC__) && __cplusplus > 202002L
    #  pragma GCC diagnostic pop
    #elif defined(_MSC_VER)
    #  pragma warning(pop)
    #endif

    static constexpr std::float_round_style round_style = std::round_toward_zero;
    static constexpr bool is_iec559 = false;
    static constexpr bool is_bounded = true;
    static constexpr bool is_modulo = true;
    static constexpr int digits = 128;
    static constexpr int digits10 = 38;
    static constexpr int max_digits10 = 0;
    static constexpr int radix = 2;
    static constexpr int min_exponent = 0;
    static constexpr int min_exponent10 = 0;
    static constexpr int max_exponent = 0;
    static constexpr int max_exponent10 = 0;
    static constexpr bool traps = std::numeric_limits<std::uint64_t>::traps;
    static constexpr bool tinyness_before = false;

    // Member functions
    BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE static constexpr auto (min)        () -> boost::int128::uint128 { return {0, 0}; }
    BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE static constexpr auto lowest       () -> boost::int128::uint128 { return {0, 0}; }
    BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE static constexpr auto (max)        () -> boost::int128::uint128 { return {UINT64_MAX, UINT64_MAX}; }
    BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE static constexpr auto epsilon      () -> boost::int128::uint128 { return {0, 0}; }
    BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE static constexpr auto round_error  () -> boost::int128::uint128 { return {0, 0}; }
    BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE static constexpr auto infinity     () -> boost::int128::uint128 { return {0, 0}; }
    BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE static constexpr auto quiet_NaN    () -> boost::int128::uint128 { return {0, 0}; }
    BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE static constexpr auto signaling_NaN() -> boost::int128::uint128 { return {0, 0}; }
    BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE static constexpr auto denorm_min   () -> boost::int128::uint128 { return {0, 0}; }
};

#if !defined(__cpp_inline_variables) || __cpp_inline_variables < 201606L

template <bool b> constexpr bool numeric_limits_impl_u128<b>::is_specialized;
template <bool b> constexpr bool numeric_limits_impl_u128<b>::is_signed;
template <bool b> constexpr bool numeric_limits_impl_u128<b>::is_integer;
template <bool b> constexpr bool numeric_limits_impl_u128<b>::is_exact;
template <bool b> constexpr bool numeric_limits_impl_u128<b>::has_infinity;
template <bool b> constexpr bool numeric_limits_impl_u128<b>::has_quiet_NaN;
template <bool b> constexpr bool numeric_limits_impl_u128<b>::has_signaling_NaN;

// These members were deprecated in C++23; suppress the deprecation warning rather
// than dropping the definitions.
#if defined(__GNUC__) && __cplusplus > 202002L
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#elif defined(_MSC_VER)
#  pragma warning(push)
#  pragma warning(disable:4996)
#endif

template <bool b> constexpr std::float_denorm_style numeric_limits_impl_u128<b>::has_denorm;
template <bool b> constexpr bool numeric_limits_impl_u128<b>::has_denorm_loss;

#if defined(__GNUC__) && __cplusplus > 202002L
#  pragma GCC diagnostic pop
#elif defined(_MSC_VER)
#  pragma warning(pop)
#endif

template <bool b> constexpr std::float_round_style numeric_limits_impl_u128<b>::round_style;
template <bool b> constexpr bool numeric_limits_impl_u128<b>::is_iec559;
template <bool b> constexpr bool numeric_limits_impl_u128<b>::is_bounded;
template <bool b> constexpr bool numeric_limits_impl_u128<b>::is_modulo;
template <bool b> constexpr int numeric_limits_impl_u128<b>::digits;
template <bool b> constexpr int numeric_limits_impl_u128<b>::digits10;
template <bool b> constexpr int numeric_limits_impl_u128<b>::max_digits10;
template <bool b> constexpr int numeric_limits_impl_u128<b>::radix;
template <bool b> constexpr int numeric_limits_impl_u128<b>::min_exponent;
template <bool b> constexpr int numeric_limits_impl_u128<b>::min_exponent10;
template <bool b> constexpr int numeric_limits_impl_u128<b>::max_exponent;
template <bool b> constexpr int numeric_limits_impl_u128<b>::max_exponent10;
template <bool b> constexpr bool numeric_limits_impl_u128<b>::traps;
template <bool b> constexpr bool numeric_limits_impl_u128<b>::tinyness_before;

#endif // !defined(__cpp_inline_variables) || __cpp_inline_variables < 201606L


} // namespace detail

} // namespace int128
} // namespace boost

namespace std {

#ifdef __clang__
#  pragma clang diagnostic push
#  pragma clang diagnostic ignored "-Wmismatched-tags"
#endif

template <>
class numeric_limits<boost::int128::uint128> :
    public boost::int128::detail::numeric_limits_impl_u128<true> {};

#ifdef __clang__
#  pragma clang diagnostic pop
#endif

} // namespace std

#endif //BOOST_DECIMAL_DETAIL_INT128_DETAIL_UINT128_IMP_HPP
