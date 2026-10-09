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

// For |x| < 1/16, expm1(x) = sinh(x) + cosh(x) - 1 = x * (S(x^2) +- |x| * C(x^2)) by the Taylor series
// S of sinh(x) / x and C of (cosh(x) - 1) / x^2, scaled by m
template <int words, std::size_t terms>
constexpr auto expm1_small(const fx<words>& frac, const boost::int128::uint128_t& m, const bool neg) noexcept -> boost::int128::uint128_t
{
    fx<words> s[terms] {};
    fx<words> c[terms - 1U] {};
    exp_head<1U, 2U>(s);
    exp_head<2U, 2U>(c);
    const auto u {fx_mul(frac, frac)};
    const auto odd {fx_horner(u, s)};
    const auto even {fx_mul(frac, fx_horner(u, c))};
    return fx_scale(m, neg ? fx_sub(odd, even) : fx_add(odd, even));
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
    const auto wide_whole {exp_whole<T>(sig, k)};
    const auto whole {static_cast<std::uint64_t>(wide_whole)};
    if (wide_whole >= exp_bound<T>())
    {
        return neg ? just_below_one<T, digits_type>(scale10, true) : T {1U, 2 * std::numeric_limits<T>::max_exponent10};
    }
    const auto frac {exp_frac<words>(sig, k, whole)};

    if (exp_is_small(whole, frac))
    {
        const auto m {sig * pow10(static_cast<boost::int128::uint128_t>(scale10 - p))};
        BOOST_DECIMAL_IF_CONSTEXPR (fx_is(words == 3))
        {
            // A first try in two words: its error is below 64 units of 2^-126, so below d2 * 2^-119 + 2 units of d2
            const auto d2 {expm1_small<2, 10U>(exp_frac<2>(sig, k, whole), m, neg)};
            if (d2 >= pow10(static_cast<boost::int128::uint128_t>(fx_scale10(2) - 1)) && !fx_near_boundary(d2, static_cast<std::uint64_t>(d2 >> 119U) + 2U))
            {
                return fx_round<T>(static_cast<digits_type>(d2), e - (scale10 - p), neg);
            }
        }

        // For x < 0, |expm1(x)| < |x| = m * 10^e, thus the digits are below m also where the sum rounds to 1
        const auto digits {expm1_small<words, exp_small_terms(words)>(frac, m, neg)};
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

    const auto y {exp_log10(whole, frac)};
    if (!neg)
    {
        // exp(x) - 1 = 10^exp10 * 8 * (10^f / 8 - 125 * 10^-(exp10 + 3)). Where 10^-exp10 is below the last bit,
        // the 1 has no effect.
        const auto r {exp10_frac(y.f)};
        auto d {fx_shr(r.m, 3U - r.a)};
        if (y.exp10 < 19 * words)
        {
            d = fx_sub(d, fx_from_decimal<words>(125U, y.exp10 + 3));
        }
        return fx_round<T>(static_cast<digits_type>(fx_scale(pow10(static_cast<boost::int128::uint128_t>(scale10)) * 8U, d)), y.exp10 - scale10, false);
    }

    // exp(x) = 10^-(exp10 + 1) * 10^(1 - f) for x < 0. Where it is below 10^-(19 words), expm1(x) is
    // closer to -1 than the rounding can see.
    if (y.exp10 + 1 >= 19 * words)
    {
        return just_below_one<T, digits_type>(scale10, true);
    }
    const auto r {exp10_frac(fx_sub(fx_one<words>(), y.f))};
    BOOST_DECIMAL_IF_CONSTEXPR (fx_is(words == 3))
    {
        // For exp(x) below 10^-12, the values of x next to each other give results closer than three words tell
        // apart: exp(x) = g 10^-(exp10 - 1) takes its own precision
        if (y.exp10 >= 12)
        {
            const auto g {fx_mul(fx_shr(r.m, 3U - r.a), fx_from_decimal<words>(8U, 2))};
            return fx_round<T>(static_cast<digits_type>(fx_lead_digits(pow10(static_cast<boost::int128::uint128_t>(scale10)), g, y.exp10 - 1, true)), -scale10, true);
        }
    }
    const auto q {fx_sub(fx_one<words>(), fx_mul(fx_shr(r.m, 3U - r.a), fx_from_decimal<words>(8U, y.exp10 + 1)))};
    return fx_round<T>(static_cast<digits_type>(fx_scale(pow10(static_cast<boost::int128::uint128_t>(scale10 + 1)), q)), -scale10 - 1, true);
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
