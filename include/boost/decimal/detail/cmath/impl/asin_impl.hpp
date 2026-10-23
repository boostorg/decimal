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

namespace asin_detail {

// Use a struct of arrays so that we can have static constexpr arrays of coefficients.
// See https://github.com/boostorg/math/issues/923 for further information
template <bool b>
struct asin_table_imp
{
    // Chebyshev fits of P in asin(x) = x + x^3 P(x^2) for x in [0, 0.5], highest power first, as
    // raw fixed point (see fixed_point_series.hpp): n = round(c * 2^62) or n = round(c * 2^126)

    // Q62, degree 5 in x^2, max kernel error 0.0479 ulp at 7 digits
    static constexpr std::array<std::int64_t, 6> d32_coeffs = {{
        INT64_C(155371621893934362),
        INT64_C(79086910334655698),
        INT64_C(143426478885863926),
        INT64_C(205678429410250444),
        INT64_C(345880832483771599),
        INT64_C(768614490127431932)
    }};

    // Q62, degree 12 in x^2, max kernel error 0.0452 ulp at 16 digits
    static constexpr std::array<std::int64_t, 13> d64_coeffs = {{
        INT64_C(132622181071150947),
        INT64_C(-68492239953733199),
        INT64_C(80247392434212570),
        INT64_C(25168307429827271),
        INT64_C(47605578609573363),
        INT64_C(52938361988655692),
        INT64_C(64430847530605378),
        INT64_C(80023786897097220),
        INT64_C(103173437159152167),
        INT64_C(140111986996306496),
        INT64_C(205878840124498533),
        INT64_C(345876451381981828),
        INT64_C(768614336404564804)
    }};

    // Q126, degree 28 in x^2, max kernel error 0.043 ulp at 34 digits
    static constexpr std::array<int128::int128_t, 29> d128_coeffs = {{
        int128::int128_t {INT64_C(370876500756886001), UINT64_C(9221949471032297446)},
        int128::int128_t {INT64_C(-956307695304863090), UINT64_C(9148781051210321482)},
        int128::int128_t {INT64_C(1310549287927847162), UINT64_C(1958114557122675838)},
        int128::int128_t {INT64_C(-1134417282113220194), UINT64_C(17968718786407945997)},
        int128::int128_t {INT64_C(736228940689646156), UINT64_C(16663177741084984228)},
        int128::int128_t {INT64_C(-340829970488362816), UINT64_C(12459077857655578150)},
        int128::int128_t {INT64_C(145857634493384410), UINT64_C(17283691294030731877)},
        int128::int128_t {INT64_C(-29177674511655379), UINT64_C(5106089515100561328)},
        int128::int128_t {INT64_C(23589553566972586), UINT64_C(4940683816029200174)},
        int128::int128_t {INT64_C(11915337754018893), UINT64_C(6093997656988561099)},
        int128::int128_t {INT64_C(15585523597377591), UINT64_C(13114903993071182046)},
        int128::int128_t {INT64_C(16404669177712143), UINT64_C(5788416753019317356)},
        int128::int128_t {INT64_C(17904543839495541), UINT64_C(305658806213704697)},
        int128::int128_t {INT64_C(19557042090611134), UINT64_C(18251937558368413649)},
        int128::int128_t {INT64_C(21491177464483278), UINT64_C(13762596495847561352)},
        int128::int128_t {INT64_C(23765442023041221), UINT64_C(4709006491505993448)},
        int128::int128_t {INT64_C(26471251718294205), UINT64_C(7048697116100673247)},
        int128::int128_t {INT64_C(29732509641295605), UINT64_C(791966617446954024)},
        int128::int128_t {INT64_C(33723073331131660), UINT64_C(5494626099549260253)},
        int128::int128_t {INT64_C(38693594343105809), UINT64_C(14383093717251864696)},
        int128::int128_t {INT64_C(45017478183132254), UINT64_C(3729214876058472566)},
        int128::int128_t {INT64_C(53273278680384444), UINT64_C(15594621309481963446)},
        int128::int128_t {INT64_C(64401474671398092), UINT64_C(16567906768798270425)},
        int128::int128_t {INT64_C(80025501070968044), UINT64_C(5657558833186617601)},
        int128::int128_t {INT64_C(103173373281578635), UINT64_C(11738962649206475942)},
        int128::int128_t {INT64_C(140111988407082097), UINT64_C(14347467083897352460)},
        int128::int128_t {INT64_C(205878840108365531), UINT64_C(7905747461347776107)},
        int128::int128_t {INT64_C(345876451382054092), UINT64_C(14757395258966581313)},
        int128::int128_t {INT64_C(768614336404564650), UINT64_C(12297829382473037246)}
    }};
};

#if !(defined(__cpp_inline_variables) && __cpp_inline_variables >= 201606L) && (!defined(_MSC_VER) || _MSC_VER != 1900)

template <bool b> constexpr std::array<std::int64_t, 6> asin_table_imp<b>::d32_coeffs;
template <bool b> constexpr std::array<std::int64_t, 13> asin_table_imp<b>::d64_coeffs;
template <bool b> constexpr std::array<int128::int128_t, 29> asin_table_imp<b>::d128_coeffs;

#endif

using asin_table = asin_table_imp<true>;

} //namespace asin_detail

// asin(x) - x = x^3 P(x^2)
template <BOOST_DECIMAL_DECIMAL_FLOATING_TYPE T>
constexpr auto asin_tail(T x) noexcept -> T;

template <> constexpr auto asin_tail<decimal32_t>(decimal32_t x) noexcept -> decimal32_t { return fixed_point_odd_tail(x, asin_detail::asin_table::d32_coeffs); }
template <> constexpr auto asin_tail<decimal_fast32_t>(decimal_fast32_t x) noexcept -> decimal_fast32_t { return fixed_point_odd_tail(x, asin_detail::asin_table::d32_coeffs); }
template <> constexpr auto asin_tail<decimal64_t>(decimal64_t x) noexcept -> decimal64_t { return fixed_point_odd_tail(x, asin_detail::asin_table::d64_coeffs); }
template <> constexpr auto asin_tail<decimal_fast64_t>(decimal_fast64_t x) noexcept -> decimal_fast64_t { return fixed_point_odd_tail(x, asin_detail::asin_table::d64_coeffs); }
template <> constexpr auto asin_tail<decimal128_t>(decimal128_t x) noexcept -> decimal128_t { return fixed_point_odd_tail(x, asin_detail::asin_table::d128_coeffs); }
template <> constexpr auto asin_tail<decimal_fast128_t>(decimal_fast128_t x) noexcept -> decimal_fast128_t { return fixed_point_odd_tail(x, asin_detail::asin_table::d128_coeffs); }

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
