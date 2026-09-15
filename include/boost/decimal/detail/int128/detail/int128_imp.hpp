// Copyright 2025 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_DECIMAL_DETAIL_INT128_DETAIL_INT128_HPP
#define BOOST_DECIMAL_DETAIL_INT128_DETAIL_INT128_HPP

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

#endif

namespace boost {
namespace int128 {

struct
    #if (defined(BOOST_DECIMAL_DETAIL_INT128_HAS_INT128) || defined(BOOST_DECIMAL_DETAIL_INT128_HAS_MSVC_INT128)) && !defined(_M_IX86)
    alignas(alignof(detail::builtin_i128))
    #endif
int128
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

    #endif

    // Defaulted basic construction
    constexpr int128() noexcept = default;
    constexpr int128(const int128&) noexcept = default;
    constexpr int128(int128&&) noexcept = default;
    constexpr int128& operator=(const int128&) noexcept = default;
    constexpr int128& operator=(int128&&) noexcept = default;

    // Requires a conversion file to be implemented
    BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int128(const uint128& v) noexcept;

    // Construct from integral types
    #if BOOST_DECIMAL_DETAIL_INT128_ENDIAN_LITTLE_BYTE

    BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int128(const std::int64_t hi, const std::uint64_t lo) noexcept : low{lo}, high{static_cast<std::uint64_t>(hi)} {}

    template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_SIGNED_INTEGER_CONCEPT>
    BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int128(const SignedInteger v) noexcept : low {static_cast<std::uint64_t>(v)}, high {v < 0 ? ~UINT64_C(0) : UINT64_C(0)} {}

    template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_UNSIGNED_INTEGER_CONCEPT>
    BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int128(const UnsignedInteger v) noexcept : low {static_cast<std::uint64_t>(v)}, high {} {}

    #if defined(BOOST_DECIMAL_DETAIL_INT128_HAS_INT128) || defined(BOOST_DECIMAL_DETAIL_INT128_HAS_MSVC_INT128)

    BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_BUILTIN_CONSTEXPR int128(const detail::builtin_i128 v) noexcept : low {static_cast<std::uint64_t>(v & static_cast<detail::builtin_i128>(detail::low_word_mask))}, high {static_cast<std::uint64_t>(v >> static_cast<detail::builtin_i128>(64U))} {}
    BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_BUILTIN_CONSTEXPR int128(const detail::builtin_u128 v) noexcept : low {static_cast<std::uint64_t>(v & static_cast<detail::builtin_u128>(detail::low_word_mask))}, high {static_cast<std::uint64_t>(v >> static_cast<detail::builtin_u128>(64U))} {}

    #endif // BOOST_DECIMAL_DETAIL_INT128_HAS_INT128

    #else // Big endian

    BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int128(const std::int64_t hi, const std::uint64_t lo) noexcept : high{static_cast<std::uint64_t>(hi)}, low{lo} {}

    template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_SIGNED_INTEGER_CONCEPT>
    BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int128(const SignedInteger v) noexcept : high{v < 0 ? ~UINT64_C(0) : UINT64_C(0)}, low{static_cast<std::uint64_t>(v)} {}

    template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_UNSIGNED_INTEGER_CONCEPT>
    BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int128(const UnsignedInteger v) noexcept : high {}, low {static_cast<std::uint64_t>(v)} {}

    #if defined(BOOST_DECIMAL_DETAIL_INT128_HAS_INT128) || defined(BOOST_DECIMAL_DETAIL_INT128_HAS_MSVC_INT128)

    BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_BUILTIN_CONSTEXPR int128(const detail::builtin_i128 v) noexcept : high {static_cast<std::uint64_t>(v >> 64U)}, low {static_cast<std::uint64_t>(v & detail::low_word_mask)} {}
    BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_BUILTIN_CONSTEXPR int128(const detail::builtin_u128 v) noexcept : high {static_cast<std::uint64_t>(v >> 64U)}, low {static_cast<std::uint64_t>(v & detail::low_word_mask)} {}

    #endif // BOOST_DECIMAL_DETAIL_INT128_HAS_INT128

    #endif // BOOST_DECIMAL_DETAIL_INT128_ENDIAN_LITTLE_BYTE

    // Construct from floating-point types
    template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_FLOATING_POINT_CONCEPT>
    BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int128(Float f) noexcept;

    // The high word read as a signed value.
    // Every operation whose meaning depends on the sign of the value goes through
    // this rather than reading high directly.
    BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr std::int64_t signed_high() const noexcept { return static_cast<std::int64_t>(high); }

    // Integer Conversion operators
    BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE explicit constexpr operator bool() const noexcept { return low || high; }

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
    // Uses the builtin 128-bit conversion where one exists, and otherwise converts
    // the unsigned magnitude as high * 2^64 + low before applying the sign.
    // See detail/float_conversion.hpp for why the sign handling is required
    BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr operator float() const noexcept;
    BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr operator double() const noexcept;

    // Long double does not exist on the CUDA or SYCL (spir64) device
    #if !defined(BOOST_DECIMAL_DETAIL_INT128_HAS_GPU_SUPPORT)
    constexpr operator long double() const noexcept;
    #endif

    // Compound Or
    template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_INTEGER_CONCEPT>
    BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int128& operator|=(Integer rhs) noexcept;

    BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int128& operator|=(int128 rhs) noexcept;

    #ifdef BOOST_DECIMAL_DETAIL_INT128_HAS_MSVC_INT128

    template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_128BIT_INTEGER_CONCEPT>
    BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE inline int128& operator|=(Integer rhs) noexcept;

    #endif // BOOST_DECIMAL_DETAIL_INT128_HAS_MSVC_INT128

    // Compound And
    template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_INTEGER_CONCEPT>
    BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int128& operator&=(Integer rhs) noexcept;

    BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int128& operator&=(int128 rhs) noexcept;

    #ifdef BOOST_DECIMAL_DETAIL_INT128_HAS_MSVC_INT128

    template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_128BIT_INTEGER_CONCEPT>
    BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE inline int128& operator&=(Integer rhs) noexcept;

    #endif // BOOST_DECIMAL_DETAIL_INT128_HAS_MSVC_INT128

    // Compound XOR
    template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_INTEGER_CONCEPT>
    BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int128& operator^=(Integer rhs) noexcept;

    BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int128& operator^=(int128 rhs) noexcept;

    #ifdef BOOST_DECIMAL_DETAIL_INT128_HAS_MSVC_INT128

    template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_128BIT_INTEGER_CONCEPT>
    BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE inline int128& operator^=(Integer rhs) noexcept;

    #endif // BOOST_DECIMAL_DETAIL_INT128_HAS_MSVC_INT128

    // Compound Left Shift
    template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_INTEGER_CONCEPT>
    BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int128& operator<<=(Integer rhs) noexcept;

    BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int128& operator<<=(int128 rhs) noexcept;

    #ifdef BOOST_DECIMAL_DETAIL_INT128_HAS_MSVC_INT128

    template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_128BIT_INTEGER_CONCEPT>
    BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE inline int128& operator<<=(Integer rhs) noexcept;

    #endif // BOOST_DECIMAL_DETAIL_INT128_HAS_MSVC_INT128

    // Compound Right Shift
    template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_INTEGER_CONCEPT>
    BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int128& operator>>=(Integer rhs) noexcept;

    BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int128& operator>>=(int128 rhs) noexcept;

    #ifdef BOOST_DECIMAL_DETAIL_INT128_HAS_MSVC_INT128

    template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_128BIT_INTEGER_CONCEPT>
    BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE inline int128& operator>>=(Integer rhs) noexcept;

    #endif // BOOST_DECIMAL_DETAIL_INT128_HAS_MSVC_INT128

    // Prefix and postfix increment
    BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int128& operator++() noexcept;
    BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int128 operator++(int) noexcept;

    // Prefix and postfix decrment
    BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int128& operator--() noexcept;
    BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int128 operator--(int) noexcept;

    // Compound Addition
    template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_INTEGER_CONCEPT>
    BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int128& operator+=(Integer rhs) noexcept;

    BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int128& operator+=(int128 rhs) noexcept;

    #ifdef BOOST_DECIMAL_DETAIL_INT128_HAS_MSVC_INT128

    template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_128BIT_INTEGER_CONCEPT>
    BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE inline int128& operator+=(Integer rhs) noexcept;

    #endif // BOOST_DECIMAL_DETAIL_INT128_HAS_MSVC_INT128

    // Compound Subtraction
    template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_INTEGER_CONCEPT>
    BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int128& operator-=(Integer rhs) noexcept;

    BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int128& operator-=(int128 rhs) noexcept;

    #ifdef BOOST_DECIMAL_DETAIL_INT128_HAS_MSVC_INT128

    template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_128BIT_INTEGER_CONCEPT>
    BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE inline int128& operator-=(Integer rhs) noexcept;

    #endif // BOOST_DECIMAL_DETAIL_INT128_HAS_MSVC_INT128

    // Compound Multiplication
    template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_INTEGER_CONCEPT>
    BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int128& operator*=(Integer rhs) noexcept;

    BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int128& operator*=(int128 rhs) noexcept;

    #ifdef BOOST_DECIMAL_DETAIL_INT128_HAS_MSVC_INT128

    template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_128BIT_INTEGER_CONCEPT>
    BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE inline int128& operator*=(Integer rhs) noexcept;

    #endif // BOOST_DECIMAL_DETAIL_INT128_HAS_MSVC_INT128

    // Compound Division
    template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_INTEGER_CONCEPT>
    BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int128& operator/=(Integer rhs) noexcept;

    BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int128& operator/=(int128 rhs) noexcept;

    #ifdef BOOST_DECIMAL_DETAIL_INT128_HAS_MSVC_INT128

    template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_128BIT_INTEGER_CONCEPT>
    BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE inline int128& operator/=(Integer rhs) noexcept;

    #endif // BOOST_DECIMAL_DETAIL_INT128_HAS_MSVC_INT128

    // Compound Modulo
    template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_INTEGER_CONCEPT>
    BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int128& operator%=(Integer rhs) noexcept;

    BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int128& operator%=(int128 rhs) noexcept;

    #ifdef BOOST_DECIMAL_DETAIL_INT128_HAS_MSVC_INT128

    template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_128BIT_INTEGER_CONCEPT>
    BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE inline int128& operator%=(Integer rhs) noexcept;

    #endif // BOOST_DECIMAL_DETAIL_INT128_HAS_MSVC_INT128

    // Compound assignment with floating point types.
    // Matches the builtin: this value is converted to Float, the operation is applied in
    // floating point, and the result is converted back, truncating toward zero.
    template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_FLOATING_POINT_CONCEPT>
    BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int128& operator+=(Float rhs) noexcept;

    template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_FLOATING_POINT_CONCEPT>
    BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int128& operator-=(Float rhs) noexcept;

    template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_FLOATING_POINT_CONCEPT>
    BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int128& operator*=(Float rhs) noexcept;

    template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_FLOATING_POINT_CONCEPT>
    BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int128& operator/=(Float rhs) noexcept;

    // The builtin does not allow a floating point operand for these, so neither do we.
    // Without these the implicit floating point constructor would silently truncate rhs
    template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_FLOATING_POINT_CONCEPT>
    BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE int128& operator%=(Float rhs) = delete;

    template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_FLOATING_POINT_CONCEPT>
    BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE int128& operator&=(Float rhs) = delete;

    template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_FLOATING_POINT_CONCEPT>
    BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE int128& operator|=(Float rhs) = delete;

    template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_FLOATING_POINT_CONCEPT>
    BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE int128& operator^=(Float rhs) = delete;

    template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_FLOATING_POINT_CONCEPT>
    BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE int128& operator<<=(Float rhs) = delete;

