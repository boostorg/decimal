// Copyright 2024 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_DECIMAL_DETAIL_CMATH_ACOS_HPP
#define BOOST_DECIMAL_DETAIL_CMATH_ACOS_HPP

#include <boost/decimal/fwd.hpp>
#include <boost/decimal/detail/type_traits.hpp>
#include <boost/decimal/detail/concepts.hpp>
#include <boost/decimal/detail/promotion.hpp>
#include <boost/decimal/detail/config.hpp>
#include <boost/decimal/detail/cmath/fabs.hpp>
#include <boost/decimal/detail/cmath/sqrt.hpp>
#include <boost/decimal/detail/cmath/impl/asin_impl.hpp>
#include <boost/decimal/detail/cmath/impl/split_pi.hpp>


#ifndef BOOST_DECIMAL_BUILD_MODULE
#include <type_traits>
#include <cstdint>
#endif

namespace boost {
namespace decimal {

namespace detail {

template <typename T>
constexpr auto acos_impl(const T x) noexcept
    BOOST_DECIMAL_REQUIRES(detail::is_decimal_floating_point_v, T)
{
    #ifndef BOOST_DECIMAL_FAST_MATH
    if (isnan(x))
    {
        return x;
    }
    #endif

    const auto absx {fabs(static_cast<T>(x))};

    T result {};

    if (absx > 1)
    {
        result = std::numeric_limits<T>::quiet_NaN();
    }
    else if (absx <= T{5, -1})
    {
        result = detail::split_pi_values<T>(detail::split_pi_detail::half_pi_hi) -
                 (detail::asin_series(x) - detail::split_pi_values<T>(detail::split_pi_detail::half_pi_lo));
    }
    else
    {
        // acos(|x|) = 2 asin(s) and acos(-|x|) = pi - 2 asin(s), with asin(s) = s + lo.
        // Only the last operation rounds at the grid of the result, which keeps acos monotone.
        T s {};
        const T lo {detail::asin_half_angle(absx, s)};

        if (x > 0)
        {
            // s + s rounds for s in [0.05, 0.1); add the exact part it rounds off to the low part
            const T two_s {s + s};
            const T two_s_lo {s - (two_s - s)};
            result = two_s + ((lo + lo) + two_s_lo);
        }
        else
        {
            // s + s can round here too, but by less than 1/10 ulp of a result above 2, so it is not added
            result = detail::split_pi_values<T>(detail::split_pi_detail::pi_hi) -
                     ((s + s) + ((lo + lo) - detail::split_pi_values<T>(detail::split_pi_detail::pi_lo)));
        }
    }

    return result;
}

} // namespace detail

BOOST_DECIMAL_EXPORT template <typename T>
constexpr auto acos(const T x) noexcept
    BOOST_DECIMAL_REQUIRES(detail::is_decimal_floating_point_v, T)
{
    using evaluation_type = detail::evaluation_type_t<T>;

    return static_cast<T>(detail::acos_impl(static_cast<evaluation_type>(x)));
}

} //namespace decimal
} //namespace boost

#endif //BOOST_DECIMAL_DETAIL_CMATH_ACOS_HPP
