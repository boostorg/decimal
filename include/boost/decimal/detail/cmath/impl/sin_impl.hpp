// Copyright 2023-2024 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_DECIMAL_DETAIL_CMATH_IMPL_SIN_IMPL_HPP
#define BOOST_DECIMAL_DETAIL_CMATH_IMPL_SIN_IMPL_HPP

#include <boost/decimal/fwd.hpp>
#include <boost/decimal/detail/config.hpp>
#include <boost/decimal/detail/int128.hpp>
#include <boost/decimal/detail/cmath/impl/fixed_point.hpp>
#include <boost/decimal/detail/cmath/impl/trig_reduce.hpp>

#ifndef BOOST_DECIMAL_BUILD_MODULE
#include <cstdint>
#endif

namespace boost {
namespace decimal {
namespace detail {
namespace trig {

constexpr auto sin_poly(const fx<1>& z) noexcept -> fx<1>
{
    return fx_alternating(z, fx_table::sin_d32);
}

constexpr auto sin_poly(const fx<2>& z) noexcept -> fx<2>
{
    return fx_alternating_split<fx_table::sin_d64_shift>(z, fx_table::sin_d64_head, fx_table::sin_d64_tail);
}

constexpr auto sin_poly(const fx<3>& z) noexcept -> fx<3>
{
    return fx_alternating_split<fx_table::sin_d128_mid_shift, fx_table::sin_d128_tail_shift>(z, fx_table::sin_d128_head, fx_table::sin_d128_mid, fx_table::sin_d128_tail);
}

// sin(r) / r for z = r^2, which is below 1 for any r != 0.
template <int N>
constexpr auto sinc_fx(const fx<N>& z) noexcept -> fx<N>
{
    return fx_clamp_below_one(fx_sub(fx_one<N>(), fx_mul(z, sin_poly(z))));
}

// sin(|r|) with the sign neg, for r != 0.
template <typename T>
constexpr auto sin_of(const trig_arg<trig_traits<T>::words>& r, const bool neg) noexcept -> T
{
    const auto rf {fixed_r(r)};
    const auto f {sinc_fx(fx_mul(rf, rf))};
    return fx_round<T>(fx_scale(r.sig, f), -r.k, neg);
}

} // namespace trig
} // namespace detail
} // namespace decimal
} // namespace boost

#endif // BOOST_DECIMAL_DETAIL_CMATH_IMPL_SIN_IMPL_HPP
