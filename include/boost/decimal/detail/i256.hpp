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

namespace impl {

// This impl works regardless of intrinsics or otherwise
constexpr u256 u256_add_impl(const int128::uint128_t& lhs, const int128::uint128_t& rhs) noexcept
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

} // namespace impl

#if !defined(BOOST_DECIMAL_NO_CONSTEVAL_DETECTION) && defined(BOOST_DECIMAL_ADD_CARRY)

constexpr u256 u256_add(const int128::uint128_t& lhs, const int128::uint128_t& rhs) noexcept
{
    if (BOOST_DECIMAL_IS_CONSTANT_EVALUATED(lhs))
    {
        return impl::u256_add_impl(lhs, rhs);
    }
    else
    {
        unsigned long long result[3] {};
        unsigned char carry {};
        carry = BOOST_DECIMAL_ADD_CARRY(carry, lhs.low, rhs.low, &result[0]);
        result[2] = static_cast<std::uint64_t>(BOOST_DECIMAL_ADD_CARRY(carry, lhs.high, rhs.high, &result[1]));

        return {UINT64_C(0), result[2], result[1], result[0]};
    }
}

#endif

} // namespace detail
} // namespace decimal
} // namespace boost

#endif // BOOST_DECIMAL_DETAIL_I256_HPP
