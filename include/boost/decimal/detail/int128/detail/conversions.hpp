// Copyright 2025 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_DECIMAL_DETAIL_INT128_CONVERSIONS_HPP
#define BOOST_DECIMAL_DETAIL_INT128_CONVERSIONS_HPP

#include <boost/decimal/detail/int128/detail/int128_imp.hpp>
#include <boost/decimal/detail/int128/detail/uint128_imp.hpp>

namespace boost {
namespace int128 {

namespace detail {

template <typename T>
struct valid_overload
{
    static constexpr bool value = std::is_same<T, uint128>::value || std::is_same<T, int128>::value;
};

template <typename T>
BOOST_DECIMAL_DETAIL_INT128_INLINE_CONSTEXPR bool is_valid_overload_v = valid_overload<T>::value;

} // namespace detail

#if BOOST_DECIMAL_DETAIL_INT128_ENDIAN_LITTLE_BYTE

BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int128::int128(const uint128& v) noexcept : low {v.low}, high {v.high} {}

BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128::uint128(const int128& v) noexcept : low {v.low}, high {v.high} {}

#else

BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int128::int128(const uint128& v) noexcept : high {v.high}, low {v.low} {}

BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128::uint128(const int128& v) noexcept : high {v.high}, low {v.low} {}

#endif // BOOST_DECIMAL_DETAIL_INT128_ENDIAN_LITTLE_BYTE

//=====================================
// Comparison Operators
//=====================================

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4127)
#endif

template <typename T, typename U, std::enable_if_t<detail::is_valid_overload_v<T> && detail::is_valid_overload_v<U> && !std::is_same<T, U>::value, bool> = true>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr bool operator==(const T lhs, const U rhs) noexcept
{
    return static_cast<uint128>(lhs) == static_cast<uint128>(rhs);
}

template <typename T, typename U, std::enable_if_t<detail::is_valid_overload_v<T> && detail::is_valid_overload_v<U> && !std::is_same<T, U>::value, bool> = true>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr bool operator!=(const T lhs, const U rhs) noexcept
{
    return static_cast<uint128>(lhs) != static_cast<uint128>(rhs);
}

template <typename T, typename U, std::enable_if_t<detail::is_valid_overload_v<T> && detail::is_valid_overload_v<U> && !std::is_same<T, U>::value, bool> = true>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr bool operator<(const T lhs, const U rhs) noexcept
{
    return static_cast<uint128>(lhs) < static_cast<uint128>(rhs);
}

template <typename T, typename U, std::enable_if_t<detail::is_valid_overload_v<T> && detail::is_valid_overload_v<U> && !std::is_same<T, U>::value, bool> = true>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr bool operator<=(const T lhs, const U rhs) noexcept
{
    return static_cast<uint128>(lhs) <= static_cast<uint128>(rhs);
}

template <typename T, typename U, std::enable_if_t<detail::is_valid_overload_v<T> && detail::is_valid_overload_v<U> && !std::is_same<T, U>::value, bool> = true>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr bool operator>(const T lhs, const U rhs) noexcept
{
    return static_cast<uint128>(lhs) > static_cast<uint128>(rhs);
}

template <typename T, typename U, std::enable_if_t<detail::is_valid_overload_v<T> && detail::is_valid_overload_v<U> && !std::is_same<T, U>::value, bool> = true>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr bool operator>=(const T lhs, const U rhs) noexcept
{
    return static_cast<uint128>(lhs) >= static_cast<uint128>(rhs);
}

#ifdef BOOST_DECIMAL_DETAIL_INT128_HAS_SPACESHIP_OPERATOR

template <typename T, typename U, std::enable_if_t<detail::is_valid_overload_v<T> && detail::is_valid_overload_v<U> && !std::is_same<T, U>::value, bool> = true>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr std::strong_ordering operator<=>(const T lhs, const U rhs) noexcept
{
    return static_cast<uint128>(lhs) <=> static_cast<uint128>(rhs);
}

#endif

//=====================================
// Arithmetic Operators
//=====================================

template <typename T, typename U, std::enable_if_t<detail::is_valid_overload_v<T> && detail::is_valid_overload_v<U> && !std::is_same<T, U>::value, bool> = true>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128 operator+(const T lhs, const U rhs) noexcept
{
    return static_cast<uint128>(lhs) + static_cast<uint128>(rhs);
}

template <typename T, typename U, std::enable_if_t<detail::is_valid_overload_v<T> && detail::is_valid_overload_v<U> && !std::is_same<T, U>::value, bool> = true>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128 operator-(const T lhs, const U rhs) noexcept
{
    return static_cast<uint128>(lhs) - static_cast<uint128>(rhs);
}

template <typename T, typename U, std::enable_if_t<detail::is_valid_overload_v<T> && detail::is_valid_overload_v<U> && !std::is_same<T, U>::value, bool> = true>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128 operator*(const T lhs, const U rhs) noexcept
{
    return static_cast<uint128>(lhs) * static_cast<uint128>(rhs);
}

template <typename T, typename U, std::enable_if_t<detail::is_valid_overload_v<T> && detail::is_valid_overload_v<U> && !std::is_same<T, U>::value, bool> = true>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128 operator/(const T lhs, const U rhs) noexcept
{
    return static_cast<uint128>(lhs) / static_cast<uint128>(rhs);
}

template <typename T, typename U, std::enable_if_t<detail::is_valid_overload_v<T> && detail::is_valid_overload_v<U> && !std::is_same<T, U>::value, bool> = true>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128 operator%(const T lhs, const U rhs) noexcept
{
    return static_cast<uint128>(lhs) % static_cast<uint128>(rhs);
}

//=====================================
// Cross-type Bitwise Operators
//=====================================

template <typename T, typename U, std::enable_if_t<detail::is_valid_overload_v<T> && detail::is_valid_overload_v<U> && !std::is_same<T, U>::value, bool> = true>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128 operator|(const T lhs, const U rhs) noexcept
{
    return static_cast<uint128>(lhs) | static_cast<uint128>(rhs);
}

template <typename T, typename U, std::enable_if_t<detail::is_valid_overload_v<T> && detail::is_valid_overload_v<U> && !std::is_same<T, U>::value, bool> = true>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128 operator&(const T lhs, const U rhs) noexcept
{
    return static_cast<uint128>(lhs) & static_cast<uint128>(rhs);
}

template <typename T, typename U, std::enable_if_t<detail::is_valid_overload_v<T> && detail::is_valid_overload_v<U> && !std::is_same<T, U>::value, bool> = true>
BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128 operator^(const T lhs, const U rhs) noexcept
{
    return static_cast<uint128>(lhs) ^ static_cast<uint128>(rhs);
}

//=====================================
// Cross-type Shift Operators
//=====================================

BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int128 operator<<(const int128 lhs, const uint128 rhs) noexcept
{
    return lhs << static_cast<int128>(rhs);
}

BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128 operator<<(const uint128 lhs, const int128 rhs) noexcept
{
    return lhs << static_cast<uint128>(rhs);
}

BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int128 operator>>(const int128 lhs, const uint128 rhs) noexcept
{
    return lhs >> static_cast<int128>(rhs);
}

BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128 operator>>(const uint128 lhs, const int128 rhs) noexcept
{
    return lhs >> static_cast<uint128>(rhs);
}

//=====================================
// int128 with builtin unsigned __int128 comparison operators
//
// These live here (not in int128_imp.hpp) 
// to avoid C++20 rewritten-candidate ambiguity on MSVC
//=====================================

#if defined(BOOST_DECIMAL_DETAIL_INT128_HAS_INT128) || defined(BOOST_DECIMAL_DETAIL_INT128_HAS_MSVC_INT128)

BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_BUILTIN_CONSTEXPR bool operator==(const int128 lhs, const detail::builtin_u128 rhs) noexcept
{
    return static_cast<uint128>(lhs) == rhs;
}

BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_BUILTIN_CONSTEXPR bool operator==(const detail::builtin_u128 lhs, const int128 rhs) noexcept
{
    return lhs == static_cast<uint128>(rhs);
}

BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_BUILTIN_CONSTEXPR bool operator!=(const int128 lhs, const detail::builtin_u128 rhs) noexcept
{
    return static_cast<uint128>(lhs) != rhs;
}

BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_BUILTIN_CONSTEXPR bool operator!=(const detail::builtin_u128 lhs, const int128 rhs) noexcept
{
    return lhs != static_cast<uint128>(rhs);
}

BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_BUILTIN_CONSTEXPR bool operator<(const int128 lhs, const detail::builtin_u128 rhs) noexcept
{
    return static_cast<uint128>(lhs) < rhs;
}

BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_BUILTIN_CONSTEXPR bool operator<(const detail::builtin_u128 lhs, const int128 rhs) noexcept
{
    return lhs < static_cast<uint128>(rhs);
}

BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_BUILTIN_CONSTEXPR bool operator<=(const int128 lhs, const detail::builtin_u128 rhs) noexcept
{
    return static_cast<uint128>(lhs) <= rhs;
}

BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_BUILTIN_CONSTEXPR bool operator<=(const detail::builtin_u128 lhs, const int128 rhs) noexcept
{
    return lhs <= static_cast<uint128>(rhs);
}

BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_BUILTIN_CONSTEXPR bool operator>(const int128 lhs, const detail::builtin_u128 rhs) noexcept
{
    return static_cast<uint128>(lhs) > rhs;
}

BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_BUILTIN_CONSTEXPR bool operator>(const detail::builtin_u128 lhs, const int128 rhs) noexcept
{
    return lhs > static_cast<uint128>(rhs);
}

BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_BUILTIN_CONSTEXPR bool operator>=(const int128 lhs, const detail::builtin_u128 rhs) noexcept
{
    return static_cast<uint128>(lhs) >= rhs;
}

BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_BUILTIN_CONSTEXPR bool operator>=(const detail::builtin_u128 lhs, const int128 rhs) noexcept
{
    return lhs >= static_cast<uint128>(rhs);
}

#endif // BOOST_DECIMAL_DETAIL_INT128_HAS_INT128

//=====================================
// int128 with builtin unsigned __int128 binary operators
//=====================================

#if defined(BOOST_DECIMAL_DETAIL_INT128_HAS_INT128) || defined(BOOST_DECIMAL_DETAIL_INT128_HAS_MSVC_INT128)

BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_BUILTIN_CONSTEXPR uint128 operator|(const int128 lhs, const detail::builtin_u128 rhs) noexcept
{
    return static_cast<uint128>(lhs) | rhs;
}

BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_BUILTIN_CONSTEXPR uint128 operator|(const detail::builtin_u128 lhs, const int128 rhs) noexcept
{
    return lhs | static_cast<uint128>(rhs);
}

BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_BUILTIN_CONSTEXPR uint128 operator&(const int128 lhs, const detail::builtin_u128 rhs) noexcept
{
    return static_cast<uint128>(lhs) & rhs;
}

BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_BUILTIN_CONSTEXPR uint128 operator&(const detail::builtin_u128 lhs, const int128 rhs) noexcept
{
    return lhs & static_cast<uint128>(rhs);
}

BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_BUILTIN_CONSTEXPR uint128 operator^(const int128 lhs, const detail::builtin_u128 rhs) noexcept
{
    return static_cast<uint128>(lhs) ^ rhs;
}

BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_BUILTIN_CONSTEXPR uint128 operator^(const detail::builtin_u128 lhs, const int128 rhs) noexcept
{
    return lhs ^ static_cast<uint128>(rhs);
}

BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_BUILTIN_CONSTEXPR uint128 operator+(const int128 lhs, const detail::builtin_u128 rhs) noexcept
{
    return static_cast<uint128>(lhs) + rhs;
}

BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_BUILTIN_CONSTEXPR uint128 operator+(const detail::builtin_u128 lhs, const int128 rhs) noexcept
{
    return lhs + static_cast<uint128>(rhs);
}

BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_BUILTIN_CONSTEXPR uint128 operator-(const int128 lhs, const detail::builtin_u128 rhs) noexcept
{
    return static_cast<uint128>(lhs) - rhs;
}

BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_BUILTIN_CONSTEXPR uint128 operator-(const detail::builtin_u128 lhs, const int128 rhs) noexcept
{
    return lhs - static_cast<uint128>(rhs);
}

BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_BUILTIN_CONSTEXPR uint128 operator*(const int128 lhs, const detail::builtin_u128 rhs) noexcept
{
    return static_cast<uint128>(lhs) * rhs;
}

BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_BUILTIN_CONSTEXPR uint128 operator*(const detail::builtin_u128 lhs, const int128 rhs) noexcept
{
    return lhs * static_cast<uint128>(rhs);
}

BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_BUILTIN_CONSTEXPR uint128 operator/(const int128 lhs, const detail::builtin_u128 rhs) noexcept
{
    return static_cast<uint128>(lhs) / rhs;
}

BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_BUILTIN_CONSTEXPR uint128 operator/(const detail::builtin_u128 lhs, const int128 rhs) noexcept
{
    return lhs / static_cast<uint128>(rhs);
}

BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_BUILTIN_CONSTEXPR uint128 operator%(const int128 lhs, const detail::builtin_u128 rhs) noexcept
{
    return static_cast<uint128>(lhs) % rhs;
}

BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE BOOST_DECIMAL_DETAIL_INT128_BUILTIN_CONSTEXPR uint128 operator%(const detail::builtin_u128 lhs, const int128 rhs) noexcept
{
    return lhs % static_cast<uint128>(rhs);
}

#endif // BOOST_DECIMAL_DETAIL_INT128_HAS_INT128

#ifdef _MSC_VER
#pragma warning(pop)
#endif

} // namespace int128
} // namespace boost

#endif // BOOST_DECIMAL_DETAIL_INT128_CONVERSIONS_HPP
