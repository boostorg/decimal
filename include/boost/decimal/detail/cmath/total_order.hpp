// Copyright 2025 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_DECIMAL_DETAIL_CMATH_TOTAL_ORDER_HPP
#define BOOST_DECIMAL_DETAIL_CMATH_TOTAL_ORDER_HPP

#include <boost/decimal/detail/config.hpp>
#include <boost/decimal/detail/concepts.hpp>
#include <boost/decimal/detail/promotion.hpp>

namespace boost {
namespace decimal {

namespace detail {

template <typename T>
constexpr auto total_ordering_impl(const T x, const T y) noexcept
    BOOST_DECIMAL_REQUIRES_RETURN(detail::is_decimal_floating_point_v, T, bool)
{
    // Part d: Check for unordered values
    const auto x_nan {isnan(x)};
    const auto x_neg {signbit(x)};
    const auto y_nan {isnan(y)};
    const auto y_neg {signbit(y)};

    if (x_nan && x_neg && !y_nan)
    {
        // d.1
        return true;
    }
    if (!x_nan && y_nan && !y_neg)
    {
        // d.2
        return true;
    }
    if (x_nan && y_nan)
    {
        // d.3.i
        if (x_neg && !y_neg)
        {
            return true;
        }
        // d.3.ii
        const auto x_signaling {issignaling(x)};
        const auto y_signaling {issignaling(y)};
        if (x_signaling && !y_signaling)
        {
            return !y_neg;
        }
        // d.3.iii

    }
}

} // namespace detail

template <typename T1, typename T2>
constexpr auto total_order(const T1 lhs, const T2 rhs) noexcept
    BOOST_DECIMAL_REQUIRES_TWO_RETURN(detail::is_decimal_floating_point_v, T1, detail::is_decimal_floating_point_v, T2, bool)
{
    using larger_type = std::conditional_t<detail::decimal_val_v<T1> >= detail::decimal_val_v<T2>, T1, T2>;
    return detail::total_ordering_impl(static_cast<larger_type>(lhs), static_cast<larger_type>(rhs));
}

} // namespace decimal
} // namespace boost

#endif // BOOST_DECIMAL_DETAIL_CMATH_TOTAL_ORDER_HPP
