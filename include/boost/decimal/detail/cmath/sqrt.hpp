// Copyright 2023 - 2024 Matt Borland
// Copyright 2023 - 2024 Christopher Kormanyos
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_DECIMAL_DETAIL_CMATH_SQRT_HPP
#define BOOST_DECIMAL_DETAIL_CMATH_SQRT_HPP

#include <boost/decimal/fwd.hpp>
#include <boost/decimal/detail/type_traits.hpp>
#include <boost/decimal/detail/concepts.hpp>
#include <boost/decimal/detail/config.hpp>

#ifndef BOOST_DECIMAL_BUILD_MODULE
#include <type_traits>
#include <cstdint>
#include <cmath>
#endif

namespace boost {
namespace decimal {

namespace detail {

template <typename T>
constexpr auto sqrt_impl(const T x) noexcept
    BOOST_DECIMAL_REQUIRES(detail::is_decimal_floating_point_v, T)
{
    const auto fpc = fpclassify(x);

    T result { };

    #ifndef BOOST_DECIMAL_FAST_MATH
    if ((fpc == FP_NAN) || (fpc == FP_ZERO))
    {
        result = x;
    }
    else if (signbit(x))
    {
        result = std::numeric_limits<T>::quiet_NaN();
    }
    else if (fpc == FP_INFINITE)
    {
        result = std::numeric_limits<T>::infinity();
    }
    #else
    if (signbit(x))
    {
        result = T{0};
    }
    #endif
    else
    {
        int exp10val { };

        const auto gn { frexp10(x, &exp10val) };

        const auto
            zeros_removal
            {
                remove_trailing_zeros(gn)
            };

        const bool is_pure { zeros_removal.trimmed_number == 1U };

        constexpr T one { 1 };

        if(is_pure)
        {
            // Here, a pure power-of-10 argument gets a straightforward result.
            // For argument 10^n where n is even, the result is exact.

            const int p10 { exp10val + static_cast<int>(zeros_removal.number_of_removed_zeros) };

            if (p10 == 0)
            {
                result = one;
            }
            else
            {
                const int p10_mod2 = (p10 % 2);

                result = T { 1, p10 / 2 };

                if (p10_mod2 == 1)
                {
                    result *= numbers::sqrt10_v<T>;
                }
                else if (p10_mod2 == -1)
                {
                    result /= numbers::sqrt10_v<T>;
                }
            }
        }
        else
        {
            // Scale the argument to the interval 1/10 <= x < 1.
            T gx { gn, -std::numeric_limits<T>::digits10 };

            exp10val += std::numeric_limits<T>::digits10;

            // For this work we perform an order-2 Pade approximation of the square root
            // at argument x = 1/2. This results in slightly more than 2 decimal digits
            // of accuracy over the interval 1/10 <= x < 1.

            // See also WolframAlpha at:
            //   https://www.wolframalpha.com/input?i=FullSimplify%5BPadeApproximant%5BSqrt%5Bx%5D%2C+%7Bx%2C+1%2F2%2C+%7B2%2C+2%7D%7D%5D%5D

            // PadeApproximant[Sqrt[x], {x, 1/2, {2, 2}}]
            // FullSimplify[%]

            // HornerForm[Numerator[Out[2]]]
            // Results in:
            //   1 + x (20 + 20 x)

            // HornerForm[Denominator[Out[2]]]
            // Results in:
            //   5 Sqrt[2] + x (20 Sqrt[2] + 4 Sqrt[2] x)

            constexpr T five { 5 };

            result =
                  (one + gx * ((one + gx) * 20))
                / (numbers::sqrt2_v<T> * ((gx * 4) * (five + gx) + five));

            // Perform 3, 4 or 5 Newton-Raphson iterations depending on precision.
            // Note from above, we start with slightly more than 2 decimal digits
            // of accuracy.

            constexpr int iter_loops
            {
                  std::numeric_limits<T>::digits10 < 10 ? 3
                : std::numeric_limits<T>::digits10 < 20 ? 4 : 5
            };

            for (int idx = 0; idx < iter_loops; ++idx)
            {
                result = (result + gx / result) / 2;
            }

            if (exp10val != 0)
            {
                // Rescale the result if necessary.

                const int exp10val_mod2 = (exp10val % 2);

                result *= T { 1, exp10val / 2 };

                if (exp10val_mod2 == 1)
                {
                    result *= numbers::sqrt10_v<T>;
                }
                else if (exp10val_mod2 == -1)
                {
                    result /= numbers::sqrt10_v<T>;
                }
            }
        }
    }

    return result;
}

} //namespace detail

BOOST_DECIMAL_EXPORT template <typename T>
constexpr auto sqrt(const T val) noexcept
    BOOST_DECIMAL_REQUIRES(detail::is_decimal_floating_point_v, T)
{
    using evaluation_type = detail::evaluation_type_t<T>;

    return static_cast<T>(detail::sqrt_impl(static_cast<evaluation_type>(val)));
}

} //namespace decimal
} //namespace boost

#endif //BOOST_DECIMAL_DETAIL_CMATH_SQRT_HPP

