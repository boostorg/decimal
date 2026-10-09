// Copyright 2023 - 2024 Matt Borland
// Copyright 2023 - 2024 Christopher Kormanyos
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_DECIMAL_DETAIL_CMATH_TANH_HPP
#define BOOST_DECIMAL_DETAIL_CMATH_TANH_HPP

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

// For |x| < 1/16, tanh(x) = x * (sinh(x) / x) / cosh(x) by the Taylor series of both in x^2, scaled by m
template <int words, std::size_t terms>
constexpr auto tanh_small(const fx<words>& frac, const boost::int128::uint128_t& m) noexcept -> boost::int128::uint128_t
{
    fx<words> s[terms] {};
    fx<words> c[terms] {};
    exp_head(s, 1U, 2U);
    exp_head(c, 0U, 2U);
    const auto u {fx_mul(frac, frac)};
    return fx_scale(m, fx_div(fx_horner(u, s), fx_horner(u, c)));
}

// The digits of tanh(x) = m (1 - x^2 R(x^2)) next to a rounding boundary, with x = t * 10^-a for t in [0.1, 1):
// R = sum 2k / (2k + 1)! x^(2k - 2) / cosh(x) and the rest t^2 R 10^-2a take their own precision
template <int words, std::size_t terms>
constexpr auto tanh_lead(const boost::int128::uint128_t& sig, const int k, const fx<words>& frac, const boost::int128::uint128_t& m) noexcept -> boost::int128::uint128_t
{
    fx<words> c[terms] {};
    fx<words> d[terms - 1U] {};
    fx<words> s[terms - 1U] {};
    exp_head(c, 0U, 2U);
    exp_head(s, 3U, 2U);
    for (std::size_t i {}; i < terms - 1U; ++i)
    {
        d[i] = fx_sub(c[i + 1U], s[i]);
    }
    const auto u {fx_mul(frac, frac)};
    int a {};
    const auto t {fx_lead_t<words>(sig, k, a)};
    const auto g {fx_mul(fx_mul(t, t), fx_div(fx_horner(u, d), fx_horner(u, c)))};
    return fx_lead_digits(m, g, 2 * a, true);
}

// tanh(|x|) for a finite x != 0, rounded once in the current mode with the sign neg
template <typename T>
constexpr auto tanh_fixed(const T x, const bool neg) noexcept -> T
{
    constexpr int p {std::numeric_limits<T>::digits10};
    constexpr int words {fx_words(p)};
    constexpr int scale10 {fx_scale10(words)};
    using digits_type = fx_digits_t<words>;

    int e {};
    const auto sig {static_cast<boost::int128::uint128_t>(frexp10(x, &e))};

    // A tiny x: tanh(x) is next below |x|
    if (e + p <= -fx_tiny_digits(p))
    {
        return fx_round_next_to_x<T>(sig, e, neg, false);
    }

    const int k {-e};
    const auto wide_whole {exp_whole<T>(sig, k)};
    const auto whole {static_cast<std::uint64_t>(wide_whole)};
    // From |x| >= 22 words, 2|x| log10(e) > 19 words: t is below the bound of the test after exp_log10
    if (wide_whole >= 22U * words)
    {
        return just_below_one<T, digits_type>(scale10, neg);
    }
    const auto frac {exp_frac<words>(sig, k, whole)};

    if (exp_is_small(whole, frac))
    {
        const auto m {sig * pow10(static_cast<boost::int128::uint128_t>(scale10 - p))};
        BOOST_DECIMAL_IF_CONSTEXPR (fx_is(words == 3))
        {
            // A first try in two words where |x| >= 10^-16, else the digits are next to m: its error is
            // below 128 units of 2^-126, so below d2 * 2^-118 + 2 units of d2
            if (e + p > -16)
            {
                const auto d2 {tanh_small<2, 10U>(exp_frac<2>(sig, k, whole), m)};
                if (d2 >= pow10(static_cast<boost::int128::uint128_t>(fx_scale10(2) - 1)) && !fx_near_boundary(d2, static_cast<std::uint64_t>(d2 >> 118U) + 2U))
                {
                    return fx_round<T>(static_cast<digits_type>(d2), e - (scale10 - p), neg);
                }
            }
        }

        // tanh(|x|) < |x| = m * 10^e, thus the digits are below m also where the quotient rounds to 1
        const auto digits {tanh_small<words, exp_small_terms(words)>(frac, m)};
        BOOST_DECIMAL_IF_CONSTEXPR (fx_is(words == 3))
        {
            if (fx_near_boundary(digits, 2U))
            {
                return fx_round<T>(static_cast<digits_type>(tanh_lead<words, exp_small_terms(words)>(sig, k, frac, m)), e - (scale10 - p), neg);
            }
        }
        return fx_round<T>(static_cast<digits_type>(digits < m ? digits : m - 1U), e - (scale10 - p), neg);
    }

    // tanh(|x|) = (1 - t) / (1 + t) with t = exp(-2|x|). With 2y = n + h and 0 <= h < 1,
    // t = 10^-(n + 1) * 10^(1 - h).
    const auto y {exp_log10(whole, frac)};
    auto h {fx_add(y.f, y.f)};
    int n {2 * y.exp10};
    // h >= 1
    if (h.w[words - 1] >= (UINT64_C(1) << 62U))
    {
        h = fx_sub(h, fx_one<words>());
        ++n;
    }

    // Where t is below 10^-(19 words), tanh(|x|) is closer to 1 than the rounding can see. Above,
    // 2t is larger than the error of 1 / (1 + t), thus q < 1.
    if (n + 1 >= 19 * words)
    {
        return just_below_one<T, digits_type>(scale10, neg);
    }
    const auto r {exp10_frac(fx_sub(fx_one<words>(), h))};
    const auto t {fx_mul(fx_shr(r.m, 3U - r.a), fx_from_decimal<words>(8U, n + 1))};
    BOOST_DECIMAL_IF_CONSTEXPR (fx_is(words == 3))
    {
        // For t below 10^-12, the values of x next to each other give results closer than three words tell apart:
        // 2t / (1 + t) = g 10^-(n - 1) takes its own precision
        if (n >= 12)
        {
            const auto g {fx_div(fx_mul(fx_shr(r.m, 3U - r.a), fx_from_decimal<words>(16U, 2)), fx_add(fx_one<words>(), t))};
            return fx_round<T>(static_cast<digits_type>(fx_lead_digits(pow10(static_cast<boost::int128::uint128_t>(scale10)), g, n - 1, true)), -scale10, neg);
        }
    }
    const auto q {fx_div(fx_sub(fx_one<words>(), t), fx_add(fx_one<words>(), t))};
    return fx_round<T>(static_cast<digits_type>(fx_scale(pow10(static_cast<boost::int128::uint128_t>(scale10 + 1)), q)), -scale10 - 1, neg);
}

} // namespace exp_detail

template <typename T>
constexpr auto tanh_impl(const T x) noexcept
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
                result = T {1, 0};
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
        result = exp_detail::tanh_fixed(x, signbit(x));
    }

    return result;
}

} // namespace detail

BOOST_DECIMAL_EXPORT template <typename T>
constexpr auto tanh(const T x) noexcept
    BOOST_DECIMAL_REQUIRES(detail::is_decimal_floating_point_v, T)
{
    using evaluation_type = detail::evaluation_type_t<T>;

    return static_cast<T>(detail::tanh_impl(static_cast<evaluation_type>(x)));
}

} // namespace decimal
} // namespace boost

#endif // BOOST_DECIMAL_DETAIL_CMATH_TANH_HPP
