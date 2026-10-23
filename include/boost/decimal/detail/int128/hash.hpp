// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_DECIMAL_DETAIL_INT128_HASH_HPP
#define BOOST_DECIMAL_DETAIL_INT128_HASH_HPP

#include <boost/decimal/detail/int128/int128.hpp>

#ifndef BOOST_DECIMAL_DETAIL_INT128_BUILD_MODULE

#include <cstddef>
#include <cstdint>
#include <functional>

#endif

namespace boost {
namespace int128 {
namespace detail {

// The cast is only useless for 64-bit platforms
// Without we get an implicit conversion warning which is arguably worse
#if defined(__GNUC__) && !defined(__clang__)
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wuseless-cast"
#endif

// splitmix64 finalizer: mixes all 64 input bits into the result before any narrowing to size_t.
// This is required for correctness on platforms where size_t is 32 bits
inline std::size_t hash_finalize_64(std::uint64_t v) noexcept
{
    v ^= v >> 30;
    v *= UINT64_C(0xbf58476d1ce4e5b9);
    v ^= v >> 27;
    v *= UINT64_C(0x94d049bb133111eb);
    v ^= v >> 31;
    return static_cast<std::size_t>(v);
}

#if defined(__GNUC__) && !defined(__clang__)
#  pragma GCC diagnostic pop
#endif

} // namespace detail
} // namespace int128
} // namespace boost

namespace std {

template <>
struct hash<boost::int128::int128>
{
    auto operator()(const boost::int128::int128 v) const noexcept -> std::size_t
    {
        const std::size_t low_hash {boost::int128::detail::hash_finalize_64(v.low)};
        const std::size_t high_hash {boost::int128::detail::hash_finalize_64(v.high)};

        // boost::hash_combine style mixing of the two finalized halves
        return low_hash ^ (high_hash + static_cast<std::size_t>(0x9e3779b9) + (low_hash << 6) + (low_hash >> 2));
    }
};

template <>
struct hash<boost::int128::uint128>
{
    auto operator()(const boost::int128::uint128 v) const noexcept -> std::size_t
    {
        const std::size_t low_hash {boost::int128::detail::hash_finalize_64(v.low)};
        const std::size_t high_hash {boost::int128::detail::hash_finalize_64(v.high)};

        // boost::hash_combine style mixing of the two finalized halves
        return low_hash ^ (high_hash + static_cast<std::size_t>(0x9e3779b9) + (low_hash << 6) + (low_hash >> 2));
    }
};

} // namespace std

namespace boost {
namespace int128 {

inline std::size_t hash_value(const uint128 v) noexcept
{
    return std::hash<uint128>{}(v);
}

inline std::size_t hash_value(const int128 v) noexcept
{
    return std::hash<int128>{}(v);
}

} // namespace int128
} // namespace boost

#endif // BOOST_DECIMAL_DETAIL_INT128_HASH_HPP
