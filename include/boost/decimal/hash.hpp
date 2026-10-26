// Copyright 2023 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_DECIMAL_HASH_HPP
#define BOOST_DECIMAL_HASH_HPP

#include <boost/decimal/decimal32_t.hpp>
#include <boost/decimal/decimal64_t.hpp>
#include <boost/decimal/decimal128_t.hpp>
#include <boost/decimal/decimal_fast32_t.hpp>
#include <boost/decimal/decimal_fast64_t.hpp>
#include <boost/decimal/decimal_fast128_t.hpp>
#include <boost/decimal/detail/config.hpp>
#include <boost/decimal/detail/cmath/frexp10.hpp>

#ifndef BOOST_DECIMAL_BUILD_MODULE
#include <functional>
#include <cstdint>
#include <type_traits>
#endif

namespace boost {
namespace decimal {
namespace detail {

// Folds a significand into one key. A 32-bit key takes in the high word of a wider significand
// by a xor, so that both words contribute to the key.
template <typename Key>
constexpr auto hash_fold(const std::uint64_t sig) noexcept -> Key
{
    return static_cast<Key>(sizeof(Key) < sizeof(sig) ? sig ^ (sig >> 32U) : sig);
}

template <typename Key>
constexpr auto hash_fold(const int128::uint128_t sig) noexcept -> Key
{
    return hash_fold<Key>(sig.high ^ sig.low);
}

// Odd multipliers from the golden ratio, one for each width of key
constexpr auto hash_multiplier(std::uint32_t) noexcept -> std::uint32_t
{
    return UINT32_C(0x9E3779B9);
}

constexpr auto hash_multiplier(std::uint64_t) noexcept -> std::uint64_t
{
    return UINT64_C(0x9E3779B97F4A7C15);
}

// std::hash requires that values which compare equal hash equally. Two things that do not
// take part in equality have to be removed first: the members of a cohort, which frexp10
// collapses onto one significand and exponent, and the sign of a zero, since +0 == -0.
// A 32-bit significand or a 32-bit size_t gets a 32-bit key, which needs no 64-bit arithmetic.
template <typename DecimalType,
          typename Key = std::conditional_t<sizeof(typename DecimalType::significand_type) <= 4U || sizeof(std::size_t) <= 4U,
                                            std::uint32_t, std::uint64_t>>
auto hash_key(const DecimalType& v) noexcept -> Key
{
    // The payload bits of an infinity do not take part in equality either, and a NaN equals
    // nothing, so the infinities and the NaNs get one key for each sign
    #ifndef BOOST_DECIMAL_FAST_MATH
    if (!isfinite(v))
    {
        return static_cast<Key>((isnan(v) ? 3U : 1U) + static_cast<unsigned>(signbit(v)));
    }
    #endif

    int exp {};
    const auto sig {frexp10(v, &exp)};

    // frexp10 gives a zero significand and exponent for both signs of zero
    const auto tag {static_cast<Key>(static_cast<Key>(static_cast<std::uint32_t>(exp)) << 1U |
                                     static_cast<Key>(sig != 0U && signbit(v)))};

    // An odd multiplier spreads the exponent and the sign over all the bits. A sum, not a xor,
    // keeps runs of significands with one exponent in runs of keys.
    return static_cast<Key>(hash_fold<Key>(sig) + tag * hash_multiplier(Key{}));
}

// std::hash of the integer key lets a library that mixes integers, such as MSVC, mix the key too
template <typename DecimalType>
auto hash_decimal(const DecimalType& v) noexcept -> std::size_t
{
    return std::hash<decltype(hash_key(v))>{}(hash_key(v));
}

} // namespace detail
} // namespace decimal
} // namespace boost

namespace std {

template <>
struct hash<boost::decimal::decimal32_t>
{
    auto operator()(const boost::decimal::decimal32_t& v) const noexcept -> std::size_t
    {
        return boost::decimal::detail::hash_decimal(v);
    }
};

template <>
struct hash<boost::decimal::decimal64_t>
{
    auto operator()(const boost::decimal::decimal64_t& v) const noexcept -> std::size_t
    {
        return boost::decimal::detail::hash_decimal(v);
    }
};

template <>
struct hash<boost::decimal::decimal128_t>
{
    auto operator()(const boost::decimal::decimal128_t& v) const noexcept -> std::size_t
    {
        return boost::decimal::detail::hash_decimal(v);
    }
};

template <>
struct hash<boost::decimal::decimal_fast32_t>
{
    auto operator()(const boost::decimal::decimal_fast32_t& v) const noexcept -> std::size_t
    {
        return boost::decimal::detail::hash_decimal(v);
    }
};

template <>
struct hash<boost::decimal::decimal_fast64_t>
{
    auto operator()(const boost::decimal::decimal_fast64_t& v) const noexcept -> std::size_t
    {
        return boost::decimal::detail::hash_decimal(v);
    }
};

template <>
struct hash<boost::decimal::decimal_fast128_t>
{
    auto operator()(const boost::decimal::decimal_fast128_t& v) const noexcept -> std::size_t
    {
        return boost::decimal::detail::hash_decimal(v);
    }
};

} // namespace std

namespace boost {
namespace decimal {

// boost::hash, and with it Boost.Unordered, finds these by argument-dependent lookup
BOOST_DECIMAL_EXPORT inline auto hash_value(const decimal32_t& v) noexcept -> std::size_t
{
    return std::hash<decimal32_t>{}(v);
}

BOOST_DECIMAL_EXPORT inline auto hash_value(const decimal64_t& v) noexcept -> std::size_t
{
    return std::hash<decimal64_t>{}(v);
}

BOOST_DECIMAL_EXPORT inline auto hash_value(const decimal128_t& v) noexcept -> std::size_t
{
    return std::hash<decimal128_t>{}(v);
}

BOOST_DECIMAL_EXPORT inline auto hash_value(const decimal_fast32_t& v) noexcept -> std::size_t
{
    return std::hash<decimal_fast32_t>{}(v);
}

BOOST_DECIMAL_EXPORT inline auto hash_value(const decimal_fast64_t& v) noexcept -> std::size_t
{
    return std::hash<decimal_fast64_t>{}(v);
}

BOOST_DECIMAL_EXPORT inline auto hash_value(const decimal_fast128_t& v) noexcept -> std::size_t
{
    return std::hash<decimal_fast128_t>{}(v);
}

} // namespace decimal
} // namespace boost

#endif //BOOST_DECIMAL_HASH_HPP
