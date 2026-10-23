// Copyright 2026 Shen-Ta Hsieh
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_DECIMAL_DETAIL_CMATH_IMPL_TAN_IMPL_HPP
#define BOOST_DECIMAL_DETAIL_CMATH_IMPL_TAN_IMPL_HPP

#include <boost/decimal/fwd.hpp>
#include <boost/decimal/detail/config.hpp>
#include <boost/decimal/detail/int128.hpp>
#include <boost/decimal/detail/u256.hpp>
#include <boost/decimal/detail/cmath/impl/trig_fixed_point.hpp>
#include <boost/decimal/detail/cmath/impl/trig_reduce.hpp>
#include <boost/decimal/detail/cmath/impl/sin_impl.hpp>
#include <boost/decimal/detail/cmath/impl/cos_impl.hpp>

#ifndef BOOST_DECIMAL_BUILD_MODULE
#include <cstddef>
#include <cstdint>
#endif

namespace boost {
namespace decimal {
namespace detail {
namespace trig {

// 1/c for c in [0.5, 1]: the seed 2^124 / (top word of c) is good to 61 bits, and two Newton steps
// give more than 190 bits.
template <int N>
constexpr auto fx_reciprocal(const fx<N>& c) noexcept -> fx<N>
{
    fx<N> y {};
    y.w[N - 1] = static_cast<std::uint64_t>(boost::int128::uint128_t {UINT64_C(1) << 60U, UINT64_C(0)} / c.w[N - 1]);

    fx<N> two {};
    two.w[N - 1] = UINT64_C(1) << 63U;
    for (int i {}; i < 2; ++i)
    {
        y = fx_mul(y, fx_sub(two, fx_mul(c, y)));
    }
    return y;
}

// tan(r) for an even quadrant, else -cot(r), for r != 0.
template <typename T>
constexpr auto tan_of(const trig_arg<trig_traits<T>::words>& r) noexcept -> T
{
    constexpr int words {trig_traits<T>::words};
    const auto rf {fixed_r(r)};
    const auto z {fx_mul(rf, rf)};
    const auto s {sinc_fx(z)};
    const auto c {cos_fx(z)};

    if ((r.n & 1U) == 0U)
    {
        // tan(r) = r * (sin(r) / r) / cos(r), and the factor is above 1 for any r != 0.
        auto m {fx_mul(s, fx_reciprocal(c))};
        if (m.w[words - 1] < (UINT64_C(1) << 62U) || fx_is_one(m))
        {
            m = fx_one<words>();
            m.w[0] += 1U;
        }
        return trig_round<T>(fx_scale(r.sig, m), -r.k, r.neg);
    }

    // -cot(r) = -(cos(r) / (sin(r) / r)) / r, and cos(r) / (sin(r) / r) is below 1 for any r != 0.
    const auto m {fx_clamp_below_one(fx_mul(c, fx_reciprocal(s)))};

    // floor(m * 10^75 / 2^(64N - 2)), then divide by sig: for m >= 0.7, the quotient has 37 or 38 digits.
    constexpr std::uint64_t pow10_75[4] {UINT64_C(0), UINT64_C(10084168908774762496),
                                         UINT64_C(12965995782233477362), UINT64_C(159309191113245227)};
    std::uint64_t p[static_cast<std::size_t>(words + 4)] {};
    for (int i {}; i < words; ++i)
    {
        std::uint64_t carry {};
        for (int j {}; j < 4; ++j)
        {
            const auto t {static_cast<boost::int128::uint128_t>(m.w[i]) * pow10_75[j] + p[i + j] + carry};
            p[i + j] = t.low;
            carry = t.high;
        }
        p[i + 4] = carry;
    }
    std::uint64_t num[4] {};
    for (int i {}; i < 4; ++i)
    {
        num[i] = (p[words - 1 + i] >> 62U) | (p[words + i] << 2U);
    }
    const auto dm {impl::div_mod(u256 {num[3], num[2], num[1], num[0]}, r.sig)};
    return trig_round<T>(boost::int128::uint128_t {dm.quotient.bytes[1], dm.quotient.bytes[0]}, r.k - 75, !r.neg);
}

} // namespace trig
} // namespace detail
} // namespace decimal
} // namespace boost

#endif // BOOST_DECIMAL_DETAIL_CMATH_IMPL_TAN_IMPL_HPP
