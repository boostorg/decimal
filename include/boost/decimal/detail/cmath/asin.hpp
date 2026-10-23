// Copyright 2024 - 2025 Matt Borland
// Copyright 2024 - 2025 Christopher Kormanyos
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_DECIMAL_DETAIL_CMATH_ASIN_HPP
#define BOOST_DECIMAL_DETAIL_CMATH_ASIN_HPP

#include <boost/decimal/fwd.hpp>
#include <boost/decimal/detail/type_traits.hpp>
#include <boost/decimal/detail/concepts.hpp>
#include <boost/decimal/detail/config.hpp>
#include <boost/decimal/detail/cmath/fpclassify.hpp>
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
constexpr auto asin_impl(const T x) noexcept
    BOOST_DECIMAL_REQUIRES(detail::is_decimal_floating_point_v, T)
{
    const auto fpc {fpclassify(x)};
    const auto isneg {signbit(x)};

    if (fpc == FP_ZERO
        #ifndef BOOST_DECIMAL_FAST_MATH
        || fpc == FP_NAN
        #endif
        )
    {
        return x;
    }

    const auto absx { fabs(x) };

    T result { };

    constexpr T cbrt_eps { cbrt(std::numeric_limits<T>::epsilon()) };

    constexpr T one { 1 };

    if (absx <= cbrt_eps)
    {
        result = absx + (absx * absx) * (absx / 6);
    }
    else if (absx <= T { 5, -1 })
    {
        result = asin_series(absx);
    }
    else
    {
        if (absx < one)
        {
            // asin(x) = 2 (pi/4_hi - s) - 2 (lo - pi/4_lo), as pi/2_hi - 2 s can round before lo is added;
            // head is exact for s >= 0.1 (else off by < 0.1 ulp of the result), and so is head + head below 0.5
            T s { };
            const T lo { asin_half_angle(absx, s) - split_pi_values<T>(split_pi_detail::quarter_pi_lo) };
            const T head { split_pi_values<T>(split_pi_detail::quarter_pi_hi) - s };

            result = head < T { 5, -1 } ? (head + head) - (lo + lo) : head + (head - (lo + lo));
        }
        else if (absx > one)
        {
            #ifndef BOOST_DECIMAL_FAST_MATH
            result = std::numeric_limits<T>::quiet_NaN();
            #else
            result = T{0};
            #endif
        }
        else
        {
            result = split_pi_values<T>(split_pi_detail::half_pi_hi);
        }
    }

    // arcsin(-x) == -arcsin(x)
    if (isneg)
    {
        result = -result;
    }

    return result;
}

} //namespace detail

BOOST_DECIMAL_EXPORT template <typename T>
constexpr auto asin(const T x) noexcept
    BOOST_DECIMAL_REQUIRES(detail::is_decimal_floating_point_v, T)
{
    using evaluation_type = detail::evaluation_type_t<T>;

    return static_cast<T>(detail::asin_impl(static_cast<evaluation_type>(x)));
}

} //namespace decimal
} //namespace boost

#endif //BOOST_DECIMAL_DETAIL_CMATH_ASIN_HPP
