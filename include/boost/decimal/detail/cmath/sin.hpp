// Copyright 2023 - 2024 Matt Borland
// Copyright 2023 - 2024 Christopher Kormanyos
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_DECIMAL_DETAIL_CMATH_SIN_HPP
#define BOOST_DECIMAL_DETAIL_CMATH_SIN_HPP

#include <boost/decimal/fwd.hpp>
#include <boost/decimal/detail/type_traits.hpp>
#include <boost/decimal/detail/concepts.hpp>
#include <boost/decimal/detail/config.hpp>
#include <boost/decimal/detail/promotion.hpp>
#include <boost/decimal/detail/cmath/impl/trig_reduce.hpp>
#include <boost/decimal/detail/cmath/impl/sin_impl.hpp>
#include <boost/decimal/detail/cmath/impl/cos_impl.hpp>

#ifndef BOOST_DECIMAL_BUILD_MODULE
#include <limits>
#include <type_traits>
#endif

namespace boost {
namespace decimal {

namespace detail {

template <typename T>
constexpr auto sin_impl(const T x) noexcept
    BOOST_DECIMAL_REQUIRES(detail::is_decimal_floating_point_v, T)
{
    #ifndef BOOST_DECIMAL_FAST_MATH
    if (isnan(x))
    {
        return x;
    }
    if (isinf(x))
    {
        return std::numeric_limits<T>::quiet_NaN();
    }
    #endif

    // x = n*pi/2 + r: sin(x) is sin(r), cos(r), -sin(r) or -cos(r) for n = 0 to 3.
    const auto r {trig::trig_prepare(x)};
    if (r.zero)
    {
        return x;
    }

    // The sign of sin(r) is r.neg, and cos(r) > 0.
    return (r.n & 1U) == 0U ? trig::sin_of<T>(r, r.neg != (r.n == 2U))
                            : trig::cos_of<T>(r, r.n == 3U);
}

} // namespace detail

BOOST_DECIMAL_EXPORT template <typename T>
constexpr auto sin(const T x) noexcept
    BOOST_DECIMAL_REQUIRES(detail::is_decimal_floating_point_v, T)
{
    using evaluation_type = detail::evaluation_type_t<T>;

    return static_cast<T>(detail::sin_impl(static_cast<evaluation_type>(x)));
}

} // namespace decimal
} // namespace boost

#endif // BOOST_DECIMAL_DETAIL_CMATH_SIN_HPP
