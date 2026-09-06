// Copyright 2023 Matt Borland
// Copyright 2023 - 2026 Christopher Kormanyos
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_DECIMAL_DETAIL_CMATH_ASINH_HPP
#define BOOST_DECIMAL_DETAIL_CMATH_ASINH_HPP

#include <boost/decimal/fwd.hpp> // NOLINT(llvm-include-order)
#include <boost/decimal/detail/type_traits.hpp>
#include <boost/decimal/detail/concepts.hpp>
#include <boost/decimal/numbers.hpp>

#ifndef BOOST_DECIMAL_BUILD_MODULE
#include <array>
#include <type_traits>
#endif

namespace boost {
namespace decimal {

namespace detail {

template <typename T>
constexpr auto asinh_impl(const T x) noexcept
    BOOST_DECIMAL_REQUIRES(detail::is_decimal_floating_point_v, T)
{
    T result { };

    if (fpclassify(x) != FP_NORMAL)
    {
        result = x;
    }
    else
    {
        // Use (parts of) the implementation of asinh from Boost.Math.

        constexpr T zero { 0, 0 };
        constexpr T one  { 1, 0 };

        if (x < zero)
        {
            result = -asinh_impl(-x);
        }
        else if (x > zero)
        {
            constexpr T tenth_root_epsilon { exp(log(std::numeric_limits<T>::epsilon()) / 10) };

            const auto xsq = x * x;

            if (x > one / tenth_root_epsilon)
            {
                // http://functions.wolfram.com/ElementaryFunctions/ArcSinh/06/01/06/01/0001/
                // approximation by Laurent series in 1/x at 0+ order from -1 to 9
                const auto inv_xsq = one / xsq;

                result =
                    numbers::ln2_v<T>
                  + ::boost::decimal::log(x)
                  + inv_xsq *
                    (
                        one / T { 4, 0 }
                      + inv_xsq *
                        (
                            -T { 3, 0 } / T { 32, 0 }
                          + inv_xsq *
                            (
                                T { 5, 0 } / T { 96, 0 }
                              + inv_xsq *
                                (
                                    -T { 35, 0 } / T { 1024, 0 }
                                  + inv_xsq * (T { 63, 0 } / T { 2560, 0 })
                                )
                            )
                        )
                    );
            }
            else if(x >= T { 4 , -1 })
            {
                // http://functions.wolfram.com/ElementaryFunctions/ArcSinh/02/
                result = ::boost::decimal::log(x + sqrt(xsq + one));
            }
            else if (x >= tenth_root_epsilon)
            {
                // As below, but rearranged to preserve digits:
                const auto sqrt_xsq_plus_one = sqrt(one + xsq);
                const auto sqrt_minus_one = xsq / (sqrt_xsq_plus_one + one);

                result = ::boost::decimal::log1p(x + sqrt_minus_one);
            }
            else
            {
                // Normal[Series[ArcSinh[x], {x, 0, 9}]]
                // FullSimplify[%]
                // HornerForm[%]
                result = x * (1 + xsq * (-(one/6) + xsq * (T { 3, 0 } / 40 + xsq * (-(T { 5, 0 } / 112) + (35 * xsq) / 1152))));
            }
        }
    }

    return result;
}

} // namespace detail

BOOST_DECIMAL_EXPORT template <typename T>
constexpr auto asinh(const T x) noexcept
    BOOST_DECIMAL_REQUIRES(detail::is_decimal_floating_point_v, T)
{
    using evaluation_type = detail::evaluation_type_t<T>;

    return static_cast<T>(detail::asinh_impl(static_cast<evaluation_type>(x)));
}

} // namespace decimal
} // namespace boost

#endif // BOOST_DECIMAL_DETAIL_CMATH_ASINH_HPP
