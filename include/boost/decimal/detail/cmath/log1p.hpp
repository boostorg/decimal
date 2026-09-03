// Copyright 2023 Matt Borland
// Copyright 2023 Christopher Kormanyos
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_DECIMAL_DETAIL_CMATH_LOG1P_HPP
#define BOOST_DECIMAL_DETAIL_CMATH_LOG1P_HPP

#include <boost/decimal/fwd.hpp> // NOLINT(llvm-include-order)
#include <boost/decimal/detail/cmath/abs.hpp>
#include <boost/decimal/detail/cmath/fma.hpp>
#include <boost/decimal/detail/cmath/impl/log1p_impl.hpp>
#include <boost/decimal/detail/concepts.hpp>
#include <boost/decimal/detail/config.hpp>
#include <boost/decimal/detail/type_traits.hpp>
#include <boost/decimal/numbers.hpp>

#ifndef BOOST_DECIMAL_BUILD_MODULE
#include <array>
#include <type_traits>
#endif

namespace boost {
namespace decimal {

namespace detail {

template <typename T>
constexpr auto log1p_impl(const T x) noexcept
    BOOST_DECIMAL_REQUIRES(detail::is_decimal_floating_point_v, T)
{
    constexpr T one { 1, 0 };

    T result { };

    const auto fpc = fpclassify(x);

    if (fpc == FP_ZERO)
    {
        result = x;
    }
    #ifndef BOOST_DECIMAL_FAST_MATH
    else if (fpc != FP_NORMAL)
    {
        result =
        (
            ((fpc == FP_INFINITE) && signbit(x)) ? std::numeric_limits<T>::quiet_NaN() : x
        );
    }
    #endif
    else if (-x >= one)
    {
        #ifndef BOOST_DECIMAL_FAST_MATH
        result =
        (
            (-x == one) ? -std::numeric_limits<T>::infinity() : std::numeric_limits<T>::quiet_NaN()
        );
        #else
        result = T{0};
        #endif
    }
    else
    {
        constexpr T small_argument { 1, -std::numeric_limits<T>::digits10 };

        if (abs(x) < small_argument)
        {
            // log1p(x) = x - x^2/2 + ... For a value of |x| this small, the sum
            // of the terms after x is less than one half of the last digit of x.
            result = x;
        }
        else if (abs(x) > T { 5, -1 })
        {
            result = ::boost::decimal::log(x + one);
        }
        else
        {
            // log1p(x) = 2 * atanh(w), with w = x / (2 + x). For |x| not more than
            // 1/2 the value of |w| is not more than 1/3, which the coefficient table
            // covers. The first branch takes every larger |x| through log(x + 1).
            // Two corrections make the result increase at every argument. The sum
            // 2 + x is not exact, and its error makes the quotient fall where x rises,
            // thus e holds that error and wh + wl holds the reduction to about two
            // times the digits of the type. The first coefficient is exactly 2, thus
            // 2*wh stays out of the rounding of the series and only tail rounds.
            constexpr T two { 2, 0 };

            const T d  { two + x };
            const T e  { x - (d - two) };  // 2 + x == d + e, exact
            const T wh { x / d };
            const T r  { detail::unchecked_fma(-wh, d, x) - wh * e };
            const T wl { r / d };

            const T w    { wh + wl };
            const T y    { w * w };
            const T tail { w * y * detail::log1p_series_tail(y) };

            result = detail::unchecked_fma(two, wh, detail::unchecked_fma(two, wl, tail));
        }
    }

    return result;
}

} // namespace detail

BOOST_DECIMAL_EXPORT template <typename T>
constexpr auto log1p(const T x) noexcept
    BOOST_DECIMAL_REQUIRES(detail::is_decimal_floating_point_v, T)
{
    using evaluation_type = detail::evaluation_type_t<T>;

    return static_cast<T>(detail::log1p_impl(static_cast<evaluation_type>(x)));
}

} // namespace decimal
} // namespace boost

#endif // BOOST_DECIMAL_DETAIL_CMATH_LOG1P_HPP
