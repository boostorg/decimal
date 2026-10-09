// Copyright 2026 Shen-Ta Hsieh
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_DECIMAL_DETAIL_CMATH_IMPL_EXP_IMPL_HPP
#define BOOST_DECIMAL_DETAIL_CMATH_IMPL_EXP_IMPL_HPP

#include <boost/decimal/fwd.hpp>
#include <boost/decimal/detail/config.hpp>
#include <boost/decimal/detail/int128.hpp>
#include <boost/decimal/detail/power_tables.hpp>
#include <boost/decimal/detail/cmath/frexp10.hpp>
#include <boost/decimal/detail/cmath/impl/fixed_point.hpp>

#ifndef BOOST_DECIMAL_BUILD_MODULE
#include <cstddef>
#include <cstdint>
#include <limits>
#include <type_traits>
#endif

namespace boost {
namespace decimal {
namespace detail {
namespace exp_detail {

// The coefficients 1/(first + step n)! for n < M, in N words
template <int N, std::size_t M>
constexpr auto exp_head(fx<N> (&c)[M], const std::size_t first = 0U, const std::size_t step = 1U) noexcept -> void
{
    for (std::size_t n {}; n < M; ++n)
    {
        c[n] = fx_top<N>(fx_table::inv_factorial[first + step * n]);
    }
}

// exp(-z) for 0 <= z <= ln(2) / 256 by the Taylor series, whose terms alternate in sign. The terms from
// z^4 on need fewer words. The error is below 2^-60, 2^-100 and 2^-160 for one, two and three words.
constexpr auto exp_neg_series(const fx<1>& z) noexcept -> fx<1>
{
    fx<1> head[7] {};
    exp_head(head);
    return fx_alternating(z, head);
}

constexpr auto exp_neg_series(const fx<2>& z) noexcept -> fx<2>
{
    fx<2> head[4] {};
    exp_head(head);
    return fx_alternating_split<fx_table::exp_d64_shift>(z, head, fx_table::exp_d64_tail);
}

constexpr auto exp_neg_series(const fx<3>& z) noexcept -> fx<3>
{
    fx<3> head[4] {};
    exp_head(head);
    return fx_alternating_split<fx_table::exp_d128_mid_shift, fx_table::exp_d128_tail_shift>(z, head, fx_table::exp_d128_mid, fx_table::exp_d128_tail);
}

// exp(-x) is below half of the smallest subnormal value and exp(x), sinh(x) and cosh(x) overflow for |x|
// at or above this bound. As sig has p digits, |x| < bound gives e < 0.
template <typename T>
constexpr auto exp_bound() noexcept -> std::uint64_t
{
    return static_cast<std::uint64_t>(std::numeric_limits<T>::max_exponent10 + std::numeric_limits<T>::digits10 + 1) * UINT64_C(2302586) / UINT64_C(1000000) + 1U;
}

// The integer part of |x| = sig * 10^-k, or the bound for k <= 0, in 64 bits where sig fits
template <typename T>
constexpr auto exp_whole(const boost::int128::uint128_t& sig, const int k) noexcept -> boost::int128::uint128_t
{
    if (k > 0 && k < 20 && sig.high == 0U)
    {
        return sig.low / pow10(static_cast<std::uint64_t>(k));
    }
    return k <= 0 ? boost::int128::uint128_t {exp_bound<T>()} : k < 39 ? sig / pow10(static_cast<boost::int128::uint128_t>(k)) : boost::int128::uint128_t {};
}

// The fraction of |x| = sig * 10^-k, for the integer part whole
template <int words>
BOOST_DECIMAL_FORCE_INLINE constexpr auto exp_frac(const boost::int128::uint128_t& sig, const int k, const std::uint64_t whole) noexcept -> fx<words>
{
    return fx_from_decimal<words>(k < 39 ? sig - static_cast<boost::int128::uint128_t>(whole) * pow10(static_cast<boost::int128::uint128_t>(k)) : sig, k);
}

// Whether |x| = whole + frac is below 1/16, where the Taylor series of sinh, cosh, tanh and expm1 converge fast
template <int words>
constexpr auto exp_is_small(const std::uint64_t whole, const fx<words>& frac) noexcept -> bool
{
    return whole == 0U && frac.w[words - 1] < (UINT64_C(1) << 58U);
}

// The number of terms in x^2 of these series for |x| < 1/16 in words words
constexpr auto exp_small_terms(const int words) noexcept -> std::size_t
{
    return words == 1 ? 5U : words == 2 ? 8U : 12U;
}

// y = exp10 + f with 0 <= f < 1
template <int words>
struct exp_parts
{
    fx<words> f;
    int exp10;
};

// y = |x| * log10(e) for |x| = whole + frac
template <int words>
constexpr auto exp_log10(const std::uint64_t whole, const fx<words>& frac) noexcept -> exp_parts<words>
{
    // whole * log10(e) + frac * log10(e) in words + 2 words, scaled by 2^(64 words + 62)
    std::uint64_t y[static_cast<std::size_t>(words + 2)] {};
    const auto carry {fx_mul_add(fx_make_span<words + 1>(y), fx_make_span<words + 1>(fx_table::log10_e.w, 3 - words), whole)};
    y[words + 1] = carry;
    const auto low {fx_mul(frac, fx_top<words>(fx_table::log10_e))};
    y[words + 1] += fx_add_to(fx_make_span<words>(y, 1), fx_make_span(low.w));

    exp_parts<words> r {{}, static_cast<int>((y[words + 1] << 2U) | (y[words] >> 62U))};
    for (int i {}; i < words; ++i)
    {
        r.f.w[i] = y[i + 1];
    }
    r.f.w[words - 1] &= (UINT64_C(1) << 62U) - 1U;
    return r;
}

// 10^f = 2^a * m with 1 <= m < 2
template <int words>
struct exp_mantissa
{
    fx<words> m;
    unsigned a;
};

// 10^f for 0 <= f <= 1: 10^f = 2^(a + i/16 + j/256) * exp(-v ln(2)) with 0 < v <= 1/256
template <int words>
constexpr auto exp10_frac(const fx<words>& f) noexcept -> exp_mantissa<words>
{
    // t = f * log2(10) = a + i/16 + (j - 1)/256 + r, and v = 1/256 - r
    auto t {fx_mul(f, fx_top<words>(fx_table::log2_10))};
    const auto a {static_cast<unsigned>(t.w[words - 1] >> 62U)};
    const auto i {static_cast<std::size_t>((t.w[words - 1] >> 58U) & 15U)};
    const auto j {static_cast<std::size_t>((t.w[words - 1] >> 54U) & 15U) + 1U};
    t.w[words - 1] &= (UINT64_C(1) << 54U) - 1U;
    fx<words> step {};
    step.w[words - 1] = UINT64_C(1) << 54U;
    const auto v {fx_sub(step, t)};

    const auto coarse {fx_mul(fx_top<words>(fx_table::pow2[i]), fx_top<words>(fx_table::pow2_fine[j]))};
    return {fx_mul(coarse, exp_neg_series(fx_mul(v, fx_top<words>(fx_table::ln2)))), a};
}

// exp(x) for a finite x != 0, rounded once in the current mode. With y = x * log10(e) = k + f and
// 0 <= f < 1, exp(x) = 10^k * 10^f.
template <typename T>
constexpr auto exp_fixed(const T x) noexcept -> T
{
    constexpr int p {std::numeric_limits<T>::digits10};
    constexpr int words {fx_words(p)};

    // One word gives 19 digits of the result, and two or three words give 38
    constexpr int scale10 {fx_scale10(words)};
    using digits_type = fx_digits_t<words>;

    int e {};
    const auto sig {static_cast<boost::int128::uint128_t>(frexp10(x, &e))};
    const bool neg {signbit(x)};

    const int k {-e};
    const auto wide_whole {exp_whole<T>(sig, k)};
    const auto whole {static_cast<std::uint64_t>(wide_whole)};
    if (wide_whole >= exp_bound<T>())
    {
        return neg ? T {1, -2 * std::numeric_limits<T>::max_exponent10} : T {1, 2 * std::numeric_limits<T>::max_exponent10};
    }

    // For |x| < 10^-(p + 1), exp(x) rounds as a value just above or just below 1
    if (k >= 2 * p + 1)
    {
        return neg ? fx_round<T>(static_cast<digits_type>(pow10(static_cast<boost::int128::uint128_t>(scale10 + 1)) - 1U), -scale10 - 1, false)
                   : fx_round<T>(static_cast<digits_type>(pow10(static_cast<boost::int128::uint128_t>(scale10))), -scale10, false);
    }

    auto y {exp_log10(whole, exp_frac<words>(sig, k, whole))};

    // -y = (-k - 1) + (1 - f)
    if (neg)
    {
        y.exp10 = -y.exp10 - 1;
        y.f = fx_sub(fx_one<words>(), y.f);
    }

    // m / 2 < 1 for fx_scale, and 10^scale10 * 2^(a + 1) * m / 2 is 10^f with scale10 + 1 digits
    const auto t {exp10_frac(y.f)};
    const auto digits {fx_scale(pow10(static_cast<boost::int128::uint128_t>(scale10)) << (t.a + 1U), fx_shr(t.m, 1U))};
    return fx_round<T>(static_cast<digits_type>(digits), y.exp10 - scale10, false);
}

} // namespace exp_detail
} // namespace detail
} // namespace decimal
} // namespace boost

#endif // BOOST_DECIMAL_DETAIL_CMATH_IMPL_EXP_IMPL_HPP
