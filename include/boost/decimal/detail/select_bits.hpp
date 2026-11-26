// Copyright 2026 Shen-Ta Hsieh
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_DECIMAL_DETAIL_SELECT_BITS_HPP
#define BOOST_DECIMAL_DETAIL_SELECT_BITS_HPP

#include <boost/decimal/detail/config.hpp>
#include "int128.hpp"

namespace boost {
namespace decimal {
namespace detail {

// x or y by a mask, because GCC can turn a ternary back into a jump that mispredicts.
// T is an integer type. A signed T needs two's complement, which each supported target has.
template <typename T>
BOOST_DECIMAL_FORCE_INLINE BOOST_DECIMAL_CUDA_CONSTEXPR auto select_bits(const bool take_x, const T x, const T y) noexcept -> T
{
    const auto mask {static_cast<T>(T{0} - static_cast<T>(take_x))};
    return static_cast<T>((x & mask) | (y & static_cast<T>(~mask)));
}

BOOST_DECIMAL_FORCE_INLINE BOOST_DECIMAL_CUDA_CONSTEXPR auto select_bits(const bool take_x, const int128::uint128_t& x, const int128::uint128_t& y) noexcept -> int128::uint128_t
{
    return {select_bits(take_x, x.high, y.high), select_bits(take_x, x.low, y.low)};
}

} // namespace detail
} // namespace decimal
} // namespace boost

#endif // BOOST_DECIMAL_DETAIL_SELECT_BITS_HPP
