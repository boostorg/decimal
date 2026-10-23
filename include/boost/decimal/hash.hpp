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
#include <boost/decimal/detail/cmath/normalize.hpp>

#ifndef BOOST_DECIMAL_BUILD_MODULE
#include <functional>
#include <cstring>
#include <cstdint>
#endif

namespace boost {
namespace decimal {
namespace detail {

// std::hash requires that values which compare equal hash equally. Two things that do not
// take part in equality have to be removed first: the members of a cohort, which normalize
// collapses onto one representative, and the sign of a zero, since +0 == -0.
template <typename DecimalType>
auto hash_representative(const DecimalType& v) noexcept -> DecimalType
{
    // A non-finite value has no cohort and no significand to normalize, and normalizing one
    // would fold every infinity and NaN onto the same arbitrary finite representative
    #ifndef BOOST_DECIMAL_FAST_MATH
    if (!isfinite(v))
    {
        return v;
    }
    #endif

    const auto normalized_v {boost::decimal::normalize(v)};

    if (normalized_v == DecimalType{0, 0})
    {
        return DecimalType{0, 0};
    }

    return normalized_v;
}

// Narrows a 64-bit encoding down to a hash value
inline auto hash_encoding(const std::uint64_t bits) noexcept -> std::size_t
{
    #ifdef BOOST_DECIMAL_HAS_32_BIT_SIZE_T

    // std::hash<std::uint64_t> is a plain cast in libstdc++, so here it would keep only the
    // low word. Every member of the zero cohort, every infinity and every NaN has a zero
    // low word, so all of them would land in one bucket. Fold the high word in first.
    return std::hash<std::uint64_t>{}(bits ^ (bits >> 32U));

    #else

    return std::hash<std::uint64_t>{}(bits);

    #endif
}

} // namespace detail
} // namespace decimal
} // namespace boost

namespace std {

template <>
struct hash<boost::decimal::decimal32_t>
{
    // Since the underlying type is a std::uint32_t, we will rely on its hash function from the STL
    auto operator()(const boost::decimal::decimal32_t& v) const noexcept -> std::size_t
    {
        const auto normalized_v {boost::decimal::detail::hash_representative(v)};

        std::uint32_t bits;
        std::memcpy(&bits, &normalized_v, sizeof(std::uint32_t));

        return std::hash<std::uint32_t>{}(bits);
    }
};

template <>
struct hash<boost::decimal::decimal64_t>
{
    // Since the underlying type is a std::uint64_t, we will rely on its hash function from the STL
    auto operator()(const boost::decimal::decimal64_t& v) const noexcept -> std::size_t
    {
        const auto normalized_v {boost::decimal::detail::hash_representative(v)};

        std::uint64_t bits;
        std::memcpy(&bits, &normalized_v, sizeof(std::uint64_t));

        return boost::decimal::detail::hash_encoding(bits);
    }
};

#if defined(__GNUC__) && __GNUC__ >= 8
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wclass-memaccess"
#endif

template <>
struct hash<boost::decimal::decimal128_t>
{
    // Take the xor of the two words and hash that
    auto operator()(const boost::decimal::decimal128_t& v) const noexcept -> std::size_t
    {
        const auto normalized_v {boost::decimal::detail::hash_representative(v)};

        boost::int128::uint128_t bits;
        std::memcpy(&bits, &normalized_v, sizeof(boost::int128::uint128_t));

        return boost::decimal::detail::hash_encoding(bits.high ^ bits.low);
    }
};

#if defined(__GNUC__) && __GNUC__ >= 8
#  pragma GCC diagnostic pop
#endif

template <>
struct hash<boost::decimal::decimal_fast32_t>
{
    // Need to convert into decimal32_t then apply our memcpy
    auto operator()(const boost::decimal::decimal_fast32_t& v) const noexcept -> std::size_t
    {
        const auto v_32 {boost::decimal::detail::hash_representative(boost::decimal::decimal32_t{v})};
        std::uint32_t bits;
        std::memcpy(&bits, &v_32, sizeof(std::uint32_t));

        return std::hash<std::uint32_t>{}(bits);
    }
};

template <>
struct hash<boost::decimal::decimal_fast64_t>
{
    // Since the underlying type is a std::uint64_t, we will rely on its hash function from the STL
    // First we convert to a decimal64_t, so they will have the same hash value
    auto operator()(const boost::decimal::decimal_fast64_t& v) const noexcept -> std::size_t
    {
        const auto v_64 {boost::decimal::detail::hash_representative(boost::decimal::decimal64_t{v})};
        std::uint64_t bits;
        std::memcpy(&bits, &v_64, sizeof(std::uint64_t));

        return boost::decimal::detail::hash_encoding(bits);
    }
};

#if defined(__GNUC__) && __GNUC__ >= 8
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wclass-memaccess"
#endif

template <>
struct hash<boost::decimal::decimal_fast128_t>
{
    // Take the xor of the two words and hash that
    auto operator()(const boost::decimal::decimal_fast128_t& v) const noexcept -> std::size_t
    {
        const auto v_128 {boost::decimal::detail::hash_representative(boost::decimal::decimal128_t{v})};
        boost::int128::uint128_t bits;
        std::memcpy(&bits, &v_128, sizeof(boost::int128::uint128_t));

        return boost::decimal::detail::hash_encoding(bits.high ^ bits.low);
    }
};

#if defined(__GNUC__) && __GNUC__ >= 8
#  pragma GCC diagnostic pop
#endif

} // namespace std

#endif //BOOST_DECIMAL_HASH_HPP
