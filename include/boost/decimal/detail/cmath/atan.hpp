// Copyright 2023 - 2024 Matt Borland
// Copyright 2023 - 2024 Christopher Kormanyos
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_DECIMAL_DETAIL_CMATH_ATAN_HPP
#define BOOST_DECIMAL_DETAIL_CMATH_ATAN_HPP

#include <boost/decimal/fwd.hpp> // NOLINT(llvm-include-order)
#include <boost/decimal/detail/cmath/impl/atan_impl.hpp>
#include <boost/decimal/detail/cmath/impl/split_pi.hpp>
#include <boost/decimal/detail/concepts.hpp>
#include <boost/decimal/detail/config.hpp>
#include <boost/decimal/detail/type_traits.hpp>
#include <boost/decimal/detail/cmath/fabs.hpp>
#include <boost/decimal/detail/cmath/fma.hpp>

#ifndef BOOST_DECIMAL_BUILD_MODULE
#include <type_traits>
#include <cstdint>
#endif

namespace boost {
namespace decimal {

namespace detail {

// atan(x + x_lo); with has_lo, x must be a positive normal number and x_lo a small correction
template <bool has_lo = false, typename T>
constexpr auto atan_impl(const T x, const T x_lo = T { }) noexcept
    BOOST_DECIMAL_REQUIRES(detail::is_decimal_floating_point_v, T)
{
    const auto fpc { fpclassify(x) };

    T result { };

    if (fpc == FP_ZERO
        #ifndef BOOST_DECIMAL_FAST_MATH
        || fpc == FP_NAN
        #endif
        )
    {
        result = x;
    }
    else if (signbit(x))
    {
        result = -atan_impl(-x);
    }
    #ifndef BOOST_DECIMAL_FAST_MATH
    else if (fpc == FP_INFINITE)
    {
        result = detail::split_pi_values<T>(detail::split_pi_detail::half_pi_hi);
    }
    #endif
    // The breakpoints are those of fdlibm: 7/16, 11/16, 19/16 and 39/16
    else if (x < T { 4375, -4 })
    {
        // x_lo changes atan by x_lo / (1 + x^2)
        result = has_lo ? x + (detail::atan_tail(x) + x_lo / (T { 1 } + x * x)) : detail::atan_series(x);
    }
    else if (x < T { 24375, -4 })
    {
        // atan(x) = atan(c) + atan(q) with q = (x - c) / (1 + x c) for c = 1/2, 1, 3/2;
        // num is exact, and den_lo is the part of the denominator that den rounds off
        T c_hi { };
        T c_lo { };
        T num { };
        T den { };
        T den_lo { };

        if (x < T { 6875, -4 })
        {
            c_hi = detail::atan_values<T>(detail::atan_detail::atan_half_hi);
            c_lo = detail::atan_values<T>(detail::atan_detail::atan_half_lo);
            const T h { x - T { 5, -1 } };
            num = h + h;
            den = T { 2 } + x;
            den_lo = x - (den - T { 2 });
        }
        else if (x < T { 11875, -4 })
        {
            c_hi = detail::split_pi_values<T>(detail::split_pi_detail::quarter_pi_hi);
            c_lo = detail::split_pi_values<T>(detail::split_pi_detail::quarter_pi_lo);
            num = x - T { 1 };
            den = x + T { 1 };
            den_lo = x - (den - T { 1 });
        }
        else
        {
            // 3 x and 2 + 3 x are exact here, since x >= 1 and 2 + 3 x < 10
            c_hi = detail::atan_values<T>(detail::atan_detail::atan_three_halves_hi);
            c_lo = detail::atan_values<T>(detail::atan_detail::atan_three_halves_lo);
            const T h { x - T { 15, -1 } };
            num = h + h;
            den = T { 2 } + T { 3 } * x;
        }

        // q + q_lo = num / (den + den_lo); for |q| < 0.1 the rounding of q changes the result by
        // less than 0.05 ulp, so only pay for the fma of the remainder when |q| >= 0.1
        const T q { num / den };
        const bool q_on_grid { fabs(q) >= T { 1, -1 } };
        const T q_lo { (q_on_grid ? detail::unchecked_fma(-q, den, num) - q * den_lo : -q * den_lo) / den };
        const T lo { detail::atan_tail(q) + ((has_lo ? c_lo + x_lo / (T { 1 } + x * x) : c_lo) + q_lo) };

        // c_hi + q is exact only if q is on the grid of the result and the sum is below 1;
        // else add q and the low part first, so only one rounding happens at the result grid
        const T hi_q { c_hi + q };
        result = (q_on_grid && hi_q < T { 1 }) ? hi_q + lo : c_hi + (q + lo);
    }
    else
    {
        // atan(x) = pi/2 - atan(1 / x), with 1 / x <= 16/39 in the kernel's range
        // with has_lo, the rounding of r = -1 / x and x_lo r^2 both change atan(r) by 1 / (1 + r^2)
        const T r { T { -1 } / x };
        const T pi_lo { detail::split_pi_values<T>(detail::split_pi_detail::half_pi_lo) };
        result = detail::split_pi_values<T>(detail::split_pi_detail::half_pi_hi) + (has_lo ?
                 r + (detail::atan_tail(r) + (pi_lo + (detail::unchecked_fma(-r, x, T { -1 }) / x + x_lo * (r * r)) / (T { 1 } + r * r))) :
                 detail::atan_series(r) + pi_lo);
    }

    return result;
}

} //namespace detail

BOOST_DECIMAL_EXPORT template <typename T>
constexpr auto atan(const T x) noexcept
    BOOST_DECIMAL_REQUIRES(detail::is_decimal_floating_point_v, T)
{
    using evaluation_type = detail::evaluation_type_t<T>;

    return static_cast<T>(detail::atan_impl(static_cast<evaluation_type>(x)));
}

} //namespace decimal
} //namespace boost

#endif //BOOST_DECIMAL_DETAIL_CMATH_ATAN_HPP