    template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_FLOATING_POINT_CONCEPT>
    BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE int128& operator>>=(Float rhs) = delete;
};

namespace detail {

// Builds an int128 from the raw two's complement words
// Enables vectorization
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_FORCE_INLINE constexpr int128 from_bits(const std::uint64_t hi, const std::uint64_t lo) noexcept
{
    int128 result {};
    result.high = hi;
    result.low = lo;
    return result;
}

} // namespace detail

//=====================================
// Float Conversion Operators
//=====================================

// When the builtin 128-bit type exists we convert through it since the compiler
// runtime (__floattisf and friends) is correctly rounded. The portable fallback
// converts the unsigned magnitude and applies the sign; see detail/float_conversion.hpp
// for why the raw words can not be composed directly for negative values

BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int128::operator float() const noexcept
{
    #if defined(BOOST_DECIMAL_DETAIL_INT128_HAS_INT128) && !defined(BOOST_DECIMAL_DETAIL_INT128_HAS_GPU_SUPPORT)

    return static_cast<float>(static_cast<detail::builtin_i128>(*this));

    #else

    return detail::signed_words_to_float<float>(signed_high(), low);

    #endif
}

BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int128::operator double() const noexcept
{
    #if defined(BOOST_DECIMAL_DETAIL_INT128_HAS_INT128) && !defined(BOOST_DECIMAL_DETAIL_INT128_HAS_GPU_SUPPORT)

    return static_cast<double>(static_cast<detail::builtin_i128>(*this));

    #else

    return detail::signed_words_to_float<double>(signed_high(), low);

    #endif
}

#if !defined(BOOST_DECIMAL_DETAIL_INT128_HAS_GPU_SUPPORT)

constexpr int128::operator long double() const noexcept
{
    #if defined(BOOST_DECIMAL_DETAIL_INT128_HAS_INT128)

    return static_cast<long double>(static_cast<detail::builtin_i128>(*this));

    #else

    return detail::signed_words_to_float<long double>(signed_high(), low);

    #endif
}

#endif

//=====================================
// Float Construction
//=====================================

// Inverse of operator(Float).
// NaN -> 0;
// f >= 2^127 -> INT128_MAX;
// f < -2^127 -> INT128_MIN.
template <BOOST_DECIMAL_DETAIL_INT128_FLOATING_POINT_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int128::int128(Float f) noexcept
{
    constexpr Float two_32 {static_cast<Float>(UINT64_C(1) << 32)};
    constexpr Float two_64 {two_32 * two_32};
    constexpr Float two_127 {two_64 * static_cast<Float>(UINT64_C(1) << 63)};

    // NaN: leave default-initialized (zero). NaN compares false to everything,
    // so neither >= 0 nor <= 0 holds.
    if (!(f >= Float{0}) && !(f <= Float{0}))
    {
        return;
    }

    if (f >= two_127)
    {
        high = UINT64_C(0x7FFFFFFFFFFFFFFF);
        low = UINT64_MAX;
        return;
    }

    if (f <= -two_127)
    {
        high = UINT64_C(0x8000000000000000);
        low = UINT64_C(0);
        return;
    }

    const bool negative {f < Float{0}};
    const Float abs_f {negative ? -f : f};

    std::uint64_t h {detail::float_to_uint64(abs_f / two_64)};
    const Float remainder {abs_f - static_cast<Float>(h) * two_64};
    std::uint64_t l {detail::float_to_uint64(remainder)};

    if (negative)
    {
        // Two's complement negation of (h, l): new_l = -l (with wraparound),
        // new_h = ~h if a borrow occurred (l != 0), else ~h + 1.
        const bool low_was_zero {l == UINT64_C(0)};
        l = UINT64_C(0) - l;
        h = ~h + (low_was_zero ? UINT64_C(1) : UINT64_C(0));
    }

    high = h;
    low = l;
}

//=====================================
// Unary Operators
//=====================================

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int128 operator+(const int128 value) noexcept
{
    return value;
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int128 operator-(const int128 value) noexcept
{
    // Spelled with the constructor rather than from_bits: clang folds the low word
    // of the low == 0 arm away here, and loses that if the members are written.
    return (value.low == 0) ? int128{static_cast<std::int64_t>(UINT64_C(0) - value.high), 0} :
                              int128{static_cast<std::int64_t>(~value.high), ~value.low + 1};
}

//=====================================
// Equality Operators
//=====================================

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr bool operator==(const int128 lhs, const bool rhs) noexcept
{
    return lhs.high == 0 && lhs.low == static_cast<std::uint64_t>(rhs);
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr bool operator==(const bool lhs, const int128 rhs) noexcept
{
    return rhs.high == 0 && rhs.low == static_cast<std::uint64_t>(lhs);
}

#if defined(__clang__)
#  pragma clang diagnostic push
#  pragma clang diagnostic ignored "-Wsign-conversion"
#  pragma clang diagnostic ignored "-Wsign-compare"
#elif defined(__GNUC__)
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wsign-conversion"
#  pragma GCC diagnostic ignored "-Wsign-compare"
#endif

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr bool operator==(const int128 lhs, const int128 rhs) noexcept
{
    // x64 and ARM64 like the values in opposite directions

    #if defined(__aarch64__) || defined(_M_ARM64) || defined(__x86_64__) || defined(_M_X64) || defined(_M_IX86)

    return lhs.low == rhs.low && lhs.high == rhs.high;

    #else

    return lhs.high == rhs.high && lhs.low == rhs.low;

    #endif
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_SIGNED_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr bool operator==(const int128 lhs, const SignedInteger rhs) noexcept
{
    return lhs.high == (rhs < 0 ? ~UINT64_C(0) : UINT64_C(0)) && lhs.low == static_cast<std::uint64_t>(rhs);
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_SIGNED_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr bool operator==(const SignedInteger lhs, const int128 rhs) noexcept
{
    return rhs.high == (lhs < 0 ? ~UINT64_C(0) : UINT64_C(0)) && rhs.low == static_cast<std::uint64_t>(lhs);
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_UNSIGNED_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr bool operator==(const int128 lhs, const UnsignedInteger rhs) noexcept
{
    return lhs.high == 0 && lhs.low == static_cast<std::uint64_t>(rhs);
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_UNSIGNED_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr bool operator==(const UnsignedInteger lhs, const int128 rhs) noexcept
{
    return rhs.high == 0 && rhs.low == static_cast<std::uint64_t>(lhs);
}

#if defined(BOOST_DECIMAL_DETAIL_INT128_HAS_INT128) || defined(BOOST_DECIMAL_DETAIL_INT128_HAS_MSVC_INT128)

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_BUILTIN_CONSTEXPR bool operator==(const int128 lhs, const detail::builtin_i128 rhs) noexcept
{
    return lhs == static_cast<int128>(rhs);
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_BUILTIN_CONSTEXPR bool operator==(const detail::builtin_i128 lhs, const int128 rhs) noexcept
{
    return static_cast<int128>(lhs) == rhs;
}

#endif // BOOST_DECIMAL_DETAIL_INT128_HAS_INT128

//=====================================
// Inequality Operators
//=====================================

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr bool operator!=(const int128 lhs, const int128 rhs) noexcept
{
    // x64 and ARM64 like the values in opposite directions

    #if defined(__aarch64__) || defined(_M_ARM64) || defined(_M_X64) || defined(_M_IX86)

    return lhs.low != rhs.low || lhs.high != rhs.high;

    #elif defined(__x86_64__) && !defined(BOOST_DECIMAL_DETAIL_INT128_NO_CONSTEVAL_DETECTION) && defined(__GNUC__) && !defined(__clang__) && defined(BOOST_DECIMAL_DETAIL_INT128_HAS_INT128)

    if (BOOST_DECIMAL_DETAIL_INT128_IS_CONSTANT_EVALUATED(lhs))
    {
        return lhs.high != rhs.high || lhs.low != rhs.low;
    }
    else
    {
        detail::builtin_i128 builtin_lhs {};
        detail::builtin_i128 builtin_rhs {};

        std::memcpy(&builtin_lhs, &lhs, sizeof(builtin_lhs));
        std::memcpy(&builtin_rhs, &rhs, sizeof(builtin_rhs));

        return builtin_lhs != builtin_rhs;
    }

    #else

    return lhs.high != rhs.high || lhs.low != rhs.low;

    #endif
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr bool operator!=(const int128 lhs, const bool rhs) noexcept
{
    return lhs.high != 0 || lhs.low != static_cast<std::uint64_t>(rhs);
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr bool operator!=(const bool lhs, const int128 rhs) noexcept
{
    return rhs.high != 0 || rhs.low != static_cast<std::uint64_t>(lhs);
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_SIGNED_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr bool operator!=(const int128 lhs, const SignedInteger rhs) noexcept
{
    return lhs.high != (rhs < 0 ? ~UINT64_C(0) : UINT64_C(0)) || lhs.low != static_cast<std::uint64_t>(rhs);
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_SIGNED_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr bool operator!=(const SignedInteger lhs, const int128 rhs) noexcept
{
    return rhs.high != (lhs < 0 ? ~UINT64_C(0) : UINT64_C(0)) || rhs.low != static_cast<std::uint64_t>(lhs);
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_UNSIGNED_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr bool operator!=(const int128 lhs, const UnsignedInteger rhs) noexcept
{
    return lhs.high != 0 || lhs.low != static_cast<std::uint64_t>(rhs);
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_UNSIGNED_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr bool operator!=(const UnsignedInteger lhs, const int128 rhs) noexcept
{
    return rhs.high != 0 || rhs.low != static_cast<std::uint64_t>(lhs);
}

#if defined(BOOST_DECIMAL_DETAIL_INT128_HAS_INT128) || defined(BOOST_DECIMAL_DETAIL_INT128_HAS_MSVC_INT128)

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_BUILTIN_CONSTEXPR bool operator!=(const int128 lhs, const detail::builtin_i128 rhs) noexcept
{
    return lhs != static_cast<int128>(rhs);
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_BUILTIN_CONSTEXPR bool operator!=(const detail::builtin_i128 lhs, const int128 rhs) noexcept
{
    return static_cast<int128>(lhs) != rhs;
}

#endif // BOOST_DECIMAL_DETAIL_INT128_HAS_INT128

//=====================================
// Less than Operators
//=====================================

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr bool operator<(const int128 lhs, const int128 rhs) noexcept
{
    // On ARM macs only with the clang compiler is casting to __int128 uniformly better (and seemingly cost free)
    #if defined(__aarch64__) && defined(__APPLE__) && defined(__clang__) && defined(BOOST_DECIMAL_DETAIL_INT128_HAS_INT128)

    return static_cast<detail::builtin_i128>(lhs) < static_cast<detail::builtin_i128>(rhs);

    #elif defined(__x86_64__) && !defined(BOOST_DECIMAL_DETAIL_INT128_NO_CONSTEVAL_DETECTION) && defined(__GNUC__) && !defined(__clang__) && defined(BOOST_DECIMAL_DETAIL_INT128_HAS_INT128)

    if (BOOST_DECIMAL_DETAIL_INT128_IS_CONSTANT_EVALUATED(lhs))
    {
        return lhs.high == rhs.high ? lhs.low < rhs.low : lhs.signed_high() < rhs.signed_high();
    }
    else
    {
        detail::builtin_i128 builtin_lhs {};
        detail::builtin_i128 builtin_rhs {};

        std::memcpy(&builtin_lhs, &lhs, sizeof(builtin_lhs));
        std::memcpy(&builtin_rhs, &rhs, sizeof(builtin_rhs));

        return builtin_lhs < builtin_rhs;
    }

    #else

    return lhs.high == rhs.high ? lhs.low < rhs.low : lhs.signed_high() < rhs.signed_high();

    #endif
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_UNSIGNED_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr bool operator<(const int128 lhs, const UnsignedInteger rhs) noexcept
{
    return lhs.signed_high() < 0 || (lhs.high == 0 && lhs.low < static_cast<std::uint64_t>(rhs));
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_UNSIGNED_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr bool operator<(const UnsignedInteger lhs, const int128 rhs) noexcept
{
    return rhs.signed_high() > 0 || (rhs.high == 0 && static_cast<std::uint64_t>(lhs) < rhs.low);
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_SIGNED_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr bool operator<(const int128 lhs, const SignedInteger rhs) noexcept
{
    if (lhs.signed_high() < 0)
    {
        return rhs >= 0 ? true : lhs < static_cast<int128>(rhs);
    }

    if (lhs.signed_high() > 0 || rhs < 0)
    {
        return false;
    }

    return lhs.low < static_cast<std::uint64_t>(rhs);
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_SIGNED_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr bool operator<(const SignedInteger lhs, const int128 rhs) noexcept
{
    if (rhs.signed_high() < 0)
    {
        return lhs >= 0 ? false : static_cast<int128>(lhs) < rhs;
    }

    // rhs is positive
    if (rhs.signed_high() > 0 || lhs < 0)
    {
        return true;
    }

    return static_cast<std::uint64_t>(lhs) < rhs.low;
}

#if defined(BOOST_DECIMAL_DETAIL_INT128_HAS_INT128) || defined(BOOST_DECIMAL_DETAIL_INT128_HAS_MSVC_INT128)

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_BUILTIN_CONSTEXPR bool operator<(const int128 lhs, const detail::builtin_i128 rhs) noexcept
{
    return lhs < static_cast<int128>(rhs);
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_BUILTIN_CONSTEXPR bool operator<(const detail::builtin_i128 lhs, const int128 rhs) noexcept
{
    return static_cast<int128>(lhs) < rhs;
}

#endif // BOOST_DECIMAL_DETAIL_INT128_HAS_INT128

//=====================================
// Greater than Operators
//=====================================

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr bool operator>(const int128 lhs, const int128 rhs) noexcept
{
    // On ARM macs only with the clang compiler is casting to __int128 uniformly better (and seemingly cost free)
    #if defined(__aarch64__) && defined(__APPLE__) && defined(__clang__) && defined(BOOST_DECIMAL_DETAIL_INT128_HAS_INT128)

    return static_cast<detail::builtin_i128>(lhs) > static_cast<detail::builtin_i128>(rhs);

    #elif defined(__x86_64__) && !defined(BOOST_DECIMAL_DETAIL_INT128_NO_CONSTEVAL_DETECTION) && defined(__GNUC__) && !defined(__clang__) && defined(BOOST_DECIMAL_DETAIL_INT128_HAS_INT128)

    if (BOOST_DECIMAL_DETAIL_INT128_IS_CONSTANT_EVALUATED(lhs))
    {
        return lhs.high == rhs.high ? lhs.low > rhs.low : lhs.signed_high() > rhs.signed_high();
    }
    else
    {
        detail::builtin_i128 builtin_lhs {};
        detail::builtin_i128 builtin_rhs {};

        std::memcpy(&builtin_lhs, &lhs, sizeof(builtin_lhs));
        std::memcpy(&builtin_rhs, &rhs, sizeof(builtin_rhs));

        return builtin_lhs > builtin_rhs;
    }

    #else

    return lhs.high == rhs.high ? lhs.low > rhs.low : lhs.signed_high() > rhs.signed_high();

    #endif
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_SIGNED_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr bool operator>(const int128 lhs, const SignedInteger rhs) noexcept
{
    return !(lhs < rhs) && !(lhs == rhs);
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_SIGNED_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr bool operator>(const SignedInteger lhs, const int128 rhs) noexcept
{
    return !(lhs < rhs) && !(lhs == rhs);
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_UNSIGNED_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr bool operator>(const int128 lhs, const UnsignedInteger rhs) noexcept
{
    return lhs.signed_high() > 0 || (lhs.high == 0 && lhs.low > static_cast<std::uint64_t>(rhs));
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_UNSIGNED_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr bool operator>(const UnsignedInteger lhs, const int128 rhs) noexcept
{
    return rhs.signed_high() < 0 || (rhs.high == 0 && static_cast<std::uint64_t>(lhs) > rhs.low);
}

#if defined(BOOST_DECIMAL_DETAIL_INT128_HAS_INT128) || defined(BOOST_DECIMAL_DETAIL_INT128_HAS_MSVC_INT128)

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_BUILTIN_CONSTEXPR bool operator>(const int128 lhs, const detail::builtin_i128 rhs) noexcept
{
    return lhs > static_cast<int128>(rhs);
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_BUILTIN_CONSTEXPR bool operator>(const detail::builtin_i128 lhs, const int128 rhs) noexcept
{
    return static_cast<int128>(lhs) > rhs;
}

#endif // BOOST_DECIMAL_DETAIL_INT128_HAS_INT128

//=====================================
// Less Equal Operators
//=====================================

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr bool operator<=(const int128 lhs, const int128 rhs) noexcept
{
    // On ARM macs only with the clang compiler is casting to __int128 uniformly better (and seemingly cost free)
    #if defined(__aarch64__) && defined(__APPLE__) && defined(__clang__) && defined(BOOST_DECIMAL_DETAIL_INT128_HAS_INT128)

    return static_cast<detail::builtin_i128>(lhs) <= static_cast<detail::builtin_i128>(rhs);

    #elif defined(__x86_64__) && !defined(BOOST_DECIMAL_DETAIL_INT128_NO_CONSTEVAL_DETECTION) && defined(__GNUC__) && !defined(__clang__) && defined(BOOST_DECIMAL_DETAIL_INT128_HAS_INT128)

    if (BOOST_DECIMAL_DETAIL_INT128_IS_CONSTANT_EVALUATED(lhs))
    {
        return lhs.high == rhs.high ? lhs.low <= rhs.low : lhs.signed_high() <= rhs.signed_high();
    }
    else
    {
        detail::builtin_i128 builtin_lhs {};
        detail::builtin_i128 builtin_rhs {};

        std::memcpy(&builtin_lhs, &lhs, sizeof(builtin_lhs));
        std::memcpy(&builtin_rhs, &rhs, sizeof(builtin_rhs));

        return builtin_lhs <= builtin_rhs;
    }

    #else

    return lhs.high == rhs.high ? lhs.low <= rhs.low : lhs.signed_high() <= rhs.signed_high();

    #endif
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_SIGNED_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr bool operator<=(const int128 lhs, const SignedInteger rhs) noexcept
{
    return !(lhs > rhs);
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_SIGNED_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr bool operator<=(const SignedInteger lhs, const int128 rhs) noexcept
{
    return !(lhs > rhs);
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_UNSIGNED_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr bool operator<=(const int128 lhs, const UnsignedInteger rhs) noexcept
{
    return lhs.signed_high() < 0 || (lhs.high == 0 && lhs.low <= static_cast<std::uint64_t>(rhs));
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_UNSIGNED_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr bool operator<=(const UnsignedInteger lhs, const int128 rhs) noexcept
{
    return rhs.signed_high() > 0 || (rhs.high == 0 && static_cast<std::uint64_t>(lhs) <= rhs.low);
}

#if defined(BOOST_DECIMAL_DETAIL_INT128_HAS_INT128) || defined(BOOST_DECIMAL_DETAIL_INT128_HAS_MSVC_INT128)

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_BUILTIN_CONSTEXPR bool operator<=(const int128 lhs, const detail::builtin_i128 rhs) noexcept
{
    return lhs <= static_cast<int128>(rhs);
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_BUILTIN_CONSTEXPR bool operator<=(const detail::builtin_i128 lhs, const int128 rhs) noexcept
{
    return static_cast<int128>(lhs) <= rhs;
}

#endif // BOOST_DECIMAL_DETAIL_INT128_HAS_INT128

//=====================================
// Greater Equal Operators
//=====================================

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr bool operator>=(const int128 lhs, const int128 rhs) noexcept
{
    // On ARM macs only with the clang compiler is casting to __int128 uniformly better (and seemingly cost free)
    #if defined(__aarch64__) && defined(__APPLE__) && defined(__clang__) && defined(BOOST_DECIMAL_DETAIL_INT128_HAS_INT128)

    return static_cast<detail::builtin_i128>(lhs) >= static_cast<detail::builtin_i128>(rhs);

    #elif defined(__x86_64__) && !defined(BOOST_DECIMAL_DETAIL_INT128_NO_CONSTEVAL_DETECTION) && defined(__GNUC__) && !defined(__clang__) && defined(BOOST_DECIMAL_DETAIL_INT128_HAS_INT128)

    if (BOOST_DECIMAL_DETAIL_INT128_IS_CONSTANT_EVALUATED(lhs))
    {
        return lhs.high == rhs.high ? lhs.low >= rhs.low : lhs.signed_high() >= rhs.signed_high();
    }
    else
    {
        detail::builtin_i128 builtin_lhs {};
        detail::builtin_i128 builtin_rhs {};

        std::memcpy(&builtin_lhs, &lhs, sizeof(builtin_lhs));
        std::memcpy(&builtin_rhs, &rhs, sizeof(builtin_rhs));

        return builtin_lhs >= builtin_rhs;
    }

    #else

    return lhs.high == rhs.high ? lhs.low >= rhs.low : lhs.signed_high() >= rhs.signed_high();

    #endif
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_SIGNED_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr bool operator>=(const int128 lhs, const SignedInteger rhs) noexcept
{
    return !(lhs < rhs);
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_SIGNED_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr bool operator>=(const SignedInteger lhs, const int128 rhs) noexcept
{
    return !(lhs < rhs);
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_UNSIGNED_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr bool operator>=(const int128 lhs, const UnsignedInteger rhs) noexcept
{
    return lhs.signed_high() > 0 || (lhs.high == 0 && lhs.low >= static_cast<std::uint64_t>(rhs));
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_UNSIGNED_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr bool operator>=(const UnsignedInteger lhs, const int128 rhs) noexcept
{
    return rhs.signed_high() < 0 || (rhs.high == 0 && static_cast<std::uint64_t>(lhs) >= rhs.low);
}

#if defined(BOOST_DECIMAL_DETAIL_INT128_HAS_INT128) || defined(BOOST_DECIMAL_DETAIL_INT128_HAS_MSVC_INT128)

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_BUILTIN_CONSTEXPR bool operator>=(const int128 lhs, const detail::builtin_i128 rhs) noexcept
{
    return lhs >= static_cast<int128>(rhs);
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_BUILTIN_CONSTEXPR bool operator>=(const detail::builtin_i128 lhs, const int128 rhs) noexcept
{
    return static_cast<int128>(lhs) >= rhs;
}

#endif // BOOST_DECIMAL_DETAIL_INT128_HAS_INT128

//=====================================
// Spaceship Operator
//=====================================

#ifdef BOOST_DECIMAL_DETAIL_INT128_HAS_SPACESHIP_OPERATOR

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr std::strong_ordering operator<=>(const int128 lhs, const int128 rhs) noexcept
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
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr std::strong_ordering operator<=>(const int128 lhs, const SignedInteger rhs) noexcept
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
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr std::strong_ordering operator<=>(const SignedInteger lhs, const int128 rhs) noexcept
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
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr std::strong_ordering operator<=>(const int128 lhs, const UnsignedInteger rhs) noexcept
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
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr std::strong_ordering operator<=>(const UnsignedInteger lhs, const int128 rhs) noexcept
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

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int128 operator~(const int128 rhs) noexcept
{
    return detail::from_bits(~rhs.high, ~rhs.low);
}

//=====================================
// Or Operator
//=====================================

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int128 operator|(const int128 lhs, const int128 rhs) noexcept
{
    return detail::from_bits(lhs.high | rhs.high, lhs.low | rhs.low);
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_SIGNED_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int128 operator|(const int128 lhs, const SignedInteger rhs) noexcept
{
    return detail::from_bits(lhs.high | (rhs < 0 ? ~UINT64_C(0) : UINT64_C(0)), lhs.low | static_cast<std::uint64_t>(rhs));
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_SIGNED_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int128 operator|(const SignedInteger lhs, const int128 rhs) noexcept
{
    return detail::from_bits(rhs.high | (lhs < 0 ? ~UINT64_C(0) : UINT64_C(0)), static_cast<std::uint64_t>(lhs) | rhs.low);
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_UNSIGNED_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int128 operator|(const int128 lhs, const UnsignedInteger rhs) noexcept
{
    return detail::from_bits(lhs.high, lhs.low | static_cast<std::uint64_t>(rhs));
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_UNSIGNED_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int128 operator|(const UnsignedInteger lhs, const int128 rhs) noexcept
{
    return detail::from_bits(rhs.high, static_cast<std::uint64_t>(lhs) | rhs.low);
}

#if defined(BOOST_DECIMAL_DETAIL_INT128_HAS_INT128) || defined(BOOST_DECIMAL_DETAIL_INT128_HAS_MSVC_INT128)

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_BUILTIN_CONSTEXPR int128 operator|(const int128 lhs, const detail::builtin_i128 rhs) noexcept
{
    return lhs | static_cast<int128>(rhs);
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_BUILTIN_CONSTEXPR int128 operator|(const detail::builtin_i128 lhs, const int128 rhs) noexcept
{
    return static_cast<int128>(lhs) | rhs;
}


#endif // BOOST_DECIMAL_DETAIL_INT128_HAS_INT128

//=====================================
// Compound OR Operator
//=====================================

template <BOOST_DECIMAL_DETAIL_INT128_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int128& int128::operator|=(const Integer rhs) noexcept
{
    *this = static_cast<int128>(*this | rhs);
    return *this;
}

BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int128& int128::operator|=(const int128 rhs) noexcept
{
    *this = *this | rhs;
    return *this;
}

#ifdef BOOST_DECIMAL_DETAIL_INT128_HAS_MSVC_INT128

template <BOOST_DECIMAL_DETAIL_INT128_128BIT_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE inline int128& int128::operator|=(const Integer rhs) noexcept
{
    *this = static_cast<int128>(*this | rhs);
    return *this;
}

#endif // BOOST_DECIMAL_DETAIL_INT128_HAS_MSVC_INT128

//=====================================
// And Operator
//=====================================

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int128 operator&(const int128 lhs, const int128 rhs) noexcept
{
    return detail::from_bits(lhs.high & rhs.high, lhs.low & rhs.low);
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_SIGNED_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int128 operator&(const int128 lhs, const SignedInteger rhs) noexcept
{
    return detail::from_bits(lhs.high & (rhs < 0 ? ~UINT64_C(0) : UINT64_C(0)), lhs.low & static_cast<std::uint64_t>(rhs));
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_SIGNED_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int128 operator&(const SignedInteger lhs, const int128 rhs) noexcept
{
    return detail::from_bits(rhs.high & (lhs < 0 ? ~UINT64_C(0) : UINT64_C(0)), static_cast<std::uint64_t>(lhs) & rhs.low);
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_UNSIGNED_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int128 operator&(const int128 lhs, const UnsignedInteger rhs) noexcept
{
    return {0, lhs.low & static_cast<std::uint64_t>(rhs)};
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_UNSIGNED_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int128 operator&(const UnsignedInteger lhs, const int128 rhs) noexcept
{
    return {0, static_cast<std::uint64_t>(lhs) & rhs.low};
}

#if defined(BOOST_DECIMAL_DETAIL_INT128_HAS_INT128) || defined(BOOST_DECIMAL_DETAIL_INT128_HAS_MSVC_INT128)

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_BUILTIN_CONSTEXPR int128 operator&(const int128 lhs, const detail::builtin_i128 rhs) noexcept
{
    return lhs & static_cast<int128>(rhs);
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_BUILTIN_CONSTEXPR int128 operator&(const detail::builtin_i128 lhs, const int128 rhs) noexcept
{
    return static_cast<int128>(lhs) & rhs;
}


#endif // BOOST_DECIMAL_DETAIL_INT128_HAS_INT128

#ifdef BOOST_DECIMAL_DETAIL_INT128_HAS_MSVC_INT128

template <BOOST_DECIMAL_DETAIL_INT128_128BIT_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE inline int128& int128::operator&=(const Integer rhs) noexcept
{
    *this = static_cast<int128>(*this & rhs);
    return *this;
}

#endif // BOOST_DECIMAL_DETAIL_INT128_HAS_MSVC_INT128

//=====================================
// Compound And Operator
//=====================================

template <BOOST_DECIMAL_DETAIL_INT128_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int128& int128::operator&=(const Integer rhs) noexcept
{
    *this = static_cast<int128>(*this & rhs);
    return *this;
}

BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int128& int128::operator&=(const int128 rhs) noexcept
{
    *this = *this & rhs;
    return *this;
}

//=====================================
// XOR Operator
//=====================================

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int128 operator^(const int128 lhs, const int128 rhs) noexcept
{
    return detail::from_bits(lhs.high ^ rhs.high, lhs.low ^ rhs.low);
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_SIGNED_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int128 operator^(const int128 lhs, const SignedInteger rhs) noexcept
{
    return detail::from_bits(lhs.high ^ (rhs < 0 ? ~UINT64_C(0) : UINT64_C(0)), lhs.low ^ static_cast<std::uint64_t>(rhs));
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_SIGNED_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int128 operator^(const SignedInteger lhs, const int128 rhs) noexcept
{
    return detail::from_bits(rhs.high ^ (lhs < 0 ? ~UINT64_C(0) : UINT64_C(0)), static_cast<std::uint64_t>(lhs) ^ rhs.low);
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_UNSIGNED_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int128 operator^(const int128 lhs, const UnsignedInteger rhs) noexcept
{
    return detail::from_bits(lhs.high, lhs.low ^ static_cast<std::uint64_t>(rhs));
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_UNSIGNED_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int128 operator^(const UnsignedInteger lhs, const int128 rhs) noexcept
{
    return detail::from_bits(rhs.high, static_cast<std::uint64_t>(lhs) ^ rhs.low);
}

#if defined(BOOST_DECIMAL_DETAIL_INT128_HAS_INT128) || defined(BOOST_DECIMAL_DETAIL_INT128_HAS_MSVC_INT128)

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_BUILTIN_CONSTEXPR int128 operator^(const int128 lhs, const detail::builtin_i128 rhs) noexcept
{
    return lhs ^ static_cast<int128>(rhs);
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_BUILTIN_CONSTEXPR int128 operator^(const detail::builtin_i128 lhs, const int128 rhs) noexcept
{
    return static_cast<int128>(lhs) ^ rhs;
}


#endif // BOOST_DECIMAL_DETAIL_INT128_HAS_INT128

//=====================================
// Compound XOR Operator
//=====================================

template <BOOST_DECIMAL_DETAIL_INT128_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int128& int128::operator^=(Integer rhs) noexcept
{
    *this = static_cast<int128>(*this ^ rhs);
    return *this;
}

BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int128& int128::operator^=(int128 rhs) noexcept
{
    *this = *this ^ rhs;
    return *this;
}

#ifdef BOOST_DECIMAL_DETAIL_INT128_HAS_MSVC_INT128

template <BOOST_DECIMAL_DETAIL_INT128_128BIT_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE inline int128& int128::operator^=(const Integer rhs) noexcept
{
    *this = static_cast<int128>(*this ^ rhs);
    return *this;
}

#endif // BOOST_DECIMAL_DETAIL_INT128_HAS_MSVC_INT128

//=====================================
// Left Shift Operator
//=====================================

namespace detail {

template <typename Integer>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int128 default_ls_impl(const int128 lhs, const Integer rhs) noexcept
{
    static_assert(std::is_integral<Integer>::value, "Only builtin types allowed");

    // A shift by a negative amount or by an amount >= 128 (the operand width) is
    // undefined behavior, exactly as for the built-in shift operators. In a
    // constant expression the compiler diagnoses it; at runtime it is unspecified.
    if (rhs == 0)
    {
        return lhs;
    }

    if (rhs == 64)
    {
        return detail::from_bits(lhs.low, 0);
    }

    if (rhs > 64)
    {
        return detail::from_bits(lhs.low << (rhs - 64), 0);
    }

    // For shifts < 64
    std::uint64_t high_part = (lhs.high << rhs) |
                              (lhs.low >> (64 - rhs));

    return detail::from_bits(high_part, lhs.low << rhs);
}

template <typename Integer>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE int128 intrinsic_ls_impl(const int128 lhs, const Integer rhs) noexcept
{
    // A shift by a negative amount or by an amount >= 128 (the operand width) is
    // undefined behavior, exactly as for the built-in shift operators; delegate
    // straight to the native type so we produce identical results.
    #ifdef BOOST_DECIMAL_DETAIL_INT128_HAS_INT128

    // Left-shifting a negative builtin_i128 is UB pre-C++20
    #  if defined(__aarch64__)

    #if defined(__GNUC__) && __GNUC__ >= 8
    #  pragma GCC diagnostic push
    #  pragma GCC diagnostic ignored "-Wclass-memaccess"
    #endif

    builtin_u128 value;
    std::memcpy(&value, &lhs, sizeof(builtin_u128));
    const auto res {value << rhs};

    int128 return_value;
    std::memcpy(&return_value, &res, sizeof(int128));
    return return_value;

    #if defined(__GNUC__) && __GNUC__ >= 8
    #  pragma GCC diagnostic pop
    #endif

    #  else

    return int128{static_cast<builtin_u128>(lhs) << rhs};

    #  endif

    #elif defined(_M_AMD64) && !defined(__GNUC__)

    if (rhs >= 64)
    {
        return detail::from_bits(lhs.low << (rhs - 64), 0);
    }
    else
    {
        int128 res;
        res.high = __shiftleft128(lhs.low, lhs.high, static_cast<unsigned char>(rhs));
        res.low = lhs.low << rhs;

        return res;
    }

    #else

    if (BOOST_DECIMAL_DETAIL_INT128_UNLIKELY(rhs == 0))
    {
        return lhs;
    }
    if (rhs == 64)
    {
        return detail::from_bits(lhs.low, 0);
    }

    if (rhs > 64)
    {
        return detail::from_bits(lhs.low << (rhs - 64), 0);
    }

    // For shifts < 64
    const auto high_part = (lhs.high << rhs) |
                           (lhs.low >> (64 - rhs));

    return detail::from_bits(high_part, lhs.low << rhs);

    #endif
}

} // namespace detail

BOOST_DECIMAL_DETAIL_INT128_EXPORT template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int128 operator<<(const int128 lhs, const Integer rhs) noexcept
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

BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int128 operator<<(const int128 lhs, const int128 rhs) noexcept
{
    // Out-of-range counts (negative, >= 128, or with the high word set) are
    // undefined, matching the built-in operators; forward to the scalar overload.
    return lhs << rhs.low;
}

#if defined(BOOST_DECIMAL_DETAIL_INT128_HAS_INT128) || defined(BOOST_DECIMAL_DETAIL_INT128_HAS_MSVC_INT128)

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_BUILTIN_CONSTEXPR detail::builtin_u128 operator<<(const detail::builtin_u128 lhs, const int128 rhs) noexcept
{
    // Out-of-range counts are undefined, matching the built-in operators.
    return lhs << static_cast<detail::builtin_u128>(rhs.low);
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_BUILTIN_CONSTEXPR detail::builtin_i128 operator<<(const detail::builtin_i128 lhs, const int128 rhs) noexcept
{
    // Out-of-range counts are undefined, matching the built-in operators.
    return lhs << static_cast<detail::builtin_u128>(rhs.low);
}

#endif

// A shift takes its value and its result type from the left operand after integral promotion,
// and only the count from the right, exactly as the builtin does

BOOST_DECIMAL_DETAIL_INT128_EXPORT template <typename Integer, std::enable_if_t<detail::is_any_integer_v<Integer> && (sizeof(Integer) * 8 <= 64), bool> = true>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr detail::promoted_t<Integer> operator<<(const Integer lhs, const int128 rhs) noexcept
{
    // Out-of-range counts are undefined, matching the built-in operators.
    return static_cast<detail::promoted_t<Integer>>(lhs) << rhs.low;
}

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4804) // Unsafe use of type bool in operation
#endif // _MSC_VER

template <BOOST_DECIMAL_DETAIL_INT128_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int128& int128::operator<<=(const Integer rhs) noexcept
{
    *this = static_cast<int128>(*this << rhs);
    return *this;
}

BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int128& int128::operator<<=(const int128 rhs) noexcept
{
    *this = *this << rhs;
    return *this;
}

#ifdef BOOST_DECIMAL_DETAIL_INT128_HAS_MSVC_INT128

template <BOOST_DECIMAL_DETAIL_INT128_128BIT_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE inline int128& int128::operator<<=(const Integer rhs) noexcept
{
    *this = static_cast<int128>(*this << rhs);
    return *this;
}

#endif // BOOST_DECIMAL_DETAIL_INT128_HAS_MSVC_INT128

#ifdef _MSC_VER
#pragma warning(pop)
#endif // _MSC_VER

//=====================================
// Right Shift Operator
//=====================================

namespace detail {

template <typename Integer>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int128 default_rs_impl(const int128 lhs, const Integer rhs) noexcept
{
    // A shift by a negative amount or by an amount >= 128 (the operand width) is
    // undefined behavior, exactly as for the built-in shift operators. In a
    // constant expression the compiler diagnoses it; at runtime it is unspecified.
    if (rhs == 0)
    {
        return lhs;
    }

    if (rhs >= 64)
    {
        return detail::from_bits(lhs.signed_high() < 0 ? ~UINT64_C(0) : UINT64_C(0),
                                 static_cast<std::uint64_t>(lhs.signed_high() >> (rhs - 64)));
    }

    // For shifts < 64
    const auto high_to_low {lhs.high << (64 - rhs)};
    const auto low_shifted {lhs.low >> rhs};
    const auto low_part {high_to_low | low_shifted};

    return detail::from_bits(static_cast<std::uint64_t>(lhs.signed_high() >> rhs), low_part);
}

template <typename Integer>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE int128 intrinsic_rs_impl(const int128 lhs, const Integer rhs) noexcept
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

    builtin_i128 value;
    std::memcpy(&value, &lhs, sizeof(builtin_i128));
    const auto res {value >> rhs};

    int128 return_value;
    std::memcpy(&return_value, &res, sizeof(int128));
    return return_value;

    #if defined(__GNUC__) && __GNUC__ >= 8
    #  pragma GCC diagnostic pop
    #endif

    #  else

    return static_cast<builtin_i128>(lhs) >> rhs;

    #  endif

    #elif defined(_M_AMD64) && !defined(__GNUC__)

    if (rhs >= 64)
    {
        return detail::from_bits(lhs.signed_high() < 0 ? ~UINT64_C(0) : UINT64_C(0),
                                 static_cast<std::uint64_t>(lhs.signed_high() >> (rhs - 64)));
    }
    else
    {
        int128 res;
        res.low = __shiftright128(lhs.low, lhs.high, static_cast<unsigned char>(rhs));
        res.high = static_cast<std::uint64_t>(lhs.signed_high() >> rhs);

        return res;
    }

    #else

    if (BOOST_DECIMAL_DETAIL_INT128_UNLIKELY(rhs == 0))
    {
        return lhs;
    }

    if (rhs >= 64)
    {
        return detail::from_bits(lhs.signed_high() < 0 ? ~UINT64_C(0) : UINT64_C(0),
                                 static_cast<std::uint64_t>(lhs.signed_high() >> (rhs - 64)));
    }

    // For shifts < 64
    const auto high_to_low {lhs.high << (64 - rhs)};
    const auto low_shifted {lhs.low >> rhs};
    const auto low_part {high_to_low | low_shifted};

    return detail::from_bits(static_cast<std::uint64_t>(lhs.signed_high() >> rhs), low_part);

    #endif
}

} // namespace detail

BOOST_DECIMAL_DETAIL_INT128_EXPORT template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int128 operator>>(const int128 lhs, const Integer rhs) noexcept
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

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int128 operator>>(const int128 lhs, const int128 rhs) noexcept
{
    // Out-of-range counts (negative, >= 128, or with the high word set) are
    // undefined, matching the built-in operators; forward to the scalar overload.
    return lhs >> rhs.low;
}

#if defined(BOOST_DECIMAL_DETAIL_INT128_HAS_INT128) || defined(BOOST_DECIMAL_DETAIL_INT128_HAS_MSVC_INT128)

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_BUILTIN_CONSTEXPR detail::builtin_u128 operator>>(const detail::builtin_u128 lhs, const int128 rhs) noexcept
{
    // Out-of-range counts are undefined, matching the built-in operators.
    return lhs >> static_cast<detail::builtin_u128>(rhs.low);
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_BUILTIN_CONSTEXPR detail::builtin_i128 operator>>(const detail::builtin_i128 lhs, const int128 rhs) noexcept
{
    // Out-of-range counts are undefined, matching the built-in operators.
    return lhs >> static_cast<detail::builtin_u128>(rhs.low);
}

#endif

// A shift takes its value and its result type from the left operand after integral promotion,
// and only the count from the right, exactly as the builtin does

BOOST_DECIMAL_DETAIL_INT128_EXPORT template <typename Integer, std::enable_if_t<detail::is_any_integer_v<Integer> && (sizeof(Integer) * 8 <= 64), bool> = true>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr detail::promoted_t<Integer> operator>>(const Integer lhs, const int128 rhs) noexcept
{
    // Out-of-range counts are undefined, matching the built-in operators.
    return static_cast<detail::promoted_t<Integer>>(lhs) >> rhs.low;
}

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4804) // Unsafe use of type bool in operation
#endif // _MSC_VER

template <BOOST_DECIMAL_DETAIL_INT128_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int128& int128::operator>>=(const Integer rhs) noexcept
{
    *this = static_cast<int128>(*this >> rhs);
    return *this;
}

BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int128& int128::operator>>=(const int128 rhs) noexcept
{
    *this = *this >> rhs;
    return *this;
}

#ifdef BOOST_DECIMAL_DETAIL_INT128_HAS_MSVC_INT128

template <BOOST_DECIMAL_DETAIL_INT128_128BIT_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE inline int128& int128::operator>>=(const Integer rhs) noexcept
{
    *this = static_cast<int128>(*this >> rhs);
    return *this;
}

#endif // BOOST_DECIMAL_DETAIL_INT128_HAS_MSVC_INT128

#ifdef _MSC_VER
#pragma warning(pop)
#endif // _MSC_VER

//=====================================
// Increment Operators
//=====================================

BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int128& int128::operator++() noexcept
{
    if (++low == UINT64_C(0))
    {
        ++high;
    }

    return *this;
}

BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int128 int128::operator++(int) noexcept
{
    const auto temp {*this};
    ++(*this);
    return temp;
}

//=====================================
// Decrement Operators
//=====================================

BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int128& int128::operator--() noexcept
{
    if (low-- == UINT64_C(0))
    {
        --high;
    }

    return *this;
}

BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int128 int128::operator--(int) noexcept
{
    const auto temp {*this};
    --(*this);
    return temp;
}

//=====================================
// Addition Operators
//=====================================

namespace detail {

BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_FORCE_INLINE constexpr int128 library_add(const int128 lhs, const int128 rhs) noexcept
{
    const auto new_low {lhs.low + rhs.low};
    const auto new_high {lhs.high +
                                        rhs.high +
                                        static_cast<std::uint64_t>(new_low < lhs.low)};

    return detail::from_bits(new_high, new_low);
}

BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_FORCE_INLINE constexpr int128 default_add(const int128 lhs, const int128 rhs) noexcept
{
    #if (defined(__x86_64__) || (defined(__aarch64__) && !defined(__APPLE__))) && !defined(_WIN32) && defined(BOOST_DECIMAL_DETAIL_INT128_HAS_INT128)

    // Compute in the unsigned domain so that overflow wraps modulo 2^128
    return int128{static_cast<detail::builtin_u128>(lhs) + static_cast<detail::builtin_u128>(rhs)};

    #elif defined(BOOST_DECIMAL_DETAIL_INT128_HAS_BUILTIN_ADD_OVERFLOW)

    std::uint64_t result_low {};
    std::uint64_t result_high {};

    result_high = lhs.high + rhs.high + __builtin_add_overflow(lhs.low, rhs.low, &result_low);

    return detail::from_bits(result_high, result_low);

    #elif defined(_M_AMD64) && !defined(BOOST_DECIMAL_DETAIL_INT128_NO_CONSTEVAL_DETECTION)

    if (BOOST_DECIMAL_DETAIL_INT128_IS_CONSTANT_EVALUATED(lhs))
    {
        return library_add(lhs, rhs); // LCOV_EXCL_LINE
    }
    else
    {
        int128 result {};
        const auto carry {BOOST_DECIMAL_DETAIL_INT128_ADD_CARRY(0, lhs.low, rhs.low, &result.low)};
        BOOST_DECIMAL_DETAIL_INT128_ADD_CARRY(carry, lhs.high, rhs.high, &result.high);

        return result;
    }

    #else

    return library_add(lhs, rhs);

    #endif
}

template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_FORCE_INLINE constexpr int128 default_add(const int128 lhs, const Integer rhs) noexcept
{
    const auto new_low {lhs.low + rhs};
    const auto new_high {lhs.high + static_cast<std::uint64_t>(new_low < lhs.low)};

    return detail::from_bits(new_high, new_low);
}

BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_FORCE_INLINE constexpr int128 library_sub(const int128 lhs, const int128 rhs) noexcept
{
    const auto new_low {lhs.low - rhs.low};
    const auto new_high {lhs.high - rhs.high - static_cast<std::uint64_t>(lhs.low < rhs.low)};

    return detail::from_bits(new_high, new_low);
}

BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_FORCE_INLINE constexpr int128 default_sub(const int128 lhs, const int128 rhs) noexcept
{
    #if defined(BOOST_DECIMAL_DETAIL_INT128_HAS_BUILTIN_SUB_OVERFLOW) && (!defined(__aarch64__) || defined(__APPLE__) || !defined(BOOST_DECIMAL_DETAIL_INT128_HAS_INT128)) && !(defined(__CUDACC__) && defined(BOOST_DECIMAL_DETAIL_INT128_ENABLE_CUDA))

    // __builtin_sub_overflow is marked constexpr so we don't need if consteval handling
    std::uint64_t result_low {};
    const auto result_high {lhs.high - rhs.high - static_cast<std::uint64_t>(__builtin_sub_overflow(lhs.low, rhs.low, &result_low))};

    return detail::from_bits(result_high, result_low);

    #elif defined(__aarch64__) && !defined(__APPLE__) && defined(BOOST_DECIMAL_DETAIL_INT128_HAS_INT128)

    // Unsigned wrap for consistent two's-complement semantics
    return int128{static_cast<detail::builtin_u128>(lhs) - static_cast<detail::builtin_u128>(rhs)};

    #elif defined(_M_AMD64) && !defined(BOOST_DECIMAL_DETAIL_INT128_NO_CONSTEVAL_DETECTION)

    if (BOOST_DECIMAL_DETAIL_INT128_IS_CONSTANT_EVALUATED(lhs))
    {
        return library_sub(lhs, rhs); // LCOV_EXCL_LINE
    }
    else
    {
        int128 result {};
        const auto borrow {BOOST_DECIMAL_DETAIL_INT128_SUB_BORROW(0, lhs.low, rhs.low, &result.low)};
        BOOST_DECIMAL_DETAIL_INT128_SUB_BORROW(borrow, lhs.high, rhs.high, &result.high);

        return result;
    }

    #else

    return library_sub(lhs, rhs);

    #endif
}

template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_FORCE_INLINE constexpr int128 default_sub(const int128 lhs, const Integer rhs) noexcept
{
    const auto new_low {lhs.low - rhs};
    const auto new_high {lhs.high - static_cast<std::uint64_t>(new_low > lhs.low)};
    return detail::from_bits(new_high, new_low);
}

}

// On s390x with multiple different versions of GCC and language standards
// doing addition via subtraction is >10% faster in the benchmarks
#if defined(__s390__) || defined(__s390x__)

BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int128 operator+(const int128 lhs, const int128 rhs) noexcept
{
    return detail::default_sub(lhs, -rhs);
}

#else

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int128 operator+(const int128 lhs, const int128 rhs) noexcept
{
    return detail::default_add(lhs, rhs);
}

#endif

BOOST_DECIMAL_DETAIL_INT128_EXPORT template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_UNSIGNED_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int128 operator+(const int128 lhs, const UnsignedInteger rhs) noexcept
{
    return detail::default_add(lhs, rhs);
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_UNSIGNED_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int128 operator+(const UnsignedInteger lhs, const int128 rhs) noexcept
{
    return detail::default_add(rhs, lhs);
}

#ifdef _MSC_VER
#  pragma warning(push)
#  pragma warning(disable : 4146) // Unary minus applied to unsigned type
#endif

BOOST_DECIMAL_DETAIL_INT128_EXPORT template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_SIGNED_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int128 operator+(const int128 lhs, const SignedInteger rhs) noexcept
{
    // Negate in the unsigned domain so INT64_MIN does not overflow (UBSAN)
    return rhs < 0 ? detail::default_sub(lhs, -static_cast<std::uint64_t>(rhs)) :
                     detail::default_add(lhs, static_cast<std::uint64_t>(rhs));
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_SIGNED_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int128 operator+(const SignedInteger lhs, const int128 rhs) noexcept
{
    return lhs < 0 ? detail::default_sub(rhs, -static_cast<std::uint64_t>(lhs)) :
                     detail::default_add(rhs, static_cast<std::uint64_t>(lhs));
}

#ifdef _MSC_VER
#  pragma warning(pop)
#endif

#if defined(BOOST_DECIMAL_DETAIL_INT128_HAS_INT128) || defined(BOOST_DECIMAL_DETAIL_INT128_HAS_MSVC_INT128)


BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_BUILTIN_CONSTEXPR int128 operator+(const int128 lhs, const detail::builtin_i128 rhs) noexcept
{
    return detail::default_add(lhs, static_cast<int128>(rhs));
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_BUILTIN_CONSTEXPR int128 operator+(const detail::builtin_i128 lhs, const int128 rhs) noexcept
{
    return detail::default_add(rhs, static_cast<int128>(lhs));
}

#endif // BOOST_DECIMAL_DETAIL_INT128_HAS_INT128

template <BOOST_DECIMAL_DETAIL_INT128_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int128& int128::operator+=(const Integer rhs) noexcept
{
    *this = static_cast<int128>(*this + rhs);
    return *this;
}

BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int128& int128::operator+=(const int128 rhs) noexcept
{
    *this = *this + rhs;
    return *this;
}

#ifdef BOOST_DECIMAL_DETAIL_INT128_HAS_MSVC_INT128

template <BOOST_DECIMAL_DETAIL_INT128_128BIT_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE inline int128& int128::operator+=(const Integer rhs) noexcept
{
    *this = static_cast<int128>(*this + rhs);
    return *this;
}

#endif // BOOST_DECIMAL_DETAIL_INT128_HAS_MSVC_INT128

//=====================================
// Subtraction Operators
//=====================================

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int128 operator-(const int128 lhs, const int128 rhs) noexcept
{
    return detail::default_sub(lhs, rhs);
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_UNSIGNED_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int128 operator-(const int128 lhs, const UnsignedInteger rhs) noexcept
{
    return detail::default_sub(lhs, rhs);
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_UNSIGNED_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int128 operator-(const UnsignedInteger lhs, const int128 rhs) noexcept
{
    return detail::default_add(-rhs, lhs);
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_SIGNED_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int128 operator-(const int128 lhs, const SignedInteger rhs) noexcept
{
    return detail::default_sub(lhs, static_cast<int128>(rhs));
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_SIGNED_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int128 operator-(const SignedInteger lhs, const int128 rhs) noexcept
{
    return detail::default_sub(static_cast<int128>(lhs), rhs);
}

#if defined(BOOST_DECIMAL_DETAIL_INT128_HAS_INT128) || defined(BOOST_DECIMAL_DETAIL_INT128_HAS_MSVC_INT128)


BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_BUILTIN_CONSTEXPR int128 operator-(const int128 lhs, const detail::builtin_i128 rhs) noexcept
{
    return lhs - static_cast<int128>(rhs);
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_BUILTIN_CONSTEXPR int128 operator-(const detail::builtin_i128 lhs, const int128 rhs) noexcept
{
    return static_cast<int128>(lhs) - rhs;
}

#endif

template <BOOST_DECIMAL_DETAIL_INT128_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int128& int128::operator-=(const Integer rhs) noexcept
{
    *this = static_cast<int128>(*this - rhs);
    return *this;
}

BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int128& int128::operator-=(const int128 rhs) noexcept
{
    *this = *this - rhs;
    return *this;
}

#ifdef BOOST_DECIMAL_DETAIL_INT128_HAS_MSVC_INT128

template <BOOST_DECIMAL_DETAIL_INT128_128BIT_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE inline int128& int128::operator-=(const Integer rhs) noexcept
{
    *this = static_cast<int128>(*this - rhs);
    return *this;
}

#endif // BOOST_DECIMAL_DETAIL_INT128_HAS_MSVC_INT128

//=====================================
// Absolute Value function
//=====================================

// Branch-free two's complement absolute value: (x ^ mask) - mask, where mask is all
// ones for a negative value and zero otherwise. abs(min()) is min(), which matches the
// behavior of the builtin signed integer types.
BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int128 abs(const int128 value) noexcept
{
    const auto sign_word {static_cast<std::uint64_t>(value.signed_high() >> 63)};
    const auto mask {detail::from_bits(sign_word, sign_word)};

    return (value ^ mask) - mask;
}

//=====================================
// Multiplication Operators
//=====================================

namespace detail {

BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_FORCE_INLINE constexpr int128 default_mul(const int128 lhs, const std::uint64_t rhs) noexcept
{
    #if defined(BOOST_DECIMAL_DETAIL_INT128_HAS_INT128) && !defined(__s390__) && !defined(__s390x__)
    
    return int128{static_cast<detail::builtin_u128>(lhs) * static_cast<detail::builtin_u128>(rhs)};

    #else

    return low_word_mul<int128>(lhs, rhs);

    #endif
}

BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_FORCE_INLINE constexpr int128 default_mul(const int128 lhs, const std::uint32_t rhs) noexcept
{
    return default_mul(lhs, static_cast<std::uint64_t>(rhs));
}

#if defined(_M_AMD64) && !defined(__GNUC__)

BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_FORCE_INLINE int128 msvc_amd64_mul(const int128 lhs, const int128 rhs) noexcept
{
    int128 result {};
    result.low = _umul128(lhs.low, rhs.low, &result.high);
    result.high += lhs.low * rhs.high;
    result.high += lhs.high * rhs.low;

    return result;
}

#endif

BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_FORCE_INLINE constexpr int128 default_mul(const int128 lhs, const int128 rhs) noexcept
{
    #if ((defined(__aarch64__) && defined(__APPLE__)) || defined(__x86_64__) || defined(__PPC__) || defined(__powerpc__)) && defined(__GNUC__) && !defined(__clang__) && defined(BOOST_DECIMAL_DETAIL_INT128_HAS_INT128)

    #  if !defined(BOOST_DECIMAL_DETAIL_INT128_NO_CONSTEVAL_DETECTION)

    if (BOOST_DECIMAL_DETAIL_INT128_IS_CONSTANT_EVALUATED(lhs))
    {
        return low_word_mul<int128>(lhs, rhs);
    }
    else
    {
        #pragma GCC diagnostic push
        #pragma GCC diagnostic ignored "-Wclass-memaccess"

        detail::builtin_u128 new_lhs {};
        detail::builtin_u128 new_rhs {};

        std::memcpy(&new_lhs, &lhs, sizeof(detail::builtin_u128));
        std::memcpy(&new_rhs, &rhs, sizeof(detail::builtin_u128));

        const auto res {new_lhs * new_rhs};
        int128 library_res {};

        std::memcpy(&library_res, &res, sizeof(detail::builtin_u128));

        return library_res;

        #pragma GCC diagnostic pop
    }

    #  elif defined(BOOST_DECIMAL_DETAIL_INT128_HAS_INT128)

    // Unsigned wrap for consistent two's-complement semantics
    return int128{static_cast<detail::builtin_u128>(lhs) * static_cast<detail::builtin_u128>(rhs)};

    #  else

    return low_word_mul<int128>(lhs, rhs);

    #  endif

    #elif defined(__aarch64__) && defined(BOOST_DECIMAL_DETAIL_INT128_HAS_INT128)

    return int128{static_cast<detail::builtin_u128>(lhs) * static_cast<detail::builtin_u128>(rhs)};

    #elif defined(_M_AMD64) && !defined(__GNUC__) && !defined(BOOST_DECIMAL_DETAIL_INT128_NO_CONSTEVAL_DETECTION)

    if (BOOST_DECIMAL_DETAIL_INT128_IS_CONSTANT_EVALUATED(rhs))
    {
        return low_word_mul<int128>(lhs, rhs); // LCOV_EXCL_LINE
    }
    else
    {
        return msvc_amd64_mul(lhs, rhs);
    }

    #elif defined(BOOST_DECIMAL_DETAIL_INT128_HAS_INT128) && !defined(__s390__) && !defined(__s390x__)

    // Multiply in the unsigned domain to avoid signed-overflow UB, then reinterpret the bits.
    return int128{static_cast<detail::builtin_u128>(lhs) * static_cast<detail::builtin_u128>(rhs)};

    #else

    return low_word_mul<int128>(lhs, rhs);

    #endif
}

} // namespace detail

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int128 operator*(const int128 lhs, const int128 rhs) noexcept
{
    return detail::default_mul(lhs, rhs);
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_UNSIGNED_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int128 operator*(const int128 lhs, const UnsignedInteger rhs) noexcept
{
    using local_eval_type = detail::evaluation_type_t<UnsignedInteger>;
    return detail::default_mul(lhs, static_cast<local_eval_type>(rhs));
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_UNSIGNED_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int128 operator*(const UnsignedInteger lhs, const int128 rhs) noexcept
{
    using local_eval_type = detail::evaluation_type_t<UnsignedInteger>;
    return detail::default_mul(rhs, static_cast<local_eval_type>(lhs));
}

#ifdef _MSC_VER
#  pragma warning(push)
#  pragma warning(disable : 4146) // Unary minus applied to unsigned
#endif

BOOST_DECIMAL_DETAIL_INT128_EXPORT template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_SIGNED_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int128 operator*(const int128 lhs, const SignedInteger rhs) noexcept
{
    return rhs < 0 ? -detail::default_mul(lhs, -static_cast<std::uint64_t>(rhs)) :
                      detail::default_mul(lhs, static_cast<std::uint64_t>(rhs));
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_SIGNED_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int128 operator*(const SignedInteger lhs, const int128 rhs) noexcept
{
    return lhs < 0 ? -detail::default_mul(rhs, -static_cast<std::uint64_t>(lhs)) :
                      detail::default_mul(rhs, static_cast<std::uint64_t>(lhs));
}

#ifdef _MSC_VER
#  pragma warning(pop)
#endif

#if defined(BOOST_DECIMAL_DETAIL_INT128_HAS_INT128) || defined(BOOST_DECIMAL_DETAIL_INT128_HAS_MSVC_INT128)

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_BUILTIN_CONSTEXPR int128 operator*(const int128 lhs, const detail::builtin_i128 rhs) noexcept
{
    return detail::default_mul(lhs, static_cast<int128>(rhs));
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_BUILTIN_CONSTEXPR int128 operator*(const detail::builtin_i128 lhs, const int128 rhs) noexcept
{
    return detail::default_mul(rhs, static_cast<int128>(lhs));
}

#endif // BOOST_DECIMAL_DETAIL_INT128_HAS_INT128

template <BOOST_DECIMAL_DETAIL_INT128_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int128& int128::operator*=(const Integer rhs) noexcept
{
    *this = static_cast<int128>(*this * rhs);
    return *this;
}

BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int128& int128::operator*=(const int128 rhs) noexcept
{
    *this = *this * rhs;
    return *this;
}

#ifdef BOOST_DECIMAL_DETAIL_INT128_HAS_MSVC_INT128

template <BOOST_DECIMAL_DETAIL_INT128_128BIT_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE inline int128& int128::operator*=(const Integer rhs) noexcept
{
    *this = static_cast<int128>(*this * rhs);
    return *this;
}

#endif // BOOST_DECIMAL_DETAIL_INT128_HAS_MSVC_INT128

//=====================================
// Division Operator
//=====================================

#if defined(__clang__)
#  pragma clang diagnostic push
#  pragma clang diagnostic ignored "-Wassume"
#endif

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int128 operator/(const int128 lhs, const int128 rhs) noexcept
{
    if (BOOST_DECIMAL_DETAIL_INT128_UNLIKELY(rhs == 0))
    {
        // Division or remainder by zero is undefined behavior for the builtin __int128 types (a hardware trap). We match that: marking it unreachable keeps codegen branch-free and vectorizable.
        BOOST_DECIMAL_DETAIL_INT128_UNREACHABLE;
    }

    constexpr int128 min_val {INT64_MIN, 0};
    const auto abs_lhs {abs(lhs)};
    const auto abs_rhs {abs(rhs)};

    if (lhs != min_val && abs_lhs < abs_rhs)
    {
        return {0,0};
    }

    const auto negative_res {(lhs.signed_high() < 0) != (rhs.signed_high() < 0)};

    // Narrow fast path: when the divisor magnitude fits in 64 bits, divide the magnitudes with
    // the hardware-accelerated one_word_div and reapply the sign. This reuses the abs values
    // computed above and beats native signed division (the out-of-line __divti3) for this case.
    if (abs_rhs.high == 0)
    {
        int128 quotient {};

        if (abs_lhs.high == 0)
        {
            quotient = {0, abs_lhs.low / abs_rhs.low};
        }
        else
        {
            detail::one_word_div(abs_lhs, abs_rhs.low, quotient);
        }

        return negative_res ? -quotient : quotient;
    }

    #if defined(BOOST_DECIMAL_DETAIL_INT128_HAS_INT128)

    return static_cast<int128>(static_cast<detail::builtin_i128>(lhs) / static_cast<detail::builtin_i128>(rhs));

    #else

    const auto quotient {detail::knuth_div(abs_lhs, abs_rhs)};
    return negative_res ? -quotient : quotient;

    #endif
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_UNSIGNED_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int128 operator/(const int128 lhs, const UnsignedInteger rhs) noexcept
{
    using eval_type = detail::evaluation_type_t<UnsignedInteger>;

    if (BOOST_DECIMAL_DETAIL_INT128_UNLIKELY(rhs == 0))
    {
        // Division or remainder by zero is undefined behavior for the builtin __int128 types (a hardware trap). We match that: marking it unreachable keeps codegen branch-free and vectorizable.
        BOOST_DECIMAL_DETAIL_INT128_UNREACHABLE;
    }

    const auto abs_lhs {abs(lhs)};

    int128 quotient {};
    detail::one_word_div(abs_lhs, static_cast<eval_type>(rhs), quotient);
    return lhs < 0 ? -quotient : quotient;
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_UNSIGNED_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int128 operator/(const UnsignedInteger lhs, const int128 rhs) noexcept
{
    if (BOOST_DECIMAL_DETAIL_INT128_UNLIKELY(rhs == 0))
    {
        // Division or remainder by zero is undefined behavior for the builtin __int128 types (a hardware trap). We match that: marking it unreachable keeps codegen branch-free and vectorizable.
        BOOST_DECIMAL_DETAIL_INT128_UNREACHABLE;
    }

    if (rhs.high != 0 && rhs.high != ~UINT64_C(0))
    {
        return {0,0};
    }
    else
    {
        auto abs_rhs {abs(rhs)};
        // rhs == -2^64 has |rhs| greater than any 64-bit lhs, so the quotient is 0 (also avoids /0)
        if (abs_rhs.high != 0)
        {
            return {0, 0};
        }
        const auto res {static_cast<std::uint64_t>(lhs) / abs_rhs.low};
        const int128 result {0, res};
        return rhs < 0 ? -result : result;
    }
}

#ifdef _MSC_VER
#  pragma warning(push)
#  pragma warning(disable : 4146) // Unary minus applied to unsigned type
#endif

BOOST_DECIMAL_DETAIL_INT128_EXPORT template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_SIGNED_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int128 operator/(const int128 lhs, const SignedInteger rhs) noexcept
{
    using eval_type = detail::evaluation_type_t<SignedInteger>;

    if (BOOST_DECIMAL_DETAIL_INT128_UNLIKELY(rhs == 0))
    {
        // Division or remainder by zero is undefined behavior for the builtin __int128 types (a hardware trap). We match that: marking it unreachable keeps codegen branch-free and vectorizable.
        BOOST_DECIMAL_DETAIL_INT128_UNREACHABLE;
    }

    int128 quotient {};

    constexpr int128 min_val {INT64_MIN, 0};
    const auto negative_res {static_cast<bool>((lhs.signed_high() < 0) ^ (rhs < 0))};
    // Negate in the unsigned domain so INT64_MIN does not overflow (UBSAN)
    const auto abs_rhs {rhs < 0 ? -static_cast<eval_type>(rhs) : static_cast<eval_type>(rhs)};
    const auto abs_lhs {abs(lhs)};

    if (lhs != min_val && abs_lhs < abs_rhs)
    {
        return {0, 0};
    }

    detail::one_word_div(abs_lhs, abs_rhs, quotient);

    return negative_res ? -quotient : quotient;
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_SIGNED_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int128 operator/(const SignedInteger lhs, const int128 rhs) noexcept
{
    if (BOOST_DECIMAL_DETAIL_INT128_UNLIKELY(rhs == 0))
    {
        // Division or remainder by zero is undefined behavior for the builtin __int128 types (a hardware trap). We match that: marking it unreachable keeps codegen branch-free and vectorizable.
        BOOST_DECIMAL_DETAIL_INT128_UNREACHABLE;
    }

    if (rhs.high != 0 && rhs.high != ~UINT64_C(0))
    {
        return {0,0};
    }
    else
    {
        const auto negative_res {static_cast<bool>((rhs.signed_high() < 0) ^ (lhs < 0))};
        const auto abs_rhs {abs(rhs)};
        // rhs == -2^64 has |rhs| greater than any 64-bit lhs, so the quotient is 0 (also avoids /0)
        if (abs_rhs.high != 0)
        {
            return {0, 0};
        }
        // Negate in the unsigned domain so INT64_MIN does not overflow (UBSAN)
        const auto abs_lhs {lhs < 0 ? -static_cast<std::uint64_t>(lhs) : static_cast<std::uint64_t>(lhs)};
        const int128 res {0, abs_lhs / abs_rhs.low};

        return negative_res ? -res : res;
    }
}

#if defined(__clang__)
#  pragma clang diagnostic pop
#endif

#ifdef _MSC_VER
#  pragma warning(pop)
#endif

#if defined(BOOST_DECIMAL_DETAIL_INT128_HAS_INT128) || defined(BOOST_DECIMAL_DETAIL_INT128_HAS_MSVC_INT128)


BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_BUILTIN_CONSTEXPR int128 operator/(const int128 lhs, const detail::builtin_i128 rhs) noexcept
{
    return static_cast<int128>(static_cast<detail::builtin_i128>(lhs) / rhs);
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_BUILTIN_CONSTEXPR int128 operator/(const detail::builtin_i128 lhs, const int128 rhs) noexcept
{
    return static_cast<int128>(lhs / static_cast<detail::builtin_i128>(rhs));
}

#elif defined(BOOST_DECIMAL_DETAIL_INT128_HAS_MSVC_INT128)


BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE inline int128 operator/(const int128 lhs, const detail::builtin_i128 rhs) noexcept
{
    return lhs / static_cast<int128>(rhs);
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE inline int128 operator/(const detail::builtin_i128 lhs, const int128 rhs) noexcept
{
    return static_cast<int128>(lhs) / rhs;
}

#endif // BOOST_DECIMAL_DETAIL_INT128_HAS_INT128

template <BOOST_DECIMAL_DETAIL_INT128_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int128& int128::operator/=(const Integer rhs) noexcept
{
    *this = static_cast<int128>(*this / rhs);
    return *this;
}

BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int128& int128::operator/=(const int128 rhs) noexcept
{
    *this = *this / rhs;
    return *this;
}

#ifdef BOOST_DECIMAL_DETAIL_INT128_HAS_MSVC_INT128

template <BOOST_DECIMAL_DETAIL_INT128_128BIT_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE inline int128& int128::operator/=(const Integer rhs) noexcept
{
    *this = static_cast<int128>(*this / rhs);
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

BOOST_DECIMAL_DETAIL_INT128_EXPORT template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_UNSIGNED_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int128 operator%(int128 lhs, UnsignedInteger rhs) noexcept;

BOOST_DECIMAL_DETAIL_INT128_EXPORT template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_UNSIGNED_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int128 operator%(UnsignedInteger lhs, int128 rhs) noexcept;

BOOST_DECIMAL_DETAIL_INT128_EXPORT template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_SIGNED_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int128 operator%(int128 lhs, SignedInteger rhs) noexcept;

BOOST_DECIMAL_DETAIL_INT128_EXPORT template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_SIGNED_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int128 operator%(SignedInteger lhs, int128 rhs) noexcept;

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int128 operator%(int128 lhs, int128 rhs) noexcept;

template <BOOST_DECIMAL_DETAIL_INT128_UNSIGNED_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int128 operator%(const int128 lhs, const UnsignedInteger rhs) noexcept
{
    using eval_type = detail::evaluation_type_t<UnsignedInteger>;

    if (BOOST_DECIMAL_DETAIL_INT128_UNLIKELY(rhs == 0))
    {
        // Division or remainder by zero is undefined behavior for the builtin __int128 types (a hardware trap). We match that: marking it unreachable keeps codegen branch-free and vectorizable.
        BOOST_DECIMAL_DETAIL_INT128_UNREACHABLE;
    }

    int128 quotient {};
    int128 remainder {};

    const auto abs_lhs {abs(lhs)};

    detail::one_word_div(abs_lhs, static_cast<eval_type>(rhs), quotient, remainder);

    return lhs < 0 ? -remainder : remainder;
}

template <BOOST_DECIMAL_DETAIL_INT128_UNSIGNED_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int128 operator%(const UnsignedInteger lhs, const int128 rhs) noexcept
{
    using eval_type = detail::evaluation_type_t<UnsignedInteger>;

    if (BOOST_DECIMAL_DETAIL_INT128_UNLIKELY(rhs == 0))
    {
        // Division or remainder by zero is undefined behavior for the builtin __int128 types (a hardware trap). We match that: marking it unreachable keeps codegen branch-free and vectorizable.
        BOOST_DECIMAL_DETAIL_INT128_UNREACHABLE;
    }

    const auto abs_rhs {abs(rhs)};

    if (abs_rhs > lhs)
    {
        return lhs;
    }

    const int128 remainder {0, static_cast<eval_type>(lhs) % abs_rhs.low};

    return remainder;
}

template <BOOST_DECIMAL_DETAIL_INT128_SIGNED_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int128 operator%(const int128 lhs, const SignedInteger rhs) noexcept
{
    return lhs % static_cast<int128>(rhs);
}

template <BOOST_DECIMAL_DETAIL_INT128_SIGNED_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int128 operator%(const SignedInteger lhs, const int128 rhs) noexcept
{
    return static_cast<int128>(lhs) % rhs;
}

BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int128 operator%(const int128 lhs, const int128 rhs) noexcept
{
    if (rhs == 0)
    {
        // Division or remainder by zero is undefined behavior for the builtin __int128 types (a hardware trap). We match that: marking it unreachable keeps codegen branch-free and vectorizable.
        BOOST_DECIMAL_DETAIL_INT128_UNREACHABLE;
    }

    constexpr int128 min_val {INT64_MIN, 0};
    const auto abs_lhs {abs(lhs)};
    const auto abs_rhs {abs(rhs)};

    if (lhs != min_val && rhs != min_val && abs_rhs > abs_lhs)
    {
        return lhs;
    }

    const auto is_neg {lhs < 0};

    // Narrow fast path: when the divisor magnitude fits in 64 bits, take the remainder of the
    // magnitudes with the hardware-accelerated one_word_div and reapply the dividend's sign.
    if (abs_rhs.high == 0)
    {
        int128 remainder {};

        if (abs_lhs.high == 0)
        {
            remainder = int128{0, abs_lhs.low % abs_rhs.low};
        }
        else
        {
            int128 quotient {};
            detail::one_word_div(abs_lhs, abs_rhs.low, quotient, remainder);
        }

        return is_neg ? -remainder : remainder;
    }

    #if defined(BOOST_DECIMAL_DETAIL_INT128_HAS_INT128)

    return static_cast<int128>(static_cast<detail::builtin_i128>(lhs) % static_cast<detail::builtin_i128>(rhs));

    #else

    int128 remainder {};
    detail::knuth_div(abs_lhs, abs_rhs, remainder);
    return is_neg ? -remainder : remainder;

    #endif
}

#if defined(BOOST_DECIMAL_DETAIL_INT128_HAS_INT128) || defined(BOOST_DECIMAL_DETAIL_INT128_HAS_MSVC_INT128)

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_BUILTIN_CONSTEXPR int128 operator%(const int128 lhs, const detail::builtin_i128 rhs) noexcept
{
    return static_cast<detail::builtin_i128>(lhs) % rhs;
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_BUILTIN_CONSTEXPR int128 operator%(const detail::builtin_i128 lhs, const int128 rhs) noexcept
{
    return lhs % static_cast<detail::builtin_i128>(rhs);
}


#elif defined(BOOST_DECIMAL_DETAIL_INT128_HAS_MSVC_INT128)

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE inline int128 operator%(const int128 lhs, const detail::builtin_i128 rhs) noexcept
{
    return lhs % static_cast<int128>(rhs);
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE inline int128 operator%(const detail::builtin_i128 lhs, const int128 rhs) noexcept
{
    return static_cast<int128>(lhs) % rhs;
}


#endif // BOOST_DECIMAL_DETAIL_INT128_HAS_INT128

template <BOOST_DECIMAL_DETAIL_INT128_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int128& int128::operator%=(const Integer rhs) noexcept
{
    *this = static_cast<int128>(*this % rhs);
    return *this;
}

BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int128& int128::operator%=(const int128 rhs) noexcept
{
    *this = *this % rhs;
    return *this;
}

#ifdef BOOST_DECIMAL_DETAIL_INT128_HAS_MSVC_INT128

template <BOOST_DECIMAL_DETAIL_INT128_128BIT_INTEGER_CONCEPT>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE inline int128& int128::operator%=(const Integer rhs) noexcept
{
    *this = static_cast<int128>(*this % rhs);
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

#define BOOST_DECIMAL_DETAIL_INT128_DETAIL_I128_INTEGER_COMPOUND_OP(op, compound_op)                                         \
    BOOST_DECIMAL_DETAIL_INT128_EXPORT template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_INTEGER_CONCEPT>                                     \
    BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr Integer& operator compound_op(Integer& lhs, const int128 rhs) noexcept \
    {                                                                                                         \
        lhs = static_cast<Integer>(lhs op rhs);                                                               \
        return lhs;                                                                                           \
    }

BOOST_DECIMAL_DETAIL_INT128_DETAIL_I128_INTEGER_COMPOUND_OP(|, |=)
BOOST_DECIMAL_DETAIL_INT128_DETAIL_I128_INTEGER_COMPOUND_OP(&, &=)
BOOST_DECIMAL_DETAIL_INT128_DETAIL_I128_INTEGER_COMPOUND_OP(^, ^=)
BOOST_DECIMAL_DETAIL_INT128_DETAIL_I128_INTEGER_COMPOUND_OP(+, +=)
BOOST_DECIMAL_DETAIL_INT128_DETAIL_I128_INTEGER_COMPOUND_OP(-, -=)
BOOST_DECIMAL_DETAIL_INT128_DETAIL_I128_INTEGER_COMPOUND_OP(*, *=)
BOOST_DECIMAL_DETAIL_INT128_DETAIL_I128_INTEGER_COMPOUND_OP(/, /=)
BOOST_DECIMAL_DETAIL_INT128_DETAIL_I128_INTEGER_COMPOUND_OP(%, %=)

// The shifts take the value from the left operand alone, so only the count comes from rhs
BOOST_DECIMAL_DETAIL_INT128_DETAIL_I128_INTEGER_COMPOUND_OP(<<, <<=)
BOOST_DECIMAL_DETAIL_INT128_DETAIL_I128_INTEGER_COMPOUND_OP(>>, >>=)

#undef BOOST_DECIMAL_DETAIL_INT128_DETAIL_I128_INTEGER_COMPOUND_OP

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
#endif

#define BOOST_DECIMAL_DETAIL_INT128_DETAIL_I128_FLOAT_BINARY_OP(op, return_type)                                       \
    BOOST_DECIMAL_DETAIL_INT128_EXPORT template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_FLOATING_POINT_CONCEPT>                         \
    BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr return_type operator op(const int128 lhs, const Float rhs) noexcept \
    {                                                                                                    \
        return static_cast<Float>(lhs) op rhs;                                                           \
    }                                                                                                    \
                                                                                                         \
    BOOST_DECIMAL_DETAIL_INT128_EXPORT template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_FLOATING_POINT_CONCEPT>                         \
    BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr return_type operator op(const Float lhs, const int128 rhs) noexcept \
    {                                                                                                    \
        return lhs op static_cast<Float>(rhs);                                                           \
    }

BOOST_DECIMAL_DETAIL_INT128_DETAIL_I128_FLOAT_BINARY_OP(+, Float)
BOOST_DECIMAL_DETAIL_INT128_DETAIL_I128_FLOAT_BINARY_OP(-, Float)
BOOST_DECIMAL_DETAIL_INT128_DETAIL_I128_FLOAT_BINARY_OP(*, Float)
BOOST_DECIMAL_DETAIL_INT128_DETAIL_I128_FLOAT_BINARY_OP(/, Float)

BOOST_DECIMAL_DETAIL_INT128_DETAIL_I128_FLOAT_BINARY_OP(==, bool)
BOOST_DECIMAL_DETAIL_INT128_DETAIL_I128_FLOAT_BINARY_OP(!=, bool)
BOOST_DECIMAL_DETAIL_INT128_DETAIL_I128_FLOAT_BINARY_OP(<, bool)
BOOST_DECIMAL_DETAIL_INT128_DETAIL_I128_FLOAT_BINARY_OP(<=, bool)
BOOST_DECIMAL_DETAIL_INT128_DETAIL_I128_FLOAT_BINARY_OP(>, bool)
BOOST_DECIMAL_DETAIL_INT128_DETAIL_I128_FLOAT_BINARY_OP(>=, bool)

// Mixing an integer and a floating point type yields a partial ordering because of NaN
#ifdef BOOST_DECIMAL_DETAIL_INT128_HAS_SPACESHIP_OPERATOR

BOOST_DECIMAL_DETAIL_INT128_DETAIL_I128_FLOAT_BINARY_OP(<=>, std::partial_ordering)

#endif // BOOST_DECIMAL_DETAIL_INT128_HAS_SPACESHIP_OPERATOR

#undef BOOST_DECIMAL_DETAIL_INT128_DETAIL_I128_FLOAT_BINARY_OP

// Compound assignment converts the result back to int128, truncating toward zero.
// A result that is NaN or outside the range of the type saturates as the floating point
// constructor does, rather than being undefined as it is for the builtin

#define BOOST_DECIMAL_DETAIL_INT128_DETAIL_I128_FLOAT_COMPOUND_OP(op, compound_op)                                     \
    template <BOOST_DECIMAL_DETAIL_INT128_FLOATING_POINT_CONCEPT>                                                       \
    BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int128& int128::operator compound_op(const Float rhs) noexcept     \
    {                                                                                                    \
        *this = static_cast<int128>(static_cast<Float>(*this) op rhs);                                    \
        return *this;                                                                                     \
    }                                                                                                    \
                                                                                                         \
    BOOST_DECIMAL_DETAIL_INT128_EXPORT template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_FLOATING_POINT_CONCEPT>                         \
    BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr Float& operator compound_op(Float& lhs, const int128 rhs) noexcept  \
    {                                                                                                    \
        lhs compound_op static_cast<Float>(rhs);                                                          \
        return lhs;                                                                                       \
    }

BOOST_DECIMAL_DETAIL_INT128_DETAIL_I128_FLOAT_COMPOUND_OP(+, +=)
BOOST_DECIMAL_DETAIL_INT128_DETAIL_I128_FLOAT_COMPOUND_OP(-, -=)
BOOST_DECIMAL_DETAIL_INT128_DETAIL_I128_FLOAT_COMPOUND_OP(*, *=)
BOOST_DECIMAL_DETAIL_INT128_DETAIL_I128_FLOAT_COMPOUND_OP(/, /=)

#undef BOOST_DECIMAL_DETAIL_INT128_DETAIL_I128_FLOAT_COMPOUND_OP

#ifdef __GNUC__
#  pragma GCC diagnostic pop
#endif

// The builtin allows no floating point operand for the modulo, bitwise and shift operators.
// Deleting them keeps that a compile error here, rather than letting the implicit floating
// point constructor silently truncate the operand

#define BOOST_DECIMAL_DETAIL_INT128_DETAIL_I128_FLOAT_DELETED_OP(op)                                                   \
    BOOST_DECIMAL_DETAIL_INT128_EXPORT template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_FLOATING_POINT_CONCEPT>                         \
    BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE int128 operator op(int128 lhs, Float rhs) = delete;                          \
                                                                                                         \
    BOOST_DECIMAL_DETAIL_INT128_EXPORT template <BOOST_DECIMAL_DETAIL_INT128_DEFAULTED_FLOATING_POINT_CONCEPT>                         \
    BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE int128 operator op(Float lhs, int128 rhs) = delete;

BOOST_DECIMAL_DETAIL_INT128_DETAIL_I128_FLOAT_DELETED_OP(%)
BOOST_DECIMAL_DETAIL_INT128_DETAIL_I128_FLOAT_DELETED_OP(&)
BOOST_DECIMAL_DETAIL_INT128_DETAIL_I128_FLOAT_DELETED_OP(|)
BOOST_DECIMAL_DETAIL_INT128_DETAIL_I128_FLOAT_DELETED_OP(^)
BOOST_DECIMAL_DETAIL_INT128_DETAIL_I128_FLOAT_DELETED_OP(<<)
BOOST_DECIMAL_DETAIL_INT128_DETAIL_I128_FLOAT_DELETED_OP(>>)

#undef BOOST_DECIMAL_DETAIL_INT128_DETAIL_I128_FLOAT_DELETED_OP

namespace detail {

template <bool>
class numeric_limits_impl_i128
{
public:

        // Member constants
    static constexpr bool is_specialized = true;
    static constexpr bool is_signed = true;
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
    static constexpr int digits = 127;
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
    BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE static constexpr auto (min)        () -> boost::int128::int128 { return {INT64_MIN, 0}; }
    BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE static constexpr auto lowest       () -> boost::int128::int128 { return {INT64_MIN, 0}; }
    BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE static constexpr auto (max)        () -> boost::int128::int128 { return {INT64_MAX, UINT64_MAX}; }
    BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE static constexpr auto epsilon      () -> boost::int128::int128 { return {0, 0}; }
    BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE static constexpr auto round_error  () -> boost::int128::int128 { return {0, 0}; }
    BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE static constexpr auto infinity     () -> boost::int128::int128 { return {0, 0}; }
    BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE static constexpr auto quiet_NaN    () -> boost::int128::int128 { return {0, 0}; }
    BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE static constexpr auto signaling_NaN() -> boost::int128::int128 { return {0, 0}; }
    BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE static constexpr auto denorm_min   () -> boost::int128::int128 { return {0, 0}; }
};

#if !defined(__cpp_inline_variables) || __cpp_inline_variables < 201606L

template <bool b> constexpr bool numeric_limits_impl_i128<b>::is_specialized;
template <bool b> constexpr bool numeric_limits_impl_i128<b>::is_signed;
template <bool b> constexpr bool numeric_limits_impl_i128<b>::is_integer;
template <bool b> constexpr bool numeric_limits_impl_i128<b>::is_exact;
template <bool b> constexpr bool numeric_limits_impl_i128<b>::has_infinity;
template <bool b> constexpr bool numeric_limits_impl_i128<b>::has_quiet_NaN;
template <bool b> constexpr bool numeric_limits_impl_i128<b>::has_signaling_NaN;

// These members were deprecated in C++23; suppress the deprecation warning rather
// than dropping the definitions.
#if defined(__GNUC__) && __cplusplus > 202002L
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#elif defined(_MSC_VER)
#  pragma warning(push)
#  pragma warning(disable:4996)
#endif

template <bool b> constexpr std::float_denorm_style numeric_limits_impl_i128<b>::has_denorm;
template <bool b> constexpr bool numeric_limits_impl_i128<b>::has_denorm_loss;

#if defined(__GNUC__) && __cplusplus > 202002L
#  pragma GCC diagnostic pop
#elif defined(_MSC_VER)
#  pragma warning(pop)
#endif

template <bool b> constexpr std::float_round_style numeric_limits_impl_i128<b>::round_style;
template <bool b> constexpr bool numeric_limits_impl_i128<b>::is_iec559;
template <bool b> constexpr bool numeric_limits_impl_i128<b>::is_bounded;
template <bool b> constexpr bool numeric_limits_impl_i128<b>::is_modulo;
template <bool b> constexpr int numeric_limits_impl_i128<b>::digits;
template <bool b> constexpr int numeric_limits_impl_i128<b>::digits10;
template <bool b> constexpr int numeric_limits_impl_i128<b>::max_digits10;
template <bool b> constexpr int numeric_limits_impl_i128<b>::radix;
template <bool b> constexpr int numeric_limits_impl_i128<b>::min_exponent;
template <bool b> constexpr int numeric_limits_impl_i128<b>::min_exponent10;
template <bool b> constexpr int numeric_limits_impl_i128<b>::max_exponent;
template <bool b> constexpr int numeric_limits_impl_i128<b>::max_exponent10;
template <bool b> constexpr bool numeric_limits_impl_i128<b>::traps;
template <bool b> constexpr bool numeric_limits_impl_i128<b>::tinyness_before;

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
class numeric_limits<boost::int128::int128> :
    public boost::int128::detail::numeric_limits_impl_i128<true> {};

#ifdef __clang__
#  pragma clang diagnostic pop
#endif

} // namespace std

#endif // BOOST_DECIMAL_DETAIL_INT128_DETAIL_INT128_HPP
