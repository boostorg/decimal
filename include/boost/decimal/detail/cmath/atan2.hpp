// Copyright 2024 Matt Borland
// Copyright 2024 Christopher Kormanyos
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_DECIMAL_DETAIL_CMATH_ATAN2_HPP
#define BOOST_DECIMAL_DETAIL_CMATH_ATAN2_HPP

#include <boost/decimal/detail/concepts.hpp>
#include <boost/decimal/detail/cmath/atan.hpp>
#include <boost/decimal/detail/cmath/fabs.hpp>
#include <boost/decimal/detail/cmath/frexp10.hpp>
#include <boost/decimal/detail/cmath/impl/split_pi.hpp>
#include <boost/decimal/detail/type_traits.hpp>
#include <boost/decimal/detail/config.hpp>
#include <boost/decimal/numbers.hpp>

#ifndef BOOST_DECIMAL_BUILD_MODULE
#include <type_traits>
#include <cstdint>
#include <cmath>
#endif

namespace boost {
namespace decimal {

namespace detail {

template <typename T>
constexpr auto atan2_impl(const T y, const T x) noexcept
    BOOST_DECIMAL_REQUIRES(detail::is_decimal_floating_point_v, T)
{
    const auto fpcx {fpclassify(x)};
    const auto fpcy {fpclassify(y)};
    const auto signx {signbit(x)}; // True if neg
    const auto signy {signbit(y)};
    const auto isfinitex {fpcx != FP_INFINITE && fpcx != FP_NAN};
    const auto isfinitey {fpcy != FP_INFINITE && fpcy != FP_NAN};

    T result { };

    #ifndef BOOST_DECIMAL_FAST_MATH
    if (fpcx == FP_NAN)
    {
        result = x;
    }
    else if (fpcy == FP_NAN)
    {
        result = y;
    }
    else
    #endif
    if (fpcy == FP_ZERO && signx)
    {
        result = signy ? -numbers::pi_v<T> : numbers::pi_v<T>;
    }
    else if (fpcy == FP_ZERO && !signx)
    {
        result = y;
    }
    #ifndef BOOST_DECIMAL_FAST_MATH
    else if (fpcy == FP_INFINITE && isfinitex)
    {
        result = split_pi_values<T>(split_pi_detail::half_pi_hi);

        if (signy) { result = -result; }
    }
    else if (fpcy == FP_INFINITE && fpcx == FP_INFINITE && signx)
    {
        result = split_pi_values<T>(split_pi_detail::three_quarter_pi);

        if (signy)
        {
            result = -result; // LCOV_EXCL_LINE : False Negative
        }
    }
    else if (fpcy == FP_INFINITE && fpcx == FP_INFINITE && !signx)
    {
        result = split_pi_values<T>(split_pi_detail::quarter_pi_hi);

        if (signy)
        {
            result = -result; // LCOV_EXCL_LINE : False Negative
        }
    }
    #endif
    else if (fpcx == FP_ZERO)
    {
        result = split_pi_values<T>(split_pi_detail::half_pi_hi);

        if (signy) { result = -result; }
    }
    #ifndef BOOST_DECIMAL_FAST_MATH
    else if (fpcx == FP_INFINITE && signx && isfinitey)
    {
        result = signy ? -numbers::pi_v<T> : numbers::pi_v<T>;
    }
    else if (fpcx == FP_INFINITE && !signx && isfinitey)
    {
        constexpr T zero { 0, 0 };

        result = signy ? -zero : zero;
    }
    #endif
    else
    {
        if (x == T{1, 0})
        {
            result = atan(y);
        }
        else
        {
            // For q in [10^k, tan(10^k)), q has a grid ten times coarser than atan(q); only there
            // pass the remainder of y / x to atan as a low part (tan(1) < 1.5575)
            const T ax {fabs(x)};
            const T q {fabs(y) / ax};
            T ret_val {};
            if (q < T {1})
            {
                ret_val = atan(q);
                int q_exp {};
                frexp10(q, &q_exp);
                if (fpclassify(q) == FP_NORMAL && ret_val < T {1, q_exp + detail::precision_v<T> - 1})
                {
                    ret_val = atan_impl<true>(q, detail::unchecked_fma(-q, ax, fabs(y)) / ax);
                }
            }
            else
            {
                ret_val = q < T {15575, -4} ? atan_impl<true>(q, detail::unchecked_fma(-q, ax, fabs(y)) / ax) : atan(q);
            }

            if (!signy && !signx)
            {
                result = ret_val;
            }
            else if (signy && !signx)
            {
                result = -ret_val;
            }
            else if (!signy && signx)
            {
                result = numbers::pi_v<T> - ret_val;
            }
            else
            {
                result = ret_val - numbers::pi_v<T>;
            }
        }
    }

    return result;
}

} // namespace detail

BOOST_DECIMAL_EXPORT template <typename T>
constexpr auto atan2(const T y, const T x) noexcept
    BOOST_DECIMAL_REQUIRES(detail::is_decimal_floating_point_v, T)
{
    using evaluation_type = detail::evaluation_type_t<T>;

    return static_cast<T>(detail::atan2_impl(static_cast<evaluation_type>(y), static_cast<evaluation_type>(x)));
}

} //namespace decimal
} //namespace boost

#endif //BOOST_DECIMAL_DETAIL_CMATH_ATAN2_HPP
