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
            constexpr T four_tenths { 4, -1 };

            const auto xsq = x * x;

            constexpr T asymp_x { one / tenth_root_epsilon };

            if (x > asymp_x)
            {
                // http://functions.wolfram.com/ElementaryFunctions/ArcSinh/06/01/06/01/0001/
                // approximation by Laurent series in 1/x at 0+ order from -1 to 9
                const auto inv_xsq = one / xsq;

                constexpr T one_fourth { T { 1, 0 } / T { 4, 0 } };
                constexpr T minus_three_over_32 { -T { 3, 0 } / T { 32, 0 } };
                constexpr T five_over_96 { T { 5, 0 } / T { 96, 0 } };
                constexpr T minus_thirty_five_over_1024 { -T { 35, 0 } / T { 1024, 0 } };
                constexpr T sixty_three_over_2560 { T { 63, 0 } / T { 2560, 0 } };

                result =
                    numbers::ln2_v<T>
                  + ::boost::decimal::log(x)
                  + inv_xsq *
                    (
                        one_fourth
                      + inv_xsq *
                        (
                            minus_three_over_32
                          + inv_xsq *
                            (
                                five_over_96
                              + inv_xsq *
                                (
                                    minus_thirty_five_over_1024
                                  + inv_xsq * sixty_three_over_2560
                                )
                            )
                        )
                    );
            }
            else if(x >= four_tenths)
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
                constexpr T minus_one_sixth { -T { 1, 0 } / T { 6, 0 } };
                constexpr T three_over_40 { T { 3, 0 } / T { 40, 0 } };
                constexpr T minus_five_over_112 { -T { 5, 0 } / T { 112, 0 } };
                constexpr T thirty_five_over_1152 { T { 35, 0 } / T { 1152, 0 } };

                result = x * (one + xsq * (minus_one_sixth + xsq * (three_over_40 + xsq * (minus_five_over_112 + thirty_five_over_1152 * xsq))));
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
