// Copyright 2023 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_DECIMAL_DETAIL_CMATH_IMPL_COS_IMPL_HPP
#define BOOST_DECIMAL_DETAIL_CMATH_IMPL_COS_IMPL_HPP

#include <boost/decimal/fwd.hpp>
#include <boost/decimal/detail/config.hpp>
#include <boost/decimal/detail/int128.hpp>
#include <boost/decimal/detail/power_tables.hpp>
#include <boost/decimal/detail/cmath/impl/fixed_point.hpp>
#include <boost/decimal/detail/cmath/impl/trig_reduce.hpp>

#ifndef BOOST_DECIMAL_BUILD_MODULE
#include <cstdint>
#endif

namespace boost {
namespace decimal {
namespace detail {
namespace trig {

constexpr auto cos_poly(const fx<1>& z) noexcept -> fx<1>
{
    return fx_alternating(z, fx_table::cos_d32);
}

constexpr auto cos_poly(const fx<2>& z) noexcept -> fx<2>
{
    return fx_alternating_split<fx_table::cos_d64_shift>(z, fx_table::cos_d64_head, fx_table::cos_d64_tail);
}

constexpr auto cos_poly(const fx<3>& z) noexcept -> fx<3>
{
    return fx_alternating_split<fx_table::cos_d128_mid_shift, fx_table::cos_d128_tail_shift>(z, fx_table::cos_d128_head, fx_table::cos_d128_mid, fx_table::cos_d128_tail);
}

// cos(r) for z = r^2, which is below 1 for any r != 0.
template <int N>
constexpr auto cos_fx(const fx<N>& z) noexcept -> fx<N>
{
    fx<N> half {};
    half.w[N - 1] = UINT64_C(1) << 61U;
    const auto inner {fx_sub(half, fx_mul(z, cos_poly(z)))};
    return fx_clamp_below_one(fx_sub(fx_one<N>(), fx_mul(z, inner)));
}

// cos(r) with the sign neg, for r != 0.
template <typename T>
constexpr auto cos_fx_of(const trig_arg<trig_traits<T>::words>& r, const bool neg) noexcept -> T
{
    const auto rf {fixed_r(r)};
    const auto f {cos_fx(fx_mul(rf, rf))};
    return fx_round<T>(fx_scale(pow10(static_cast<boost::int128::uint128_t>(38)), f), -38, neg);
}

} // namespace trig
} // namespace detail
} // namespace decimal
} // namespace boost

#endif // BOOST_DECIMAL_DETAIL_CMATH_IMPL_COS_IMPL_HPP
