// Copyright 2026 Shen-Ta Hsieh
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_DECIMAL_DETAIL_CMATH_IMPL_FIXED_POINT_SERIES_HPP
#define BOOST_DECIMAL_DETAIL_CMATH_IMPL_FIXED_POINT_SERIES_HPP

#include <boost/decimal/detail/config.hpp>
#include <boost/decimal/detail/concepts.hpp>
#include <boost/decimal/detail/construction_sign.hpp>
#include <boost/decimal/detail/cmath/frexp10.hpp>
#include <boost/decimal/detail/int128.hpp>
#include <boost/decimal/detail/cmath/impl/fixed_point.hpp>

#ifndef BOOST_DECIMAL_BUILD_MODULE
#include <array>
#include <cstddef>
#include <cstdint>
#include <type_traits>
#endif

namespace boost {
namespace decimal {
namespace detail {

namespace fixed_point_detail {

// A coefficient of the raw fixed point n / 2^(64N - 2) (see fx), as its magnitude and its sign
template <int N>
struct fx_signed
{
    fx<N> m;
    bool neg;
};

constexpr auto from_raw(const std::int64_t n) noexcept -> fx_signed<1>
{
    return fx_signed<1> {fx<1> {{n < 0 ? UINT64_C(0) - static_cast<std::uint64_t>(n) : static_cast<std::uint64_t>(n)}}, n < 0};
}

constexpr auto from_raw(const int128::int128_t& n) noexcept -> fx_signed<2>
{
    const int128::uint128_t m {n < 0 ? -n : n};
    return fx_signed<2> {fx<2> {{m.low, m.high}}, n < 0};
}

// a + b with their signs
template <int N>
constexpr auto add(const fx_signed<N>& a, const fx_signed<N>& b) noexcept -> fx_signed<N>
{
    if (a.neg == b.neg)
    {
        return fx_signed<N> {fx_add(a.m, b.m), a.neg};
    }
    return fx_less(a.m, b.m) ? fx_signed<N> {fx_sub(b.m, a.m), b.neg} : fx_signed<N> {fx_sub(a.m, b.m), a.neg};
}

} // namespace fixed_point_detail

// x^3 P(x^2) from raw fixed-point coefficients of P, highest power first: Q62 (std::int64_t) for one
// word of fx and Q126 (int128_t) for two. u = x^2 P(x^2) is formed in fixed point and rounded once
// to T, and the result is x * u.
template <BOOST_DECIMAL_DECIMAL_FLOATING_TYPE T, typename Raw, std::size_t M>
constexpr auto fixed_point_odd_tail(T x, const std::array<Raw, M>& coeffs) noexcept -> T
{
    constexpr int words {sizeof(Raw) == sizeof(std::int64_t) ? 1 : 2};
    constexpr int scale10 {fx_scale10(words)};
    using digits_type = std::conditional_t<words == 1, std::uint64_t, int128::uint128_t>;

    const T t {x * x};
    int e10 {};
    const auto sig {frexp10(t, &e10)};
    const auto tq {fx_from_decimal<words>(static_cast<int128::uint128_t>(sig), -e10)};

    auto r {fixed_point_detail::from_raw(coeffs[0])};
    for (std::size_t i {1}; i < M; ++i)
    {
        r = fixed_point_detail::add(fixed_point_detail::fx_signed<words> {fx_mul(r.m, tq), r.neg}, fixed_point_detail::from_raw(coeffs[i]));
    }

    const auto digits {static_cast<digits_type>(fx_scale(pow10(static_cast<int128::uint128_t>(scale10)), fx_mul(r.m, tq)))};
    return x * T {digits, -scale10, r.neg ? construction_sign::negative : construction_sign::positive};
}

} // namespace detail
} // namespace decimal
} // namespace boost

#endif // BOOST_DECIMAL_DETAIL_CMATH_IMPL_FIXED_POINT_SERIES_HPP
