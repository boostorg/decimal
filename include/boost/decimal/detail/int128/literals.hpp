// Copyright 2022 Peter Dimov
// Copyright 2025 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_DECIMAL_DETAIL_INT128_LITERALS_HPP
#define BOOST_DECIMAL_DETAIL_INT128_LITERALS_HPP

#include <boost/decimal/detail/int128/int128.hpp>
#include <boost/decimal/detail/int128/detail/mini_from_chars.hpp>
#include <boost/decimal/detail/int128/detail/utilities.hpp>
#include <boost/decimal/detail/int128/detail/config.hpp>

namespace boost {
namespace int128 {
namespace literals {

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128 operator ""_u128(const char* str) noexcept
{
    return detail::parse_literal<uint128>(str, str + detail::strlen(str));
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128 operator ""_U128(const char* str) noexcept
{
    return detail::parse_literal<uint128>(str, str + detail::strlen(str));
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128 operator ""_u128(const char* str, std::size_t len) noexcept
{
    return detail::parse_literal<uint128>(str, str + len);
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr uint128 operator ""_U128(const char* str, std::size_t len) noexcept
{
    return detail::parse_literal<uint128>(str, str + len);
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int128 operator ""_i128(const char* str) noexcept
{
    return detail::parse_literal<int128>(str, str + detail::strlen(str));
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int128 operator ""_I128(const char* str) noexcept
{
    return detail::parse_literal<int128>(str, str + detail::strlen(str));
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int128 operator ""_i128(const char* str, std::size_t len) noexcept
{
    return detail::parse_literal<int128>(str, str + len);
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr int128 operator ""_I128(const char* str, std::size_t len) noexcept
{
    return detail::parse_literal<int128>(str, str + len);
}

} // namespace literals
} // namespace int128
} // namespace boost

#include <boost/decimal/detail/int128/detail/literal_macros.hpp>

#endif // BOOST_DECIMAL_DETAIL_INT128_LITERALS_HPP
