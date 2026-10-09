// Copyright 2023 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_DECIMAL_DETAIL_CMATH_RINT_HPP
#define BOOST_DECIMAL_DETAIL_CMATH_RINT_HPP

#include <boost/decimal/fwd.hpp>
#include <boost/decimal/detail/type_traits.hpp>
#include <boost/decimal/detail/concepts.hpp>
#include <boost/decimal/detail/cmath/impl/round_integral.hpp>
#include <boost/decimal/detail/config.hpp>

#ifndef BOOST_DECIMAL_BUILD_MODULE
#include <type_traits>
#include <limits>
#include <cmath>
#include <climits>
#endif

namespace boost {
namespace decimal {

namespace detail {

// The current rounding mode
template <BOOST_DECIMAL_DECIMAL_FLOATING_TYPE T>
constexpr auto current_rounding_mode(const T x) noexcept -> rounding_mode
{
    static_cast<void>(x);
    auto round {_boost_decimal_global_rounding_mode};
    #ifndef BOOST_DECIMAL_NO_CONSTEVAL_DETECTION
    if (!BOOST_DECIMAL_IS_CONSTANT_EVALUATED(x))
    {
        round = _boost_decimal_global_runtime_rounding_mode;
    }
    #endif

    return round;
}

} //namespace detail

// Rounds the number using the default rounding mode
BOOST_DECIMAL_EXPORT template <typename T>
constexpr auto rint(const T num) noexcept
    BOOST_DECIMAL_REQUIRES(detail::is_decimal_floating_point_v, T)
{
    return detail::round_integral_impl(num, detail::current_rounding_mode(num));
}

BOOST_DECIMAL_EXPORT template <typename T>
constexpr auto lrint(const T num) noexcept
    BOOST_DECIMAL_REQUIRES_RETURN(detail::is_decimal_floating_point_v, T, long)
{
    return detail::round_integral_int_impl<T, long>(num, detail::current_rounding_mode(num));
}

BOOST_DECIMAL_EXPORT template <typename T>
constexpr auto llrint(const T num) noexcept
    BOOST_DECIMAL_REQUIRES_RETURN(detail::is_decimal_floating_point_v, T, long long)
{
    return detail::round_integral_int_impl<T, long long>(num, detail::current_rounding_mode(num));
}

} //namespace decimal
} //namespace boost

#endif //BOOST_DECIMAL_DETAIL_CMATH_RINT_HPP
