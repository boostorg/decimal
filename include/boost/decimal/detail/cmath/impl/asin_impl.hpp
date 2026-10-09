// Copyright 2024 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_DECIMAL_DETAIL_CMATH_IMPL_ASIN_IMPL_HPP
#define BOOST_DECIMAL_DETAIL_CMATH_IMPL_ASIN_IMPL_HPP

#include <boost/decimal/detail/config.hpp>
#include <boost/decimal/detail/concepts.hpp>
#include "../../int128.hpp"
#include <boost/decimal/detail/cmath/impl/fixed_point_series.hpp>
#include <boost/decimal/detail/cmath/fma.hpp>
#include <boost/decimal/detail/cmath/sqrt.hpp>

#ifndef BOOST_DECIMAL_BUILD_MODULE
#include <array>
#include <cstddef>
#include <cstdint>
#endif

namespace boost {
namespace decimal {
namespace detail {

// asin(x) - x = x^3 P(x^2)
template <BOOST_DECIMAL_DECIMAL_FLOATING_TYPE T>
constexpr auto asin_tail(T x) noexcept -> T;

template <> constexpr auto asin_tail<decimal32_t>(decimal32_t x) noexcept -> decimal32_t { return fixed_point_odd_tail(x, fx_table::asin_d32); }
template <> constexpr auto asin_tail<decimal_fast32_t>(decimal_fast32_t x) noexcept -> decimal_fast32_t { return fixed_point_odd_tail(x, fx_table::asin_d32); }
template <> constexpr auto asin_tail<decimal64_t>(decimal64_t x) noexcept -> decimal64_t { return fixed_point_odd_tail(x, fx_table::asin_d64); }
template <> constexpr auto asin_tail<decimal_fast64_t>(decimal_fast64_t x) noexcept -> decimal_fast64_t { return fixed_point_odd_tail(x, fx_table::asin_d64); }
template <> constexpr auto asin_tail<decimal128_t>(decimal128_t x) noexcept -> decimal128_t { return fixed_point_odd_tail(x, fx_table::asin_d128); }
template <> constexpr auto asin_tail<decimal_fast128_t>(decimal_fast128_t x) noexcept -> decimal_fast128_t { return fixed_point_odd_tail(x, fx_table::asin_d128); }

template <BOOST_DECIMAL_DECIMAL_FLOATING_TYPE T>
constexpr auto asin_series(T x) noexcept -> T
{
    return x + asin_tail(x);
}

// Half-angle step for a in (0.5, 1]: s = sqrt((1 - a) / 2), and asin(s) ~= s + the returned low
// part, which also covers the rounding error of s; asin(a) = pi/2 - 2 asin(s), acos(a) = 2 asin(s)
template <BOOST_DECIMAL_DECIMAL_FLOATING_TYPE T>
constexpr auto asin_half_angle(T a, T& s) noexcept -> T
{
    const T w {1 - a};
    const T z {w / 2};
    s = sqrt(z);

    if (s == 0)
    {
        return s;
    }

    // sqrt(w / 2) - s = (w - 2 s^2) / (4 s), times asin'(s) = 1 / sqrt(1 - z) ~= 1 / (1 - z / 2);
    // s + s rounds for s in [0.05, 0.1), and two_s_lo is the exact part it rounds off
    const T two_s {s + s};
    const T two_s_lo {s - (two_s - s)};
    const T c {(detail::unchecked_fma(-two_s, s, w) - two_s_lo * s) / (two_s * (T {2} - z))};
    return asin_tail(s) + c;
}

} //namespace detail
} //namespace decimal
} //namespace boost

#endif //BOOST_DECIMAL_DETAIL_CMATH_IMPL_ASIN_IMPL_HPP
