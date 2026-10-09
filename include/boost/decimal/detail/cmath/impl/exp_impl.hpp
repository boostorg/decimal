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

// The top N words of a, which truncates it to N words
template <int N, int M>
constexpr auto fx_top(const fx<M>& a) noexcept -> fx<N>
{
    fx<N> r {};
    for (int i {}; i < N; ++i)
    {
        r.w[i] = a.w[M - N + i];
    }
    return r;
}

// The first M coefficients 1/n! of the Taylor series of exp(-z), in N words
template <int N, std::size_t M>
constexpr auto exp_head(fx<N> (&c)[M]) noexcept -> void
{
    for (std::size_t n {}; n < M; ++n)
    {
        c[n] = fx_top<N>(fx_table::inv_factorial[n]);
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

// exp(x) for a finite x != 0, rounded once in the current mode. With y = x * log10(e) = k + f and
// 0 <= f < 1, exp(x) = 10^k * 10^f, and 10^f = 2^(a + i/16 + j/256) * exp(-v ln(2)) with 0 < v <= 1/256.
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

    // Above bound, exp(x) overflows and exp(-x) is below half of the smallest subnormal value.
    // As sig has p digits, |x| < bound gives e < 0.
    constexpr std::uint64_t bound {static_cast<std::uint64_t>(std::numeric_limits<T>::max_exponent10 + p + 1) * UINT64_C(2302586) / UINT64_C(1000000) + 1U};
    const int k {-e};
    const boost::int128::uint128_t wide_whole {k <= 0 ? boost::int128::uint128_t {bound} : k < 39 ? sig / pow10(static_cast<boost::int128::uint128_t>(k)) : boost::int128::uint128_t {}};
    const auto whole {static_cast<std::uint64_t>(wide_whole)};
    if (wide_whole >= bound)
    {
        return neg ? T {1, -2 * std::numeric_limits<T>::max_exponent10} : T {1, 2 * std::numeric_limits<T>::max_exponent10};
    }

    // For |x| < 10^-(p + 1), exp(x) rounds as a value just above or just below 1
    if (k >= 2 * p + 1)
    {
        return neg ? fx_round<T>(static_cast<digits_type>(pow10(static_cast<boost::int128::uint128_t>(scale10 + 1)) - 1U), -scale10 - 1, false)
                   : fx_round<T>(static_cast<digits_type>(pow10(static_cast<boost::int128::uint128_t>(scale10))), -scale10, false);
    }

    // The fraction of |x| is part * 10^-scale with scale >= 38, as fx_from_far needs
    auto part {k < 39 ? sig - static_cast<boost::int128::uint128_t>(whole) * pow10(static_cast<boost::int128::uint128_t>(k)) : sig};
    int scale {k};
    if (scale < 38)
    {
        part *= pow10(static_cast<boost::int128::uint128_t>(38 - scale));
        scale = 38;
    }
    const auto frac {fx_from_far<words>(part, scale)};

    // y = whole * log10(e) + frac * log10(e) in words + 2 words, scaled by 2^(64 words + 62)
    std::uint64_t y[static_cast<std::size_t>(words + 2)] {};
    const auto carry {fx_mul_add(fx_make_span<words + 1>(y), fx_make_span<words + 1>(fx_table::log10_e.w, 3 - words), whole)};
    y[words + 1] = carry;
    const auto low {fx_mul(frac, fx_top<words>(fx_table::log10_e))};
    y[words + 1] += fx_add_to(fx_make_span<words>(y, 1), fx_make_span(low.w));

    int exp10 {static_cast<int>((y[words + 1] << 2U) | (y[words] >> 62U))};
    fx<words> f {};
    for (int i {}; i < words; ++i)
    {
        f.w[i] = y[i + 1];
    }
    f.w[words - 1] &= (UINT64_C(1) << 62U) - 1U;

    // -y = (-k - 1) + (1 - f)
    if (neg)
    {
        exp10 = -exp10 - 1;
        f = fx_sub(fx_one<words>(), f);
    }

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
    const auto m {fx_mul(coarse, exp_neg_series(fx_mul(v, fx_top<words>(fx_table::ln2))))};

    // m / 2 < 1 for fx_scale, and 10^scale10 * 2^(a + 1) * m / 2 is 10^f with scale10 + 1 digits
    auto half {m};
    for (int word {}; word < words; ++word)
    {
        half.w[word] = (m.w[word] >> 1U) | (word + 1 < words ? m.w[word + 1] << 63U : UINT64_C(0));
    }
    const auto digits {fx_scale(pow10(static_cast<boost::int128::uint128_t>(scale10)) << (a + 1U), half)};
    return fx_round<T>(static_cast<digits_type>(digits), exp10 - scale10, false);
}

} // namespace exp_detail
} // namespace detail
} // namespace decimal
} // namespace boost

#endif // BOOST_DECIMAL_DETAIL_CMATH_IMPL_EXP_IMPL_HPP
