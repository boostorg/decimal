// Copyright 2023 Matt Borland
// Copyright 2023 Christopher Kormanyos
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_DECIMAL_DETAIL_CMATH_EXPM1_HPP
#define BOOST_DECIMAL_DETAIL_CMATH_EXPM1_HPP

#include <boost/decimal/fwd.hpp> // NOLINT(llvm-include-order)
#include <boost/decimal/detail/cmath/exp.hpp>
#include <boost/decimal/detail/concepts.hpp>
#include <boost/decimal/detail/check_non_finite.hpp>
#include <boost/decimal/detail/config.hpp>
#include <boost/decimal/detail/type_traits.hpp>

#ifndef BOOST_DECIMAL_BUILD_MODULE
#include <cmath>
#include <type_traits>
#endif

namespace boost {
namespace decimal {

namespace detail {

namespace exp_detail {

// For a small |x|, expm1(x) = sinh(x) + cosh(x) - 1 = x * (S(x^2) +- |x| * C(x^2)) by the Taylor series
// S of sinh(x) / x and C of (cosh(x) - 1) / x^2, without the x
template <int words, std::size_t terms>
constexpr auto expm1_small(const fx<words>& frac, const bool neg) noexcept -> fx<words>
{
    fx<words> s[terms] {};
    fx<words> c[terms - 1U] {};
    exp_head<1U, 2U>(s);
    exp_head<2U, 2U>(c);
    const auto u {fx_mul(frac, frac)};
    const auto odd {fx_horner(u, s)};
    const auto even {fx_mul(frac, fx_horner(u, c))};
    return neg ? fx_sub(odd, even) : fx_add(odd, even);
}

// For x > 0, exp(x) - 1 = 10^exp10 * 8d with d = 10^f / 8 - 125 * 10^-(exp10 + 3), and for x < 0, 1 - exp(x) = d with
// exp(x) = 10^-(exp10 + 1) * 10^(1 - f), for y = |x| log10(e) = exp10 + f
template <int words, bool Precise = false>
BOOST_DECIMAL_FORCE_INLINE constexpr auto expm1_large(const exp_parts<words>& y, const bool neg) noexcept -> fx<words>
{
    if (!neg)
    {
        // Where 10^-exp10 is below the last bit, the 1 has no effect
        const auto r {exp10_frac<words, Precise>(y.f)};
        auto d {fx_shr(r.m, 3U - r.a)};
        if (y.exp10 < 19 * words)
        {
            d = fx_sub(d, fx_from_decimal<words>(125U, y.exp10 + 3));
        }
        return d;
    }
    const auto r {exp10_frac<words, Precise>(fx_sub(fx_one<words>(), y.f))};
    return fx_sub(fx_one<words>(), fx_mul(fx_shr(r.m, 3U - r.a), fx_from_decimal<words>(8U, y.exp10 + 1)));
}

// For decimal64_t, expm1(x) in one word, rounded once in out where it cannot be next to a rounding boundary. The series
// takes 10 terms in S and 9 in C.
template <typename T>
constexpr auto expm1_try(const boost::int128::uint128_t& sig, const int e, const std::uint64_t whole, const bool neg, T& out) noexcept -> bool
{
    using sig_type = boost::int128::uint128_t;

    const auto frac {exp_frac<1>(sig, -e, whole)};
    const bool half {whole == 0U && frac.w[0] < (UINT64_C(1) << 61U)};
    if (half || (whole == 0U && !neg))
    {
        // Products truncate: the error is in [-2.1, 2.4] units of 2^-62 for x < 0, and (-4.4, 0], (-8.3, 0] or
        // (-38.3, 0] for 0 < x below 1/2, 3/4 or 1, so c centers it
        const bool below {half || frac.w[0] < (UINT64_C(3) << 60U)};
        const fx<1> c {{neg ? 0U : half ? 2U : below ? 4U : 19U}};
        return exp_try_round(sig * 1000U, fx_add(expm1_small<1, 10U>(frac, neg), c), e - 3, neg, half ? 3U : below ? 5U : 20U, out);
    }

    const auto y {exp_log10(whole, frac, fx_table::log10_e)};
    if (!neg)
    {
        // x >= 1: all steps truncate, so 10^f has a relative error in (-13.5, 2.1] units of 2^-62 and d in
        // (-21.3, 3.2] with exp(x) below 1.59 (exp(x) - 1), plus (-1, 1) units: 9 units more center it
        const auto d {expm1_large(y, false)};
        const bool below {d.w[0] < (UINT64_C(1) << 59U)};
        const sig_type m {pow10(static_cast<sig_type>(below ? 19 : 18)) * 8U};
        auto digits {fx_scale(m, d)};
        digits += (digits * 9U) >> 62U;
        const auto w {((digits * 25U) >> 63U) + (m >> 62U) + 4U};
        return fx_try_round(digits.low, y.exp10 - (below ? 19 : 18), false, w.low, out);
    }

    // x <= -1/2: 10^(1 - f) has a relative error in (-5.4, 10.1] units of 2^-62 and t = exp(x) in t (-5.4, 10.1]
    // + (-3.1, 0], so 1 - t in t [-10.1, 5.4) + [0, 3.1): 2.375 t units of 2^-62 more and 2 digits less center it
    if (y.exp10 + 1 >= 19)
    {
        // exp(x) is below 10^-19, thus expm1(x) is as close to -1 for 16 digits
        constexpr int words {fx_words(std::numeric_limits<T>::digits10)};
        out = just_below_one<T, fx_digits_t<words>>(fx_scale10(words), true);
        return true;
    }
    auto digits {fx_scale(pow10(static_cast<sig_type>(19)), expm1_large(y, true))};
    const auto q {pow10(static_cast<sig_type>(19)) - digits};
    digits += ((q * 19U) >> 65U) - 2U;
    return fx_try_round(digits.low, -19, true, ((q * 63U) >> 65U).low + 6U, out);
}

// For decimal128_t and |x| >= 1/2, expm1(x) in two words with the precise series of exp, rounded once in out where it
// cannot be next to a rounding boundary. Below 1/2, two words of the series are not faster than three.
template <typename T>
constexpr auto expm1_precise_try(const boost::int128::uint128_t& sig, const int e, const std::uint64_t whole, const bool neg, T& out) noexcept -> bool
{
    using sig_type = boost::int128::uint128_t;
    constexpr int words {fx_words(std::numeric_limits<T>::digits10)};
    using digits_type = fx_digits_t<words>;
    constexpr int scale10 {fx_scale10(2)};

    const auto frac {exp_frac<2>(sig, -e, whole)};

    // 10^f has a relative error below 46 units of 2^-126. For x >= 1/2, 8d is above 0.39 * 10^f, thus its error is
    // below 150 units of 8d. For x <= -1/2, 1 - t is above 0.39 with an error below 46 t + 3 units.
    const auto y {exp_log10(whole, frac, fx_table::log10_e)};
    if (!neg)
    {
        const auto d {fx_scale(pow10(static_cast<sig_type>(scale10)) * 8U, expm1_large<2, true>(y, false))};
        if (fx_near_boundary(d, static_cast<std::uint64_t>((d >> 119U) + (d >> 120U)) + 2U))
        {
            return false;
        }
        out = fx_round<T>(static_cast<digits_type>(d), y.exp10 - scale10, false);
        return true;
    }
    if (y.exp10 + 1 >= 38)
    {
        out = just_below_one<T, digits_type>(fx_scale10(words), true);
        return true;
    }
    const auto d {fx_scale(pow10(static_cast<sig_type>(scale10 + 1)), expm1_large<2, true>(y, true))};
    if (fx_near_boundary(d, static_cast<std::uint64_t>(d >> 119U) + 2U))
    {
        return false;
    }
    out = fx_round<T>(static_cast<digits_type>(d), -scale10 - 1, true);
    return true;
}

// expm1(x) = exp(x) - 1 for a finite x != 0, rounded once in the current mode
template <typename T>
constexpr auto expm1_fixed(const T x) noexcept -> T
{
    constexpr int p {std::numeric_limits<T>::digits10};
    constexpr int words {fx_words(p)};
    constexpr int scale10 {fx_scale10(words)};
    using digits_type = fx_digits_t<words>;

    int e {};
    const auto sig {static_cast<boost::int128::uint128_t>(frexp10(x, &e))};
    const bool neg {signbit(x)};

    // A tiny x: for |x| < 10^-p, expm1(x) = x + x^2 / 2 + ... is next above x
    if (e + p <= -p)
    {
        return fx_round_next_to_x<T>(sig, e, neg, !neg);
    }

    const int k {-e};
    const auto wide_whole {exp_whole(sig, k)};
    const auto whole {static_cast<std::uint64_t>(wide_whole)};
    if (wide_whole >= exp_bound<T>())
    {
        return neg ? just_below_one<T, digits_type>(scale10, true) : T {1U, 2 * std::numeric_limits<T>::max_exponent10};
    }
    BOOST_DECIMAL_IF_CONSTEXPR (fx_is(words == 2))
    {
        T r {};
        if (expm1_try(sig, e, whole, neg, r))
        {
            return r;
        }
    }
    BOOST_DECIMAL_IF_CONSTEXPR (fx_is(words == 3))
    {
        T r {};
        if (decimal_at_least(sig, e, p, 50U, 0) && expm1_precise_try(sig, e, whole, neg, r))
        {
            return r;
        }
    }
    const auto frac {exp_frac<words>(sig, k, whole)};

    if (exp_is_small(whole, frac))
    {
        const auto m {sig * pow10(static_cast<boost::int128::uint128_t>(scale10 - p))};
        BOOST_DECIMAL_IF_CONSTEXPR (fx_is(words == 3))
        {
            // A first try in two words: its error is below 64 units of 2^-126, so below d2 * 2^-119 + 2 units of d2
            const auto d2 {fx_scale(m, expm1_small<2, 10U>(exp_frac<2>(sig, k, whole), neg))};
            if (d2 >= pow10(static_cast<boost::int128::uint128_t>(fx_scale10(2) - 1)) && !fx_near_boundary(d2, static_cast<std::uint64_t>(d2 >> 119U) + 2U))
            {
                return fx_round<T>(static_cast<digits_type>(d2), e - (scale10 - p), neg);
            }
        }

        // For x < 0, |expm1(x)| < |x| = m * 10^e, thus the digits are below m also where the sum rounds to 1
        const auto digits {fx_scale(m, expm1_small<words, exp_small_terms(words)>(frac, neg))};
        BOOST_DECIMAL_IF_CONSTEXPR (fx_is(words == 3))
        {
            // Next to a rounding boundary, expm1(x) = m (1 + x R) and the rest t R 10^-a takes its own precision
            if (fx_near_boundary(digits, 2U))
            {
                fx<words> s {};
                const auto r {expm1_rest<words, exp_small_terms(words)>(frac, neg, s)};
                int a {};
                const auto g {fx_mul(fx_lead_t<words>(sig, k, a), r)};
                return fx_round<T>(static_cast<digits_type>(fx_lead_digits(m, g, a, neg)), e - (scale10 - p), neg);
            }
        }
        return fx_round<T>(static_cast<digits_type>(neg && digits >= m ? m - 1U : digits), e - (scale10 - p), neg);
    }

    const auto y {exp_log10(whole, frac, fx_table::log10_e)};
    if (!neg)
    {
        return fx_round<T>(static_cast<digits_type>(fx_scale(pow10(static_cast<boost::int128::uint128_t>(scale10)) * 8U, expm1_large(y, false))), y.exp10 - scale10, false);
    }

    // Where exp(x) is below 10^-(19 words), expm1(x) is closer to -1 than the rounding can see
    if (y.exp10 + 1 >= 19 * words)
    {
        return just_below_one<T, digits_type>(scale10, true);
    }
    BOOST_DECIMAL_IF_CONSTEXPR (fx_is(words == 3))
    {
        // For exp(x) = 10^(1 - f) 10^-(exp10 + 1) below 10^-12, the values of x next to each other give results closer
        // than three words tell apart: exp(x) = g 10^-(exp10 - 1) takes its own precision
        if (y.exp10 >= 12)
        {
            const auto r {exp10_frac<words>(fx_sub(fx_one<words>(), y.f))};
            const auto g {fx_mul(fx_shr(r.m, 3U - r.a), fx_from_decimal<words>(8U, 2))};
            return fx_round<T>(static_cast<digits_type>(fx_lead_digits(pow10(static_cast<boost::int128::uint128_t>(scale10)), g, y.exp10 - 1, true)), -scale10, true);
        }
    }
    return fx_round<T>(static_cast<digits_type>(fx_scale(pow10(static_cast<boost::int128::uint128_t>(scale10 + 1)), expm1_large(y, true))), -scale10 - 1, true);
}

} // namespace exp_detail

template <typename T>
constexpr auto expm1_impl(const T x) noexcept
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
            if (signbit(x))
            {
                result = -T {1, 0};
            }
            else
            {
                result = x;
            }
        }
        else if (fpc == FP_NAN)
        {
            result = detail::check_non_finite(x);
        }
    }
    #endif
    else
    {
        result = exp_detail::expm1_fixed(x);
    }

    return result;
}

} // namespace detail

BOOST_DECIMAL_EXPORT template <typename T>
constexpr auto expm1(const T x) noexcept
    BOOST_DECIMAL_REQUIRES(detail::is_decimal_floating_point_v, T)
{
    using evaluation_type = detail::evaluation_type_t<T>;

    return static_cast<T>(detail::expm1_impl(static_cast<evaluation_type>(x)));
}

} // namespace decimal
} // namespace boost

#endif // BOOST_DECIMAL_DETAIL_CMATH_EXPM1_HPP
