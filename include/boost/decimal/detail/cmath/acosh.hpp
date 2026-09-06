// Copyright 2023 Matt Borland
// Copyright 2023 - 2026 Christopher Kormanyos
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_DECIMAL_DETAIL_CMATH_ACOSH_HPP
#define BOOST_DECIMAL_DETAIL_CMATH_ACOSH_HPP

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
constexpr auto acosh_impl(const T x) noexcept
    BOOST_DECIMAL_REQUIRES(detail::is_decimal_floating_point_v, T)
{
    const auto fpc = fpclassify(x);

    T result { };

    #ifndef BOOST_DECIMAL_FAST_MATH
    if (fpc != FP_NORMAL)
    {
        if ((fpc == FP_INFINITE) && (!signbit(x)))
        {
            result = x;
        }
        else if (fpc == FP_ZERO)
        {
            result = -std::numeric_limits<T>::quiet_NaN();
        }
        else
        {
            result = x;
        }
    }
    #else
    if (fpc == FP_ZERO)
    {
        result = T{0, 0};
    }
    #endif
    else
    {
        constexpr T one  { 1, 0 };

        if (x < one)
        {
            // In this case, acosh(x) for x < 1 is -NaN.

            result = -std::numeric_limits<T>::quiet_NaN();
        }
        else if (x > one)
        {
            // Use (parts of) the implementation of acosh from Boost.Math.

            constexpr T root_epsilon { sqrt(std::numeric_limits<T>::epsilon()) };
            constexpr T one_point_five { 15, -1 };

            const auto y = x - one;

            if (y >= root_epsilon)
            {
                constexpr T asymp_x { one / root_epsilon };

                if (x > asymp_x)
                {
                    // http://functions.wolfram.com/ElementaryFunctions/ArcCosh/06/01/06/01/0001/
                    // approximation by Laurent series in 1/x at 0+.
                    const auto inv_xsq = one / (x * x);

                    constexpr T three_over_32 { T { 3, 0 } / T { 32, 0 } };
                    constexpr T one_fourth    { T { 1, 0 } / T { 4, 0 } };
                    constexpr T five_over_96  { T { 5, 0 } / T { 96, 0 } };
                    constexpr T thirty_five_over_1024 { T { 35, 0 } / T { 1024, 0 } };
                    constexpr T sixty_three_over_2560 { T { 63, 0 } / T { 2560, 0 } };
                    constexpr T seventy_seven_over_4096 { T { 77, 0 } / T { 4096, 0 } };

                    result =
                        ::boost::decimal::log(x) + numbers::ln2_v<T>
                      - inv_xsq *
                        (
                            one_fourth
                          + inv_xsq *
                            (
                                three_over_32
                              + inv_xsq *
                                (
                                    five_over_96
                                  + inv_xsq *
                                    (
                                        thirty_five_over_1024
                                      + inv_xsq *
                                        (
                                            sixty_three_over_2560
                                          + inv_xsq * seventy_seven_over_4096
                                        )
                                    )
                                )
                            )
                        );
                }
                else if (x < one_point_five)
                {
                    // This is just a rearrangement of the standard form below
                    // devised to minimise loss of precision when x ~ 1:

                    const auto two_y = y + y;

                    result = ::boost::decimal::log1p(y + sqrt((y * y) + two_y));
                }
                else
                {
                    // http://functions.wolfram.com/ElementaryFunctions/ArcCosh/02/
                    return(::boost::decimal::log(x + sqrt((x * x) - one)));
                }
            }
            else
            {
                // see http://functions.wolfram.com/ElementaryFunctions/ArcCosh/06/01/04/01/0001/

                const auto two_y = y + y;

                constexpr T minus_one_twelfth { -T { 1, 0 } / T { 12, 0 } };
                constexpr T three_over_160 { T { 3, 0 } / T { 160, 0 } };
                constexpr T minus_five_over_896 { -T { 5, 0 } / T { 896, 0 } };
                constexpr T thirty_five_over_18432 { T { 35, 0 } / T { 18432, 0 } };
                constexpr T minus_sixty_three_over_90112 { -T { 63, 0 } / T { 90112, 0 } };
                constexpr T two_hundred_thirty_one_over_425984 { T { 231, 0 } / T { 425984, 0 } };
                constexpr T minus_four_hundred_twenty_nine_over_1966080 { -T { 429, 0 } / T { 1966080, 0 } };
                constexpr T six_thousand_four_hundred_thirty_five_over_71303168 { T { 6435, 0 } / T { 71303168, 0 } };

                // approximation by Taylor series in y at 0 through order 8
                result = sqrt(two_y) *
                (
                    one + y *
                    (
                        minus_one_twelfth + y *
                        (
                            three_over_160 + y *
                            (
                                minus_five_over_896 + y *
                                (
                                    thirty_five_over_18432 + y *
                                    (
                                        minus_sixty_three_over_90112 + y *
                                        (
                                            two_hundred_thirty_one_over_425984 + y *
                                            (
                                                minus_four_hundred_twenty_nine_over_1966080 + y *
                                                six_thousand_four_hundred_thirty_five_over_71303168
                                            )
                                        )
                                    )
                                )
                            )
                        )
                    )
                );
            }
        }
        else
        {
            // This branch handles acosh(1) = 0.
            result = T { 0, 0 };
        }
    }

    return result;
}

} // namespace detail

BOOST_DECIMAL_EXPORT template <typename T>
constexpr auto acosh(const T x) noexcept
    BOOST_DECIMAL_REQUIRES(detail::is_decimal_floating_point_v, T)
{
    using evaluation_type = detail::evaluation_type_t<T>;

    return static_cast<T>(detail::acosh_impl(static_cast<evaluation_type>(x)));
}

} // namespace decimal
} // namespace boost

#endif // BOOST_DECIMAL_DETAIL_CMATH_ACOSH_HPP
