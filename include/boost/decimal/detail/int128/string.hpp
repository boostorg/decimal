// Copyright 2025 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_DECIMAL_DETAIL_INT128_STRING_HPP
#define BOOST_DECIMAL_DETAIL_INT128_STRING_HPP

#include <boost/decimal/detail/int128/int128.hpp>
#include <boost/decimal/detail/int128/detail/mini_to_chars.hpp>

#ifndef BOOST_DECIMAL_DETAIL_INT128_BUILD_MODULE

#include <string>

#endif

namespace boost {
namespace int128 {

template <typename T>
auto to_string(const T& value) -> std::enable_if_t<(std::is_same<T, int128>::value || std::is_same<T, uint128>::value), std::string>
{
    char buffer[detail::mini_to_chars_buffer_size];
    const auto last {detail::mini_to_chars(buffer, value, 10, false)};
    return std::string{last, buffer + sizeof(buffer) - 1};
}

template <typename T>
auto to_wstring(const T& value) -> std::enable_if_t<(std::is_same<T, int128>::value || std::is_same<T, uint128>::value), std::wstring>
{
    char buffer[detail::mini_to_chars_buffer_size];
    const auto last {detail::mini_to_chars(buffer, value, 10, false)};
    return std::wstring{last, buffer + sizeof(buffer) - 1};
}

} // namespace int128
} // namespace boost

#endif // BOOST_DECIMAL_DETAIL_INT128_STRING_HPP
