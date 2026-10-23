// Copyright 2024 Matt Borland
// Copyright 2024 Christopher Kormanyos
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_DECIMAL_DETAIL_CMATH_LOG2_HPP
#define BOOST_DECIMAL_DETAIL_CMATH_LOG2_HPP

#include <boost/decimal/fwd.hpp> // NOLINT(llvm-include-order)
#include <boost/decimal/detail/type_traits.hpp>
#include <boost/decimal/detail/concepts.hpp>
#include <boost/decimal/detail/config.hpp>
#include <boost/decimal/numbers.hpp>
#include <boost/decimal/detail/cmath/log.hpp>
#include <boost/decimal/detail/cmath/log1p.hpp>

#ifndef BOOST_DECIMAL_BUILD_MODULE
#include <cmath>
#include <type_traits>
#endif

namespace boost {
namespace decimal {

namespace detail {

template <typename T>
constexpr auto log2_impl(const T x) noexcept
    BOOST_DECIMAL_REQUIRES(detail::is_decimal_floating_point_v, T)
{
    constexpr T one { 1 };

    if ((x != one) && (x >= T { 5, -1 }) && (x <= T { 15, -1 }))
    {
        // As in log10, the two parts of log1p near 1 round only once.
        const auto parts { detail::log1p_parts(x - one) };
        constexpr auto k { detail::two_over_ln2<T>() };

        return detail::unchecked_fma(parts.wh, k.hi, detail::unchecked_fma(parts.wh, k.lo, parts.s * numbers::log2e_v<T>));
    }

    return ::boost::decimal::log(x) / numbers::ln2_v<T>;
}

} //namespace detail

BOOST_DECIMAL_EXPORT template <typename T>
constexpr auto log2(const T x) noexcept
    BOOST_DECIMAL_REQUIRES(detail::is_decimal_floating_point_v, T)
{
    using evaluation_type = detail::evaluation_type_t<T>;

    return static_cast<T>(detail::log2_impl(static_cast<evaluation_type>(x)));
}

} //namespace decimal
} //namespace boost

#endif //BOOST_DECIMAL_DETAIL_CMATH_LOG2_HPP
