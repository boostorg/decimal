// Copyright 2023 - 2025 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//
// This is not a fully featured implementation of a 256-bit integer like int128::uint128_t is
// i256 only contains the minimum amount that we need to perform operations like decimal128_t add/sub

#ifndef BOOST_DECIMAL_DETAIL_I256_HPP
#define BOOST_DECIMAL_DETAIL_I256_HPP

#include <boost/decimal/detail/config.hpp>
#include <boost/decimal/detail/u256.hpp>
#include <boost/decimal/detail/int128.hpp>

namespace boost {
namespace decimal {
namespace detail {

constexpr u256 u256_add(const int128::uint128_t& lhs, const int128::uint128_t& rhs) noexcept
{
    u256 result;
    std::uint64_t carry {};

    auto sum {lhs.low + rhs.low};
    result[0] = sum;
    carry = (sum < lhs.low) ? 1 : 0;

    sum = lhs.high + rhs.high + carry;
    result[1] = sum;
    result[2] = static_cast<std::uint64_t>(sum < lhs.high || (sum == lhs.high && carry));

    return result;
}

} // namespace detail
} // namespace decimal
} // namespace boost

#endif // BOOST_DECIMAL_DETAIL_I256_HPP
