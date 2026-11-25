// Copyright 2023 Matt Borland
// Copyright 2023 - 2026 Christopher Kormanyos
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_DECIMAL_DETAIL_CMATH_ASINH_HPP
#define BOOST_DECIMAL_DETAIL_CMATH_ASINH_HPP

#include <boost/decimal/fwd.hpp> // NOLINT(llvm-include-order)
#include <boost/decimal/detail/type_traits.hpp>
#include <boost/decimal/detail/cmath/impl/exp_impl.hpp>
#include <boost/decimal/detail/cmath/log.hpp>
#include <boost/decimal/detail/concepts.hpp>

#ifndef BOOST_DECIMAL_BUILD_MODULE
#include <limits>
#include <type_traits>
#endif

namespace boost {
namespace decimal {

namespace detail {

namespace log_detail {

// asinh(x) / x = 1 - t/6 + 3t^2/40 - ... for t = x^2 < 2^-16, to below 2^-69, 2^-133 and 2^-197 in one, two
// and three words. The terms from t^4 and t^8 on are below 2^-64 and 2^-128.
template <int N>
constexpr auto asinh_series(const fx<N>& t) noexcept -> fx<N>
{
    return fx_series<true, N == 1 ? 4U : N == 2 ? 8U : 12U, 4U, 8U>(t, fx_table::asinh_coeff);
}

// asinh(x) = ln(|x| + sqrt(x^2 + 1)) for |x| >= 1/256. Below 1, it is ln(u + sqrt(u^2 + 1)) with u = |x|. Else
// |x| = u * 2^-s * 10^n10 with u in [0.5, 1), and it is ln((u + sqrt(u^2 + 4^s * 10^(-2 n10))) * 2^-s * 10^n10).
template <int N>
constexpr auto asinh_ln(const boost::int128::uint128_t& sig, const int e, const int p) noexcept -> fx_wide<N>
{
    if (e + p <= 0)
    {
        const auto u {fx_from_decimal<N>(sig, -e)};
        return log_wide_of(fx_add(u, fx_sqrt(fx_add(fx_one<N>(), fx_mul(u, u)))), 0, 0);
    }
    return log_root_sum<N, false>(sig, p, e + p);
}

// asinh of a finite x != 0, rounded once in the current mode
template <typename T>
constexpr auto asinh_fixed(const T x) noexcept -> T
{
    constexpr int p {std::numeric_limits<T>::digits10};
    constexpr int words {fx_words(p)};

    int e {};
    const auto sig {static_cast<boost::int128::uint128_t>(frexp10(x, &e))};
    const bool neg {signbit(x)};

    // A tiny x: asinh(x) is next below |x|
    if (e + p <= -fx_tiny_digits(p))
    {
        return fx_round_next_to_x<T>(sig, e, neg, false);
    }

    BOOST_DECIMAL_IF_CONSTEXPR (fx_is(words <= 2))
    {
        // A first try in one word for |x| < 0.11, from 10 terms above 1/256: the series is 2 units of 2^-62 off
        if (!decimal_at_least(sig, e, p, 11U, 0) && (words == 2 || !decimal_tiny(sig, e)))
        {
            const auto u {fx_from_decimal<1>(sig, -e)};
            const auto t {fx_mul(u, u)};
            const auto s {decimal_tiny(sig, e) ? asinh_series(t) : fx_series<true, 10U, 4U, 8U>(t, fx_table::asinh_coeff)};
            T r {};
            if (exp_detail::exp_try_scale(sig, e, s, neg, 3U, r))
            {
                return r;
            }
        }
    }
    BOOST_DECIMAL_IF_CONSTEXPR (fx_is(words == 3))
    {
        // The same in two words for |x| < 1/64, with 16 units for an error of 2 units of 2^-126 in the series
        if (decimal_tiny(sig, e, 64U))
        {
            const auto u {fx_from_decimal<2>(sig, -e)};
            T r {};
            if (fx_try_scale2(sig, e, fx_series<true, 11U, 6U, 11U>(fx_mul(u, u), fx_table::asinh_coeff), neg, 16U, r))
            {
                return r;
            }
        }
    }
    // For |x| < 1/256, asinh(x) = x * asinh(x) / x < |x|, also where the series rounds to 1
    if (decimal_tiny(sig, e))
    {
        const auto u {fx_from_decimal<words>(sig, -e)};
        const auto z {fx_mul(u, u)};
        const auto s {asinh_series(z)};
        BOOST_DECIMAL_IF_CONSTEXPR (fx_is(words == 3))
        {
            // Next to a rounding boundary, asinh(x) = m (1 - x^2 R) with R = 1/6 - 3 x^2/40 + ..., and the rest t^2 R
            // 10^-2a takes its own precision
            constexpr int shift {fx_scale10(words) + 1 - p};
            const auto m {sig * pow10(static_cast<boost::int128::uint128_t>(shift))};
            if (fx_near_boundary(fx_scale(m, s), 2U))
            {
                int a {};
                const auto t {fx_lead_t<words>(sig, -e, a)};
                const auto g {fx_mul(fx_mul(t, t), fx_horner_range<true>(z, fx_table::asinh_coeff, 1U, 12U, fx<words> {}))};
                return fx_round<T>(static_cast<fx_digits_t<words>>(fx_lead_digits(m, g, 2 * a, true)), e - shift, neg);
            }
        }
        return fx_round_below_x<T>(sig, e, s, neg);
    }

    BOOST_DECIMAL_IF_CONSTEXPR (fx_is(words > 1))
    {
        // A first try in one word fewer for |x| >= 0.2 below 256 units of its last bit, or 1/64 <= |x| < 0.1 below 32,
        // and in one word for |x| >= 0.11 in (-10.8, 0.6) units of 2^-62 below 1 and in (-16, 4.4) from 1 on
        T r {};
        const bool near_zero {words == 3 && e + p < 0 && !decimal_tiny(sig, e, 64U)};
        if ((near_zero || decimal_at_least(sig, e, p, words == 3 ? 20U : 11U, 0))
            && fx_wide_try_round(asinh_ln<words == 3 ? 2 : 1>(sig, e, p), neg, r, near_zero ? 5U : words == 3 ? 8U : 4U))
        {
            return r;
        }
    }
    return fx_wide_round<T, words>(asinh_ln<words>(sig, e, p), neg);
}

} // namespace log_detail

template <typename T>
constexpr auto asinh_impl(const T x) noexcept
    BOOST_DECIMAL_REQUIRES(detail::is_decimal_floating_point_v, T)
{
    T result { };

    const auto fpc {fpclassify(x)};

    if (fpc != FP_NORMAL && fpc != FP_SUBNORMAL)
    {
        result = issignaling(x) ? nan_conversion(x) : x;
    }
    else
    {
        result = log_detail::asinh_fixed(x);
    }

    return result;
}

} // namespace detail

BOOST_DECIMAL_EXPORT template <typename T>
constexpr auto asinh(const T x) noexcept
    BOOST_DECIMAL_REQUIRES(detail::is_decimal_floating_point_v, T)
{
    using evaluation_type = detail::evaluation_type_t<T>;

    return static_cast<T>(detail::asinh_impl(static_cast<evaluation_type>(x)));
}

} // namespace decimal
} // namespace boost

#endif // BOOST_DECIMAL_DETAIL_CMATH_ASINH_HPP
