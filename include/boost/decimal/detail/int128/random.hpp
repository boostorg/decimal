// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_DECIMAL_DETAIL_INT128_RANDOM_HPP
#define BOOST_DECIMAL_DETAIL_INT128_RANDOM_HPP

#include <boost/decimal/detail/int128/int128.hpp>

namespace boost {
namespace random {
namespace traits {

template <class T, bool intrinsic>
struct make_unsigned_imp;

template <>
struct make_unsigned_imp<int128::uint128, false>
{
    using type = int128::uint128;
};

template <>
struct make_unsigned_imp<int128::int128, false>
{
    using type = int128::uint128;
};

template <class T>
struct make_unsigned;

template <>
struct make_unsigned<int128::uint128>
{
    using type = int128::uint128;
};

template <>
struct make_unsigned<int128::int128>
{
    using type = int128::uint128;
};

template <class T, bool intrinsic>
struct make_unsigned_or_unbounded_imp;

template <>
struct make_unsigned_or_unbounded_imp<int128::uint128, false>
{
    using type = int128::uint128;
};

template <>
struct make_unsigned_or_unbounded_imp<int128::int128, false>
{
    using type = int128::uint128;
};

template <class T>
struct make_unsigned_or_unbounded;

template <>
struct make_unsigned_or_unbounded<int128::uint128>
{
    using type = int128::uint128;
};

template <>
struct make_unsigned_or_unbounded<int128::int128>
{
    using type = int128::uint128;
};

template <class T>
struct is_integral;

template <>
struct is_integral<int128::uint128> : std::true_type {};

template <>
struct is_integral<int128::int128> : std::true_type {};

template <class T>
struct is_signed;

template <>
struct is_signed<int128::uint128> : std::false_type {};

template <>
struct is_signed<int128::int128> : std::true_type {};

} // namespace traits
} // namespace random
} // namespace boost

#endif // BOOST_DECIMAL_DETAIL_INT128_RANDOM_HPP
