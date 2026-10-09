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

// The coefficients 1/(First + Step n)! for n < M, in N words
template <std::size_t First = 0U, std::size_t Step = 1U, int N, std::size_t M>
constexpr auto exp_head(fx<N> (&c)[M]) noexcept -> void
{
    static_assert(First + Step * (M - 1U) < sizeof(fx_table::inv_factorial) / sizeof(fx_table::inv_factorial[0]), "1/n! is in the table");

    for (std::size_t n {}; n < M; ++n)
    {
        c[n] = fx_top<N>(fx_table::inv_factorial[First + Step * n]);
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

// exp(-z) as above in two words with the error below 2^-124: terms to z^6 in two words, and z^7 to z^11 in one
constexpr auto exp_neg_series_precise(const fx<2>& z) noexcept -> fx<2>
{
    fx<2> head[7] {};
    exp_head(head);
    return fx_alternating_split<fx_table::exp_d128_fast_shift>(z, head, fx_table::exp_d128_fast_tail);
}

template <int N>
constexpr auto exp_neg_series_of(const fx<N>& z, std::false_type) noexcept -> fx<N>
{
    return exp_neg_series(z);
}

constexpr auto exp_neg_series_of(const fx<2>& z, std::true_type) noexcept -> fx<2>
{
    return exp_neg_series_precise(z);
}

// exp(-x) is below half of the smallest subnormal value and exp(x), sinh(x) and cosh(x) overflow for |x|
// at or above this bound, and the same for exp2(x) with Base2. As sig has p digits, |x| < bound gives e < 0.
template <typename T, bool Base2 = false>
constexpr auto exp_bound() noexcept -> std::uint64_t
{
    // log2(10) and ln(10) times 10^6, rounded up
    const auto c {Base2 ? UINT64_C(3321929) : UINT64_C(2302586)};
    return static_cast<std::uint64_t>(std::numeric_limits<T>::max_exponent10 + std::numeric_limits<T>::digits10 + 1) * c / UINT64_C(1000000) + 1U;
}

// The integer part of |x| = sig * 10^-k, or a value above each bound for k <= 0, in 64 bits where sig fits
constexpr auto exp_whole(const boost::int128::uint128_t& sig, const int k) noexcept -> boost::int128::uint128_t
{
    if (k > 0 && k < 20 && sig.high == 0U)
    {
        return sig.low / pow10(static_cast<std::uint64_t>(k));
    }
    return k <= 0 ? boost::int128::uint128_t {UINT64_MAX} : k < 39 ? sig / pow10(static_cast<boost::int128::uint128_t>(k)) : boost::int128::uint128_t {};
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

// expm1(x) / x = 1 + x R for x = frac, or 1 - x R for x = -frac with neg, where R = even(x^2) + x odd(x^2) and both
// series start from 1/2! and 1/3!. Returns R, and the ratio in s.
template <int words, std::size_t terms>
constexpr auto expm1_rest(const fx<words>& frac, const bool neg, fx<words>& s) noexcept -> fx<words>
{
    fx<words> even[terms - 1U] {};
    fx<words> odd[terms - 1U] {};
    exp_head<2U, 2U>(even);
    exp_head<3U, 2U>(odd);
    const auto u {fx_mul(frac, frac)};
    const auto h {fx_horner(u, even)};
    const auto o {fx_mul(frac, fx_horner(u, odd))};
    const auto r {neg ? fx_sub(h, o) : fx_add(h, o)};
    s = neg ? fx_sub(fx_one<words>(), fx_mul(frac, r)) : fx_add(fx_one<words>(), fx_mul(frac, r));
    return r;
}

// y = exp10 + f with 0 <= f < 1
template <int words>
struct exp_parts
{
    fx<words> f;
    int exp10;
};

// y = |x| * c for |x| = whole + frac, with c = log10(e) or log10(2). In one word, the error of f is below 3.5 units
// of 2^-62.
template <int words>
constexpr auto exp_log10(const std::uint64_t whole, const fx<words>& frac, const fx<4>& c) noexcept -> exp_parts<words>
{
    // whole * c + frac * c in words + 2 words, scaled by 2^(64 words + 62)
    std::uint64_t y[static_cast<std::size_t>(words + 2)] {};
    const auto carry {fx_mul_add(fx_make_span<words + 1>(y), fx_make_span<words + 1>(c.w, 3 - words), whole)};
    y[words + 1] = carry;
    const auto low {fx_mul(frac, fx_top<words>(c))};
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

// 10^f = 2^(a + i/16 + j/256) * exp(-v ln(2)) for 0 <= f <= 1 and 0 < v <= 1/256. In one word, m has a relative error
// below 6.4 units of 2^-62 plus ln(10) times the error of f.
template <int words, bool Precise = false>
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
    const auto z {fx_mul(v, fx_top<words>(fx_table::ln2))};
    return {fx_mul(coarse, exp_neg_series_of(z, std::integral_constant<bool, Precise> {})), a};
}

// For decimal32_t and decimal64_t, m * s * 10^e10 for an exact m and s with an error below err units of 2^-62, rounded
// once in out where it cannot be next to a rounding boundary: 19 digits from m * s with m * s in [10^17, 10^20)
template <typename T>
constexpr auto exp_try_round(boost::int128::uint128_t m, const fx<1>& s, int e10, const bool neg, const std::uint64_t err, T& out) noexcept -> bool
{
    using sig_type = boost::int128::uint128_t;

    auto d {fx_scale(m, s)};
    if (d < pow10(static_cast<sig_type>(18)))
    {
        m *= 10U;
        --e10;
        d = fx_scale(m, s);
    }
    auto w {((m * err) >> 62U).low + 2U};
    if (d >= pow10(static_cast<sig_type>(19)))
    {
        d /= 10U;
        ++e10;
        w = w / 10U + 2U;
    }
    return fx_try_round(d.low, e10, neg, w, out);
}

// The same for sig * 10^e * s, where sig has the digits of T: for decimal128_t, sig stays as it is and no try rounds
template <typename T>
constexpr auto exp_try_scale(const boost::int128::uint128_t& sig, const int e, const fx<1>& s, const bool neg, const std::uint64_t err, T& out) noexcept -> bool
{
    constexpr int up {std::numeric_limits<T>::digits10 < 19 ? 19 - std::numeric_limits<T>::digits10 : 0};
    return exp_try_round(sig * pow10(static_cast<boost::int128::uint128_t>(up)), s, e - up, neg, err, out);
}

// -y = (-exp10 - 1) + (1 - f) for y = exp10 + f with 0 <= f < 1
template <int words>
BOOST_DECIMAL_FORCE_INLINE constexpr auto exp_negate(int& exp10, fx<words>& f) noexcept -> void
{
    exp10 = -exp10 - 1;
    f = fx_sub(fx_one<words>(), f);
}

// y = |x| * c for |x| = sig * 10^-k with the integer part whole, or -y = (-exp10 - 1) + (1 - f) for neg
template <int words>
constexpr auto exp_log10_signed(const boost::int128::uint128_t& sig, const int k, const std::uint64_t whole, const fx<4>& c, const bool neg) noexcept -> exp_parts<words>
{
    auto y {exp_log10(whole, exp_frac<words>(sig, k, whole), c)};
    if (neg)
    {
        exp_negate(y.exp10, y.f);
    }
    return y;
}

// 10^f = 2^a * m as scale10 + 1 digits: m / 2 < 1 for fx_scale, and 10^scale10 * 2^(a + 1) * m / 2 is 10^f
template <int words>
constexpr auto exp10_digits(const exp_mantissa<words>& t, const int scale10) noexcept -> boost::int128::uint128_t
{
    return fx_scale(pow10(static_cast<boost::int128::uint128_t>(scale10)) << (t.a + 1U), fx_shr(t.m, 1U));
}

// For |x| = sig * 10^-k with sig below 10^16 and 12 <= k <= 32, exp10 + f = x c with c = log10(e) or log10(2), from
// one product with c * 10^-k from the table: f log2(10) = a + i/16 + j/256 - v, and the result is 256 v ln(2)
template <bool Base2, bool Narrow>
BOOST_DECIMAL_FORCE_INLINE constexpr auto exp_word_reduce(const std::uint64_t sig, const int k, const bool neg, int& exp10, unsigned& a, unsigned& i, unsigned& j) noexcept -> fx<1>
{
    using wide = boost::int128::uint128_t;
    const auto& c {(Base2 ? fx_table::log10_2_pow10_bin : fx_table::log10_e_pow10_bin)[k - 12]};

    // q = sig * c: q[3] is the integer part, and q[2] and q[1] are the fraction f. Narrow keeps 64 bits of f and of
    // log2(10), which is sufficient for decimal32_t.
    std::uint64_t q[4] {};
    std::uint64_t carry {};
    for (int n {Narrow ? 1 : 0}; n < 3; ++n)
    {
        q[n] = mul_add(sig, c[2 - n], 0U, carry);
    }
    wide f {q[2], Narrow ? 0U : q[1]};
    exp10 = static_cast<int>(carry);
    if (neg)
    {
        f = Narrow ? wide {~f.high, 0U} : ~f;
        exp10 = -exp10 - 1;
    }

    const auto t {Narrow ? wide {f.high} * fx_table::log2_10.w[3] : fx_mul_high(f, wide {fx_table::log2_10.w[3], fx_table::log2_10.w[2]})};
    a = static_cast<unsigned>(t.high >> 62U);
    i = static_cast<unsigned>((t.high >> 58U) & 15U);
    j = static_cast<unsigned>((t.high >> 54U) & 15U) + 1U;
    const auto v {wide {UINT64_C(1) << 54U, 0U} - wide {t.high & ((UINT64_C(1) << 54U) - 1U), t.low}};
    return fx_mul(fx<1> {{(v.high << 8U) | (v.low >> 56U)}}, fx_top<1>(fx_table::ln2));
}

// The 64 fraction bits of 2^(i/16 + j/256), which is below 2
BOOST_DECIMAL_FORCE_INLINE constexpr auto exp_word_coarse(const std::size_t i, const std::size_t j) noexcept -> std::uint64_t
{
    const auto& p {fx_table::pow2[i]};
    const auto& r {fx_table::pow2_fine[j]};
    const std::uint64_t p1 {(p.w[2] << 2U) | (p.w[1] >> 62U)};
    const std::uint64_t r1 {(r.w[2] << 2U) | (r.w[1] >> 62U)};
    std::uint64_t high {};
    boost::int128::detail::umul(p1, r1, high);
    return p1 + r1 + high;
}

// (1 + coarse / 2^64) e / 64, the change of the fraction bits coarse for the factor 1 -+ e / (256 * 2^62)
BOOST_DECIMAL_FORCE_INLINE constexpr auto exp_word_step(const std::uint64_t coarse, const std::uint64_t e) noexcept -> std::uint64_t
{
    std::uint64_t high {};
    boost::int128::detail::umul(coarse, e, high);
    return (e + high) >> 6U;
}

// The 64 fraction bits m of 2^(i/16 + j/256) exp(-z) = 1 + m / 2^64 in [1, 2), for 256 z from exp_word_reduce. With
// Narrow, the series stops at z^3 and m has a relative error below 2^-38.
template <bool Narrow>
BOOST_DECIMAL_FORCE_INLINE constexpr auto exp_word_fraction(const fx<1>& z, const std::size_t i, const std::size_t j) noexcept -> std::uint64_t
{
    // e = 256 (1 - exp(-z)) = 256 z (1 - z/2! + z^2/3! - ...): the larger scale keeps the error of 1 - exp(-z)
    // below 2^-68
    fx<1> head[Narrow ? 3 : 7] {};
    exp_head<1U, 1U>(head);
    const auto e {fx_mul(z, fx_alternating(fx<1> {{z.w[0] >> 8U}}, head)).w[0]};

    const auto coarse {exp_word_coarse(i, j)};
    const auto cut {exp_word_step(coarse, e)};
    return cut < coarse ? coarse - cut : 0U;
}

// The error bound in units of the 19 digits from exp_word_reduce: full for decimal64_t, and 2^25 for decimal32_t,
// which takes Narrow
template <typename T>
constexpr auto exp_word_bound(const std::uint64_t full) noexcept -> std::uint64_t
{
    return std::numeric_limits<T>::digits10 < 16 ? UINT64_C(1) << 25U : full;
}

// The 19 digits of 10^f = 2^a 2^(i/16) 2^(j/256) exp(-z) from exp_word_reduce, with an error in (-1.4, 1.6) units, or
// below 2^25 units with Narrow
template <bool Base2, bool Narrow>
constexpr auto exp_word_digits(const std::uint64_t sig, const int k, const bool neg, int& exp10) noexcept -> std::uint64_t
{
    unsigned a {};
    unsigned i {};
    unsigned j {};
    const auto z {exp_word_reduce<Base2, Narrow>(sig, k, neg, exp10, a, i, j)};

    const auto m {exp_word_fraction<Narrow>(z, i, j)};

    // 10^18 2^a (1 + m / 2^64), and 1 more to center the error
    const std::uint64_t base {pow10(static_cast<std::uint64_t>(18)) << a};
    std::uint64_t high {};
    boost::int128::detail::umul(base, m, high);
    return base + high + 1U;
}

// A value in (1 - 10^-(scale10 + 1), 1), rounded once in the current mode with the sign neg
template <typename T, typename digits_type>
constexpr auto just_below_one(const int scale10, const bool neg) noexcept -> T
{
    return fx_round<T>(static_cast<digits_type>(pow10(static_cast<boost::int128::uint128_t>(scale10 + 1)) - 1U), -scale10 - 1, neg);
}

// Whether the 38 or 39 digits d of exp from words words, below 2 units off in three words and below d * 2^-119 + 2 in
// two, can round to another value than the exact one: for decimal64_t the boundaries are multiples of 5 * 10^21
template <int words>
constexpr auto exp_near_boundary(const boost::int128::uint128_t& d) noexcept -> bool
{
    if (fx_is(words == 3))
    {
        return fx_near_boundary(d, 2U);
    }
    const auto unit {pow10(static_cast<boost::int128::uint128_t>(21)) * 5U};
    const auto rest {d % unit};
    const auto w {(d >> 119U) + 2U};
    return rest < w || rest > unit - w;
}

// exp_fixed past its first try in one word, for |x| = sig * 10^-k
template <typename T, bool Base2>
constexpr auto exp_wide_stage(const boost::int128::uint128_t& sig, const int k, const bool neg) noexcept -> T
{
    constexpr int p {std::numeric_limits<T>::digits10};
    constexpr int words {fx_words(p)};

    // One word gives 19 digits of the result, and two or three words give 38
    constexpr int scale10 {fx_scale10(words)};
    using digits_type = fx_digits_t<words>;

    const auto wide_whole {exp_whole(sig, k)};
    const auto whole {static_cast<std::uint64_t>(wide_whole)};
    if (wide_whole >= exp_bound<T, Base2>())
    {
        return neg ? T {1, -2 * std::numeric_limits<T>::max_exponent10} : T {1, 2 * std::numeric_limits<T>::max_exponent10};
    }
    const auto& c {Base2 ? fx_table::log10_2 : fx_table::log10_e};

    BOOST_DECIMAL_IF_CONSTEXPR (fx_is(words == 3))
    {
        // A first try in two words: its error is below 46 units of 2^-126, so below d2 * 2^-119 + 2 units of d2
        const auto y2 {exp_log10_signed<2>(sig, k, whole, c, neg)};
        const auto d2 {exp10_digits(exp10_frac<2, true>(y2.f), fx_scale10(2))};
        if (!fx_near_boundary(d2, static_cast<std::uint64_t>(d2 >> 119U) + 2U))
        {
            return fx_round<T>(static_cast<digits_type>(d2), y2.exp10 - fx_scale10(2), false);
        }
    }

    // Two words take the precise series: next to a tie, 1 + x + x^2 / 2 for a small x needs 32 digits
    const auto y {exp_log10_signed<words>(sig, k, whole, c, neg)};
    const auto d {exp10_digits(exp10_frac<words, (words == 2)>(y.f), scale10)};
    BOOST_DECIMAL_IF_CONSTEXPR (fx_is(words > 1))
    {
        // Next to a rounding boundary for a small x, exp(x) = 1 + x S with S = expm1(x) / x, and 2^x = 1 + x ln(2) S at
        // x ln(2): the rest past 1 takes its own precision, with 1 as 10^37, or 10^38 below 1
        if (whole == 0U && exp_near_boundary<words>(d))
        {
            auto frac {exp_frac<words>(sig, k, whole)};
            if (Base2)
            {
                frac = fx_mul(frac, fx_top<words>(fx_table::ln2));
            }
            if (exp_is_small(whole, frac))
            {
                fx<words> s {};
                static_cast<void>(expm1_rest<words, exp_small_terms(words)>(frac, neg, s));
                // The products truncate, so S is below its value for x > 0, and 32 units less keep it below for x < 0:
                // exp(x) is off 1 + v by v^3 / 3 only for x = v - v^2 / 2, and so the rest stays on its side of v
                fx<words> low {};
                low.w[0] = neg ? 32U : 0U;
                int a {};
                auto g {fx_mul(fx_lead_t<words>(sig, k, a), fx_sub(s, low))};
                if (Base2)
                {
                    g = fx_mul(g, fx_top<words>(fx_table::ln2));
                }
                const auto one {pow10(static_cast<boost::int128::uint128_t>(neg ? 38 : 37))};
                return fx_round<T>(static_cast<digits_type>(fx_lead_digits(one, g, a, neg)), neg ? -38 : -37, false);
            }
        }
    }
    return fx_round<T>(static_cast<digits_type>(d), y.exp10 - scale10, false);
}

// The same out of line, for two and three words: the first try in one word then keeps no state of that part
template <typename T, bool Base2>
BOOST_DECIMAL_NO_INLINE constexpr auto exp_wide_far(const boost::int128::uint128_t sig, const int k, const bool neg) noexcept -> T
{
    return exp_wide_stage<T, Base2>(sig, k, neg);
}

// exp(x), or exp2(x) with Base2, for a finite x != 0 where the result is not exact, rounded once in
// the current mode. With y = x * log10(e) or x * log10(2) = k + f and 0 <= f < 1, the result is 10^k * 10^f.
template <typename T, bool Base2 = false>
constexpr auto exp_fixed(const T x) noexcept -> T
{
    constexpr int p {std::numeric_limits<T>::digits10};
    constexpr int words {fx_words(p)};
    constexpr int scale10 {fx_scale10(words)};
    using digits_type = fx_digits_t<words>;

    int e {};
    const auto sig {static_cast<boost::int128::uint128_t>(frexp10(x, &e))};
    const bool neg {signbit(x)};

    const int k {-e};

    // For |x| < 10^-(p + 1), exp(x) rounds as a value just above or just below 1
    if (k >= 2 * p + 1)
    {
        return neg ? just_below_one<T, digits_type>(scale10, false)
                   : fx_round<T>(static_cast<digits_type>(pow10(static_cast<boost::int128::uint128_t>(scale10))), -scale10, false);
    }

    BOOST_DECIMAL_IF_CONSTEXPR (fx_is(words < 3))
    {
        // A first try in one word for |x| = s1 * 10^-k1 below the bound, with s1 of 16 digits: for k1 > 15, 10^-k1 is
        // below it. The bound of the digits is 2 units, or 2^25 units from the shorter series for decimal32_t.
        constexpr int up {p < 16 ? 16 - p : 0};
        const int k1 {k + up};
        const auto s1 {sig.low * pow10(static_cast<std::uint64_t>(up))};
        if (k1 >= 12 && k1 <= 32 && (k1 > 15 || s1 < exp_bound<T, Base2>() * pow10(static_cast<std::uint64_t>(k1))))
        {
            int e1 {};
            const auto d1 {exp_word_digits<Base2, (p < 16)>(s1, k1, neg, e1)};
            T r {};
            if (fx_try_round(d1, e1 - fx_scale10(1), false, exp_word_bound<T>(2U), r))
            {
                return r;
            }
        }
    }

    return fx_is(words > 1) ? exp_wide_far<T, Base2>(sig, k, neg) : exp_wide_stage<T, Base2>(sig, k, neg);
}

} // namespace exp_detail
} // namespace detail
} // namespace decimal
} // namespace boost

#endif // BOOST_DECIMAL_DETAIL_CMATH_IMPL_EXP_IMPL_HPP
