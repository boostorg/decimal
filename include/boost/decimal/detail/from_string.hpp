// Copyright 2025 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_DECIMAL_DETAIL_FROM_STRING_HPP
#define BOOST_DECIMAL_DETAIL_FROM_STRING_HPP

#include <boost/decimal/detail/config.hpp>
#include <boost/decimal/detail/concepts.hpp>

#ifndef BOOST_DECIMAL_BUILD_MODULE
#include <limits>
#endif // BOOST_DECIMAL_BUILD_MODULE

namespace boost {
namespace decimal {

// Forward decl needed for string constructor
BOOST_DECIMAL_EXPORT template <BOOST_DECIMAL_DECIMAL_FLOATING_TYPE TargetDecimalType>
constexpr auto from_chars(const char* first, const char* last, TargetDecimalType& value, chars_format fmt = chars_format::general) noexcept -> from_chars_result;

namespace detail {

template <BOOST_DECIMAL_DECIMAL_FLOATING_TYPE T>
constexpr auto construct_from_string(const char* str, const std::size_t len) -> T
{
    T v {};
    const auto r {from_chars(str, str + len, v)};
    if (!r)
    {
        v = std::numeric_limits<T>::quiet_NaN();
        BOOST_DECIMAL_THROW_EXCEPTION(std::runtime_error("Can not construct from invalid string"));
    }

    return v;
}

} // namespace detail
} // namespace decimal
} // namespace boost

#endif // BOOST_DECIMAL_DETAIL_FROM_STRING_HPP
