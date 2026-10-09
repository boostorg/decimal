// Copyright 2023 - 2024 Matt Borland
// Copyright 2023 - 2024 Christopher Kormanyos
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_DECIMAL_DETAIL_CMATH_LOG_HPP
#define BOOST_DECIMAL_DETAIL_CMATH_LOG_HPP

#include <boost/decimal/fwd.hpp> // NOLINT(llvm-include-order)
#include <boost/decimal/detail/concepts.hpp>
#include <boost/decimal/detail/check_non_finite.hpp>
#include <boost/decimal/detail/config.hpp>
#include <boost/decimal/detail/type_traits.hpp>
#include <boost/decimal/detail/cmath/impl/fixed_point.hpp>
#include <boost/decimal/detail/cmath/frexp10.hpp>
#include <boost/decimal/detail/countl.hpp>
#include <boost/decimal/detail/integer_search_trees.hpp>

#ifndef BOOST_DECIMAL_BUILD_MODULE
#include <cmath>
#include <cstdint>
#include <limits>
#include <type_traits>
#endif

namespace boost {
namespace decimal {

namespace detail {

namespace log_detail {

enum class log_base { e, ten, two };

// n * c, with the top N + 1 words of c
template <int N>
constexpr auto log_mul(const fx<4>& c, const std::uint64_t n) noexcept -> fx_wide<N>
{
    std::uint64_t p[static_cast<std::size_t>(N + 2)] {};
    const auto carry {fx_mul_add(fx_make_span<N + 1>(p), fx_make_span<N + 1>(c.w, 3 - N), n)};
    p[N + 1] = carry;
    fx_wide<N> r {};
    for (int i {}; i <= N; ++i)
    {
        r.w[i] = p[i + 1];
    }
    return r;
}

// r plus c units of its last bit, which centers a one-sided error for fx_wide_try_round
template <int N>
constexpr auto log_center(const fx_wide<N>& r, const std::uint64_t c) noexcept -> fx_wide<N>
{
    fx_wide<N> d {};
    d.w[0] = c;
    return fx_wide_add(r, d);
}

// n * c for a small n, with the top N words of c
template <int N, int M>
constexpr auto log_mul_small(const fx<M>& c, const std::uint64_t n) noexcept -> fx_wide<N>
{
    fx_wide<N> r {};
    const auto t {fx_top<N>(c)};
    const auto carry {fx_mul_add(fx_make_span<N>(r.w), fx_make_span(t.w), n)};
    r.w[N] = carry;
    return r;
}

// atanh(z) / z = 1 + t/3 + t^2/5 + ... for t = z^2 < 2^-16. The first term left out is below 2^-67, 2^-132 and
// 2^-196 for one, two and three words, and the terms from t^4 and t^8 on are below 2^-64 and 2^-128.
template <int N>
constexpr auto atanh_series(const fx<N>& t) noexcept -> fx<N>
{
    return fx_series<false, N == 1 ? 4U : N == 2 ? 8U : 12U, 4U, 8U>(t, fx_table::inv_odd);
}

// |z| for z = w / (2 + w), with w = m, or w = -m for neg, where 0 <= m < 1/256
template <int N>
constexpr auto atanh_arg(const fx<N>& m, const bool neg) noexcept -> fx<N>
{
    if (!neg)
    {
        const auto h {fx_shr(m, 1U)};
        return fx_div(h, fx_add(fx_one<N>(), h));
    }
    return fx_div(m, fx_sub(fx_int<N>(2U), m));
}

// ln(1 + w) for 0 <= w < 1.004 * 2^-16: the series w - w^2/2 + ... to w^4/4, w^9/9 or w^12/12, without a division.
// The first term left out is below 2^-66, 2^-147 and 2^-195 of the sum.
template <int N>
constexpr auto log_one_plus(const fx<N>& w) noexcept -> fx<N>
{
    return fx_mul(w, fx_series<true, N == 1 ? 4U : N == 2 ? 9U : 12U, 4U, 8U>(w, fx_table::log_inv));
}

// ln(1 + w) / w for w = m, or for w = -m with neg, where 0 <= m < 1/256: in one word, the series 1 - w/2 + ...
// to w^8/9 without a division
constexpr auto log_ratio(const fx<1>& m, const bool neg) noexcept -> fx<1>
{
    return neg ? fx_series<false, 9U, 8U, 8U>(m, fx_table::log_inv) : fx_series<true, 9U, 8U, 8U>(m, fx_table::log_inv);
}

// In more words, (1 - z) atanh(z) / z with z = w / (2 + w)
template <int N>
constexpr auto log_ratio(const fx<N>& m, const bool neg) noexcept -> fx<N>
{
    const auto z {atanh_arg(m, neg)};
    return fx_mul(neg ? fx_add(fx_one<N>(), z) : fx_sub(fx_one<N>(), z), atanh_series(fx_mul(z, z)));
}

// log(v * 2^n2 * 10^n10) for 1 <= v < 2. Three steps v1 = v * r1 / 2^8, v2 = v1 * r2 / 2^16 and w + 1 = v2 * r3 / 2^24
// with short r1, r2 and r3 from tables give w < 1.004 * 2^-16, and ln(v) = ln(1 + w) - sum ln(r_i / 2^(8i)).
template <int N>
constexpr auto log_wide_of(const fx<N>& v, const int n2, const int n10, const log_base base) noexcept -> fx_wide<N>
{
    const auto i {static_cast<std::size_t>((v.w[N - 1] >> 58U) & 15U)};
    const auto v1 {fx_mul_shr(v, fx_table::log_r1[i], 8U)};
    const auto j {static_cast<std::size_t>((v1.w[N - 1] >> 54U) & 255U)};
    const auto v2 {fx_mul_shr(v1, fx_table::log_r2[j], 16U)};
    const auto k {static_cast<std::size_t>((v2.w[N - 1] >> 46U) & 255U)};
    auto w {fx_mul_shr(v2, fx_table::log_r3[k], 24U)};
    w.w[N - 1] &= ~(UINT64_C(1) << 62U);

    const auto l12 {fx_add(fx_top<N>(fx_table::log_l1[i]), fx_top<N>(fx_table::log_l2[j]))};
    const auto lv {fx_add(fx_add(l12, fx_top<N>(fx_table::log_l3[k])), log_one_plus(w))};

    const auto m2 {static_cast<std::uint64_t>(n2 < 0 ? -n2 : n2)};
    const auto m10 {static_cast<std::uint64_t>(n10 < 0 ? -n10 : n10)};
    fx_wide<N> r {};
    if (base == log_base::e)
    {
        r = fx_wide_add(fx_widen(lv), log_mul<N>(fx_table::ln2, m2), n2 < 0);
        return fx_wide_add(r, log_mul<N>(fx_table::ln10, m10), n10 < 0);
    }
    if (base == log_base::ten)
    {
        r = fx_wide_add(fx_widen(fx_mul(lv, fx_top<N>(fx_table::log10_e))), log_mul<N>(fx_table::log10_2, m2), n2 < 0);
        return fx_wide_add(r, fx_wide_int<N>(m10), n10 < 0);
    }
    r = fx_wide_add(fx_widen(fx_mul(lv, fx_top<N>(fx_table::log2_e))), fx_wide_int<N>(m2), n2 < 0);
    return fx_wide_add(r, log_mul<N>(fx_table::log2_10, m10), n10 < 0);
}

// v in [1, 2) with sig = v * 2^b for 0 < sig < 2^127, exact in two words and more, and in one word for sig < 2^63
template <int N>
constexpr auto log_normalize(const boost::int128::uint128_t& sig, int& b) noexcept -> fx<N>
{
    b = 127 - boost::int128::countl_zero(sig);
    const auto m {sig << static_cast<unsigned>(126 - b)};
    fx<N> v {};
    v.w[N - 1] = m.high;
    BOOST_DECIMAL_IF_CONSTEXPR (fx_is(N > 1))
    {
        v.w[N > 1 ? N - 2 : 0] = m.low;
    }
    return v;
}

// log(sig * 10^e) for 0 < sig < 2^127
template <int N>
constexpr auto log_wide_of(const boost::int128::uint128_t& sig, const int e, const log_base base) noexcept -> fx_wide<N>
{
    int b {};
    const auto v {log_normalize<N>(sig, b)};
    return log_wide_of(v, b, e, base);
}

// log(1 + w) for w = sig * 10^e with 0 < |w| < 1/256 and the sign neg, rounded once:
// ln(1 + w) = w * ln(1 + w) / w with the ratio from log_ratio.
template <typename T>
constexpr auto log_near(const boost::int128::uint128_t& sig, const int e, const bool neg, const log_base base) noexcept -> T
{
    constexpr int p {std::numeric_limits<T>::digits10};
    constexpr int words {fx_words(p)};
    constexpr int scale10 {fx_scale10(words)};
    using digits_type = fx_digits_t<words>;

    const auto v {fx_from_decimal<words>(sig, -e)};
    auto g {log_ratio(v, neg)};
    if (base == log_base::ten)
    {
        g = fx_mul(g, fx_top<words>(fx_table::log10_e));
    }
    else if (base == log_base::two)
    {
        g = fx_mul(g, fx_top<words>(fx_table::log2_e));
    }

    // m has scale10 + 1 digits where g < 1, else scale10
    const int len {g.w[words - 1] >= (UINT64_C(1) << 62U) ? scale10 : scale10 + 1};
    const int up {len - num_digits(sig)};
    const auto m {sig * pow10(static_cast<boost::int128::uint128_t>(up))};
    auto digits {fx_scale(m, g)};

    // ln(1 + w) < w for w > 0, also where g rounds to 1. For w < 0, g >= 1 gives digits >= m.
    if (base == log_base::e && !neg && digits >= m)
    {
        digits = m - 1U;
    }
    BOOST_DECIMAL_IF_CONSTEXPR (fx_is(words == 3))
    {
        // Near a rounding boundary, ln(1 + w) = m (1 -+ |w| R) for w >= 0 or w < 0 with z = |w| q, q = 1 / (2 +- |w|),
        // R = q -+ |w| q^2 A (1 -+ z) and A = 1/3 + z^2/5 + ...: the rest t R 10^-a takes its own precision
        if (base == log_base::e && fx_near_boundary(digits, 2U))
        {
            const auto q {fx_div(fx_one<words>(), neg ? fx_sub(fx_int<words>(2U), v) : fx_add(fx_int<words>(2U), v))};
            const auto z {fx_mul(v, q)};
            const auto a {fx_horner_range<false>(fx_mul(z, z), fx_table::inv_odd, 1U, 11U, fx<words> {})};
            const auto c {fx_mul(fx_mul(z, q), fx_mul(a, neg ? fx_add(fx_one<words>(), z) : fx_sub(fx_one<words>(), z)))};
            const auto r {neg ? fx_add(q, c) : fx_sub(q, c)};
            int s {};
            const auto t {fx_lead_t<words>(sig, -e, s)};
            return fx_round<T>(static_cast<digits_type>(fx_lead_digits(m, fx_mul(t, r), s, !neg)), e - up, neg);
        }
    }
    return fx_round<T>(static_cast<digits_type>(digits), e - up, neg);
}

// log of a finite x > 0, rounded once in the current mode
template <typename T>
constexpr auto log_fixed(const T x, const log_base base) noexcept -> T
{
    constexpr int p {std::numeric_limits<T>::digits10};
    constexpr int words {fx_words(p)};

    int e {};
    const auto sig {static_cast<boost::int128::uint128_t>(frexp10(x, &e))};

    // log10 of a power of 10 and log2 of a power of 2 are exact. A power of 2 is 2^c * 5^-e * 10^e,
    // thus sig is 2^c times 5^-e, which is 10^-e / 2^-e for -e <= 27 and has 19 digits at -e = 27.
    if (base == log_base::ten && sig == pow10(static_cast<boost::int128::uint128_t>(p - 1)))
    {
        return T {e + p - 1};
    }
    // From -e = 49 on, 5^-e > 10^34 > sig, and 5^55 still fits
    if (base == log_base::two && static_cast<unsigned>(-e) <= 55U)
    {
        const int c {boost::int128::countr_zero(sig)};
        // The low words of sig * 2^-e and 10^-e * 2^c differ for most x that are not a power of 2, at a low cost
        if (c >= 64 || e < -38 || (sig.low << static_cast<unsigned>(-e)) == (pow10(static_cast<boost::int128::uint128_t>(-e)).low << static_cast<unsigned>(c)))
        {
            const auto pow5 {e < -27 ? (pow10(static_cast<boost::int128::uint128_t>(27)) >> 27U) * (pow10(static_cast<boost::int128::uint128_t>(-e - 27)) >> static_cast<unsigned>(-e - 27))
                                     : pow10(static_cast<boost::int128::uint128_t>(-e)) >> static_cast<unsigned>(-e)};
            if ((sig >> static_cast<unsigned>(c)) == pow5)
            {
                return T {c + e};
            }
        }
    }

    // Near 1, x - 1 is exact with the exponent e of x
    if (e == -p || e == 1 - p)
    {
        const auto one {pow10(static_cast<boost::int128::uint128_t>(-e))};
        const bool below {sig < one};
        const auto d {below ? one - sig : sig - one};
        if (d == 0U)
        {
            return T {0};
        }
        if (decimal_tiny(d, e))
        {
            return log_near<T>(d, e, below, base);
        }
    }
    BOOST_DECIMAL_IF_CONSTEXPR (fx_is(words > 1))
    {
        // A first try in one word fewer for x outside [0.9, 1.11), outside [0.79, 1.3) for log10 in one word. One word
        // has an error in (-9.1, 1.1) units of 2^-62 for ln, (-5.8, 0] for log10 and (-13, 1.1) for log2, centered by c
        const bool near_one {(e == -p && sig >= pow10(static_cast<boost::int128::uint128_t>(p - 1)) * 9U)
                             || (e == 1 - p && sig < pow10(static_cast<boost::int128::uint128_t>(p - 3)) * 111U)};
        const bool far {words == 3 || base != log_base::ten ? !near_one : decimal_at_least(sig, e, p, 13U, 1) || !decimal_at_least(sig, e, p, 79U, 0)};
        T r {};
        const std::uint64_t c {words == 3 ? 0U : base == log_base::e ? 4U : base == log_base::ten ? 3U : 6U};
        if (far && fx_wide_try_round(log_center(log_wide_of<words == 3 ? 2 : 1>(sig, e, base), c), false, r, words == 3 ? 6U : base == log_base::ten ? 2U : 3U))
        {
            return r;
        }
    }
    return fx_wide_round<T, words>(log_wide_of<words>(sig, e, base), false);
}

} // namespace log_detail

template <typename T>
constexpr auto log_impl(const T x) noexcept
    BOOST_DECIMAL_REQUIRES(detail::is_decimal_floating_point_v, T)
{
    T result { };

    const auto fpc = fpclassify(x);

    if (fpc == FP_ZERO)
    {
        result = -std::numeric_limits<T>::infinity();
    }
    else if (fpc == FP_NAN)
    {
        result = detail::check_non_finite(x);
    }
    else if (signbit(x))
    {
        #ifndef BOOST_DECIMAL_FAST_MATH
        result = std::numeric_limits<T>::quiet_NaN();
        #else
        result = T{0};
        #endif
    }
    #ifndef BOOST_DECIMAL_FAST_MATH
    else if (fpc == FP_INFINITE)
    {
        result = std::numeric_limits<T>::infinity();
    }
    #endif
    else
    {
        result = log_detail::log_fixed(x, log_detail::log_base::e);
    }

    return result;
}

} //namespace detail

BOOST_DECIMAL_EXPORT template <typename T>
constexpr auto log(const T x) noexcept
    BOOST_DECIMAL_REQUIRES(detail::is_decimal_floating_point_v, T)
{
    using evaluation_type = detail::evaluation_type_t<T>;

    return static_cast<T>(detail::log_impl(static_cast<evaluation_type>(x)));
}

} // namespace decimal
} // namespace boost

#endif // BOOST_DECIMAL_DETAIL_CMATH_LOG_HPP
