// Copyright 2023 - 2024 Matt Borland
// Copyright 2023 - 2024 Christopher Kormanyos
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_DECIMAL_DETAIL_CMATH_SINH_HPP
#define BOOST_DECIMAL_DETAIL_CMATH_SINH_HPP

#include <boost/decimal/fwd.hpp> // NOLINT(llvm-include-order)
#include <boost/decimal/detail/cmath/exp.hpp>
#include <boost/decimal/detail/concepts.hpp>
#include <boost/decimal/detail/check_non_finite.hpp>
#include <boost/decimal/detail/config.hpp>
#include <boost/decimal/detail/type_traits.hpp>

#ifndef BOOST_DECIMAL_BUILD_MODULE
#include <type_traits>
#endif

namespace boost {
namespace decimal {

namespace detail {

namespace exp_detail {

// The Taylor series of sinh(x) / x or cosh(x) in x^2
template <int words, std::size_t terms>
constexpr auto sinh_cosh_series(const fx<words>& frac, const bool is_cosh) noexcept -> fx<words>
{
    fx<words> c[terms] {};
    if (is_cosh)
    {
        exp_head<0U, 2U>(c);
    }
    else
    {
        exp_head<1U, 2U>(c);
    }
    return fx_horner(fx_mul(frac, frac), c);
}

// The digits of sinh(x) or cosh(x) = m (1 + x^2 R(x^2)) next to a rounding boundary, with x = t * 10^-a for t in
// [0.1, 1): the rest x^2 R = t^2 R 10^-2a takes its own precision
template <int words, std::size_t terms>
constexpr auto sinh_cosh_lead(const boost::int128::uint128_t& sig, const int k, const fx<words>& frac, const boost::int128::uint128_t& m, const bool is_cosh) noexcept -> boost::int128::uint128_t
{
    fx<words> c[terms - 1U] {};
    if (is_cosh)
    {
        exp_head<2U, 2U>(c);
    }
    else
    {
        exp_head<3U, 2U>(c);
    }
    int a {};
    const auto t {fx_lead_t<words>(sig, k, a)};
    const auto g {fx_mul(fx_mul(t, t), fx_horner(fx_mul(frac, frac), c))};
    return fx_lead_digits(m, g, 2 * a, false);
}

// (exp(|x|) +- exp(-|x|)) / 2 = 10^exp10 * 4d for |x| = whole + frac, with exp10 and d in the parts
template <int words, bool Precise = false>
BOOST_DECIMAL_FORCE_INLINE constexpr auto sinh_cosh_parts(const std::uint64_t whole, const fx<words>& frac, const bool is_cosh) noexcept -> exp_parts<words>
{
    // exp(|x|) = 10^exp10 * 10^f and exp(-|x|) = 10^-gap * 10^exp10 * 10^(1 - f) with gap = 2 exp10 + 1.
    // In d, each 10^f is divided by 8 so that it is below 4, and 10^(1 - f) / 8 = 10 / (8 * 2^a * m) = 5/4 * 2^-a / m.
    const auto y {exp_log10(whole, frac, fx_table::log10_e)};
    const auto big {exp10_frac<words, Precise>(y.f)};
    auto d {fx_shr(big.m, 3U - big.a)};

    // Where 10^-gap is below the last bit of d, exp(-|x|) has no effect
    const int gap {2 * y.exp10 + 1};
    if (gap < 19 * words)
    {
        const auto small {fx_mul_shr(fx_recip(big.m), 5U, 2U)};
        const auto low {fx_mul(fx_shr(small, big.a), fx_from_decimal<words>(1U, gap))};
        d = is_cosh ? fx_add(d, low) : fx_sub(d, low);
    }
    return {d, y.exp10};
}

// The 19 digits of sinh(|x|) or cosh(x) = 10^exp10 2^(a - 1) (M -+ 10^(-2 exp10) 2^(-2 a) / M) for |x| = sig * 10^-k:
// M and 1 / M are from exp_word_reduce and the same tables. The error is in (-2.3, 2.3) units for sinh from 1. With
// Narrow, the series stop at z^3 with the same relative error, and the error is below 2^25 units for sinh from 0.1.
template <bool Narrow>
constexpr auto sinh_cosh_word_digits(const std::uint64_t sig, const int k, const bool is_cosh, int& exp10) noexcept -> std::uint64_t
{
    using wide = boost::int128::uint128_t;

    unsigned a {};
    unsigned i {};
    unsigned j {};
    const auto z {exp_word_reduce<false, Narrow>(sig, k, false, exp10, a, i, j)};

    // 256 (1 - exp(-z)) = 256 z (even - z odd) and 256 (exp(z) - 1) = 256 z (even + z odd), with the series in z^2
    fx<1> ce[Narrow ? 2 : 4] {};
    fx<1> co[Narrow ? 1 : 3] {};
    exp_head<1U, 2U>(ce);
    exp_head<2U, 2U>(co);
    const fx<1> small {{z.w[0] >> 8U}};
    const auto u {fx_mul(small, small)};
    const auto even {fx_horner(u, ce)};
    const auto odd {fx_mul(small, fx_horner(u, co))};

    // M = 1 + m / 2^64 in [1, 2), and 2 / M = 1 + inv / 2^64 in (1, 2]
    const auto coarse {exp_word_coarse(i, j)};
    const auto cut {exp_word_step(coarse, fx_mul(z, fx_sub(even, odd)).w[0])};
    wide q {1U, cut < coarse ? coarse - cut : 0U};
    if (exp10 < 10)
    {
        const auto back {exp_word_coarse(15U - i, 16U - j)};
        const auto inv {back + exp_word_step(back, fx_mul(z, fx_add(even, odd)).w[0])};
        const auto t {fx_table::half_pow100_inv[exp10]};
        std::uint64_t high {};
        boost::int128::detail::umul(t, inv < back ? UINT64_MAX : inv, high);
        const wide low {0U, (t + high) >> (2U * a + 6U * static_cast<unsigned>(exp10))};
        q = is_cosh ? q + low : q - low;
    }

    // 10^18 2^(a - 1) q / 2^64, or 10^19 for a value below 1
    const bool below {a == 0U ? q.high < 2U : a == 1U && q.high < 1U};
    const std::uint64_t base {(pow10(static_cast<std::uint64_t>(below ? 19 : 18)) >> 1U) << a};
    std::uint64_t high {};
    boost::int128::detail::umul(base, q.low, high);
    exp10 -= below ? 19 : 18;
    return base * q.high + high + 1U;
}

// For |x| = frac < 1, sinh(|x|) = x S(x^2) with 10 terms of S, and cosh(x) below 1/16, in one word, rounded once in out
// where it cannot be next to a rounding boundary
template <typename T>
constexpr auto sinh_cosh_try(const boost::int128::uint128_t& sig, const int e, const fx<1>& frac, const bool is_cosh, const bool neg, T& out) noexcept -> bool
{
    using sig_type = boost::int128::uint128_t;

    if (is_cosh && !exp_is_small(0U, frac))
    {
        return false;
    }

    // cosh takes the 5 terms below, with an error below 2^-60. Products truncate, so S has an error in (-2.1, 0],
    // (-4, 0] or (-18.7, 0] units of 2^-62 for |x| below 1/2, 3/4 or 1
    const bool half {frac.w[0] < (UINT64_C(1) << 61U)};
    const bool below {half || frac.w[0] < (UINT64_C(3) << 60U)};
    const fx<1> c {{half ? 1U : below ? 2U : 9U}};
    return is_cosh ? exp_try_round(pow10(static_cast<sig_type>(18)), sinh_cosh_series<1, 5U>(frac, true), -18, neg, 4U, out)
                   : exp_try_scale(sig, e, fx_add(sinh_cosh_series<1, 10U>(frac, false), c), neg, half ? 2U : below ? 3U : 10U, out);
}

// For decimal128_t and |x| >= 1/16, sinh(|x|) or cosh(x) in two words with the precise series of exp, rounded once in
// out where it cannot be next to a rounding boundary. Below 1, sinh(x) = x S(x^2) with 17 terms of S.
template <typename T>
constexpr auto sinh_cosh_precise_try(const boost::int128::uint128_t& sig, const int e, const std::uint64_t whole, const bool is_cosh, const bool neg, T& out) noexcept -> bool
{
    using sig_type = boost::int128::uint128_t;
    using digits_type = fx_digits_t<fx_words(std::numeric_limits<T>::digits10)>;
    constexpr int scale10 {fx_scale10(2)};
    constexpr int up {scale10 - std::numeric_limits<T>::digits10};

    const auto frac {exp_frac<2>(sig, -e, whole)};
    if (exp_is_small(whole, frac))
    {
        return false;
    }
    if (whole == 0U && !is_cosh)
    {
        // S has an error below 34 units of 2^-126, so the digits below d * 2^-120 + 2 units
        const auto d {fx_scale(sig * pow10(static_cast<sig_type>(up)), sinh_cosh_series<2, 17U>(frac, false))};
        if (fx_near_boundary(d, static_cast<std::uint64_t>(d >> 120U) + 2U))
        {
            return false;
        }
        out = fx_round<T>(static_cast<digits_type>(d), e - up, neg);
        return true;
    }

    // 10^f has a relative error below 46 units of 2^-126, and 4d below 110 for cosh and for sinh with |x| >= 1
    const auto r {sinh_cosh_parts<2, true>(whole, frac, is_cosh)};
    const auto d {fx_scale(pow10(static_cast<sig_type>(scale10)) * 4U, r.f)};
    if (fx_near_boundary(d, static_cast<std::uint64_t>(d >> 119U) + 2U))
    {
        return false;
    }
    out = fx_round<T>(static_cast<digits_type>(d), r.exp10 - scale10, neg);
    return true;
}

// sinh(|x|) for is_cosh false and cosh(x) for is_cosh true, for a finite x != 0, rounded once in the
// current mode with the sign neg
template <typename T>
constexpr auto sinh_cosh_fixed(const T x, const bool is_cosh, const bool neg) noexcept -> T
{
    constexpr int p {std::numeric_limits<T>::digits10};
    constexpr int words {fx_words(p)};
    constexpr int scale10 {fx_scale10(words)};
    using digits_type = fx_digits_t<words>;

    int e {};
    const auto sig {static_cast<boost::int128::uint128_t>(frexp10(x, &e))};

    // A tiny x: sinh(x) is next above |x|, and cosh(x) next above 1
    if (e + p <= -fx_tiny_digits(p))
    {
        return is_cosh ? fx_round_next_to_x<T>(pow10(static_cast<boost::int128::uint128_t>(p - 1)), 1 - p, false, true) : fx_round_next_to_x<T>(sig, e, neg, true);
    }

    const int k {-e};
    BOOST_DECIMAL_IF_CONSTEXPR (fx_is(words < 3))
    {
        // A first try for |x| >= 1, for cosh from 1/16 and for decimal32_t sinh from 0.1, below the overflow bound:
        // sig takes 16 digits
        constexpr int up {p < 16 ? 16 - p : 0};
        const int k1 {k + up};
        const auto s1 {sig.low * pow10(static_cast<std::uint64_t>(up))};
        if (k1 >= 12 && (k1 > 15 ? (is_cosh ? k1 == 16 || (k1 == 17 && s1 >= UINT64_C(6250000000000000)) : fx_is(p < 16) && k1 == 16) : s1 < exp_bound<T>() * pow10(static_cast<std::uint64_t>(k1))))
        {
            int e1 {};
            const auto d1 {sinh_cosh_word_digits<(p < 16)>(s1, k1, is_cosh, e1)};
            T r {};
            if (fx_try_round(d1, e1, neg, exp_word_bound<T>(3U), r))
            {
                return r;
            }
        }
    }
    const auto wide_whole {exp_whole(sig, k)};
    const auto whole {static_cast<std::uint64_t>(wide_whole)};
    if (wide_whole >= exp_bound<T>())
    {
        return T {1U, 2 * std::numeric_limits<T>::max_exponent10, neg};
    }
    BOOST_DECIMAL_IF_CONSTEXPR (fx_is(words == 2))
    {
        T r {};
        if (whole == 0U && sinh_cosh_try(sig, e, exp_frac<1>(sig, k, whole), is_cosh, neg, r))
        {
            return r;
        }
    }
    BOOST_DECIMAL_IF_CONSTEXPR (fx_is(words == 3))
    {
        T r {};
        if (sinh_cosh_precise_try(sig, e, whole, is_cosh, neg, r))
        {
            return r;
        }
    }
    const auto frac {exp_frac<words>(sig, k, whole)};
    BOOST_DECIMAL_IF_CONSTEXPR (fx_is(words == 1))
    {
        T r {};
        if (whole == 0U && !is_cosh && !exp_is_small(whole, frac) && sinh_cosh_try(sig, e, fx_top<1>(frac), is_cosh, neg, r))
        {
            return r;
        }
    }

    // For |x| < 1/16, the Taylor series sinh(x) = x * (1 + x^2/3! + ...) and cosh(x) = 1 + x^2/2! + ...
    // The error is below 2^-60, 2^-100 and 2^-160 for one, two and three words.
    if (exp_is_small(whole, frac))
    {
        const auto m {is_cosh ? pow10(static_cast<boost::int128::uint128_t>(scale10)) : sig * pow10(static_cast<boost::int128::uint128_t>(scale10 - p))};
        const int exp10 {is_cosh ? -scale10 : e - (scale10 - p)};
        BOOST_DECIMAL_IF_CONSTEXPR (fx_is(words == 3))
        {
            // A first try in two words where |x| >= 10^-16, else the digits are next to m: its error is
            // below 64 units of 2^-126, so below d2 * 2^-119 + 2 units of d2
            if (e + p > -16)
            {
                const auto d2 {fx_scale(m, sinh_cosh_series<2, 10U>(exp_frac<2>(sig, k, whole), is_cosh))};
                if (!fx_near_boundary(d2, static_cast<std::uint64_t>(d2 >> 119U) + 2U))
                {
                    return fx_round<T>(static_cast<digits_type>(d2), exp10, neg);
                }
            }
        }
        const auto d {fx_scale(m, sinh_cosh_series<words, exp_small_terms(words)>(frac, is_cosh))};
        BOOST_DECIMAL_IF_CONSTEXPR (fx_is(words == 3))
        {
            if (fx_near_boundary(d, 2U))
            {
                return fx_round<T>(static_cast<digits_type>(sinh_cosh_lead<words, exp_small_terms(words)>(sig, k, frac, m, is_cosh)), exp10, neg);
            }
        }
        return fx_round<T>(static_cast<digits_type>(d), exp10, neg);
    }

    const auto r {sinh_cosh_parts(whole, frac, is_cosh)};
    return fx_round<T>(static_cast<digits_type>(fx_scale(pow10(static_cast<boost::int128::uint128_t>(scale10)) * 4U, r.f)), r.exp10 - scale10, neg);
}

} // namespace exp_detail

template <typename T>
constexpr auto sinh_impl(const T x) noexcept
    BOOST_DECIMAL_REQUIRES(detail::is_decimal_floating_point_v, T)
{
    const auto fpc = fpclassify(x);

    constexpr T zero { 0, 0 };

    auto result = zero;

    if (fpc == FP_ZERO)
    {
        result = x;
    }
    #ifndef BOOST_DECIMAL_FAST_MATH
    else if (fpc != FP_NORMAL && fpc != FP_SUBNORMAL)
    {
        if (fpc == FP_INFINITE)
        {
            result = x;
        }
        else if (fpc == FP_NAN)
        {
            result = detail::check_non_finite(x);
        }
    }
    #endif
    else
    {
        result = exp_detail::sinh_cosh_fixed(x, false, signbit(x));
    }

    return result;
}

} // namespace detail

BOOST_DECIMAL_EXPORT template <typename T>
constexpr auto sinh(const T x) noexcept
    BOOST_DECIMAL_REQUIRES(detail::is_decimal_floating_point_v, T)
{
    using evaluation_type = detail::evaluation_type_t<T>;

    return static_cast<T>(detail::sinh_impl(static_cast<evaluation_type>(x)));
}

} // namespace decimal
} // namespace boost

#endif // BOOST_DECIMAL_DETAIL_CMATH_SINH_HPP
