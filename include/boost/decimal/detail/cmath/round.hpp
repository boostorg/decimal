// Copyright 2023 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_DECIMAL_DETAIL_CMATH_ROUND_HPP
#define BOOST_DECIMAL_DETAIL_CMATH_ROUND_HPP

#include <boost/decimal/fwd.hpp>
#include <boost/decimal/detail/type_traits.hpp>
#include <boost/decimal/detail/concepts.hpp>
#include <boost/decimal/detail/cmath/impl/round_integral.hpp>
#include <boost/decimal/detail/config.hpp>
#include <boost/decimal/detail/cmath/abs.hpp>

#ifndef BOOST_DECIMAL_BUILD_MODULE
#include <type_traits>
#include <limits>
#include <cstdint>
#endif

namespace boost {
namespace decimal {

BOOST_DECIMAL_EXPORT template <typename T>
constexpr auto round(const T num) noexcept
    BOOST_DECIMAL_REQUIRES(detail::is_decimal_floating_point_v, T)
{
    return detail::round_integral_impl(num, rounding_mode::fe_dec_to_nearest_from_zero);
}

BOOST_DECIMAL_EXPORT template <typename T>
constexpr auto lround(const T num) noexcept
    BOOST_DECIMAL_REQUIRES_RETURN(detail::is_decimal_floating_point_v, T, long)
{
    return detail::round_integral_int_impl<T, long>(num, rounding_mode::fe_dec_to_nearest_from_zero);
}

BOOST_DECIMAL_EXPORT template <typename T>
constexpr auto llround(const T num) noexcept
    BOOST_DECIMAL_REQUIRES_RETURN(detail::is_decimal_floating_point_v, T, long long)
{
    return detail::round_integral_int_impl<T, long long>(num, rounding_mode::fe_dec_to_nearest_from_zero);
}

} //namespace decimal
} //namespace boost

#endif //BOOST_DECIMAL_DETAIL_CMATH_ROUND_HPP
