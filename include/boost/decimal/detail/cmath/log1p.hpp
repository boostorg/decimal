// Copyright 2023 Matt Borland
// Copyright 2023 Christopher Kormanyos
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_DECIMAL_DETAIL_CMATH_LOG1P_HPP
#define BOOST_DECIMAL_DETAIL_CMATH_LOG1P_HPP

#include <boost/decimal/fwd.hpp> // NOLINT(llvm-include-order)
#include <boost/decimal/detail/cmath/impl/exp_impl.hpp>
#include <boost/decimal/detail/cmath/log.hpp>
#include <boost/decimal/detail/concepts.hpp>
#include <boost/decimal/detail/check_non_finite.hpp>
#include <boost/decimal/detail/config.hpp>
#include <boost/decimal/detail/type_traits.hpp>

#ifndef BOOST_DECIMAL_BUILD_MODULE
#include <limits>
#include <type_traits>
#endif

namespace boost {
namespace decimal {

namespace detail {

namespace log_detail {

// ln(1 + x) for |x| >= 1/256 and x > -1: 1 + x is exact with the exponent e for e < 0, and with the exponent 0 where
// it has at most 38 digits. Else ln(1 + x) = ln(x) + 1/x - ... with 1/x below 10^-38.
template <int N>
constexpr auto log1p_wide(const boost::int128::uint128_t& sig, const int e, const bool neg, const int p) noexcept -> fx_wide<N>
{
    if (e < 0)
    {
        const auto one {pow10(static_cast<boost::int128::uint128_t>(-e))};
        const auto y {neg ? one - sig : one + sig};
        return log_wide_of<N>(y, e, log_base::e);
    }
    if (e + p <= 38)
    {
        const auto y {sig * pow10(static_cast<boost::int128::uint128_t>(e)) + 1U};
        return log_wide_of<N>(y, 0, log_base::e);
    }
    // 1 / x = (1 / d) * 2^s * 10^-(e + p) for d = sig * 10^-p * 2^s in [1, 2), in all the words
    unsigned s {};
    const auto d {fx_normalize(fx_from_decimal<N>(sig, p), s)};
    const auto inv {fx_mul(fx_recip(d), fx_from_decimal<N>(1U << s, e + p))};
    return fx_wide_add(log_wide_of<N>(sig, e, log_base::e), fx_widen(inv));
}

// log1p of a finite x != 0, rounded once in the current mode
template <typename T>
constexpr auto log1p_fixed(const T x) noexcept -> T
{
    constexpr int p {std::numeric_limits<T>::digits10};
    constexpr int words {fx_words(p)};

    int e {};
    const auto sig {static_cast<boost::int128::uint128_t>(frexp10(x, &e))};
    const bool neg {signbit(x)};

    // A tiny x: for |x| < 10^-p, log1p(x) = x - x^2 / 2 + ... is next below x
    if (e + p <= -p)
    {
        return fx_round_next_to_x<T>(sig, e, neg, neg);
    }

    // x <= -1 has e + p > 0: log1p(-1) = -infinity, and NaN below
    if (neg && e + p > 0)
    {
        #ifndef BOOST_DECIMAL_FAST_MATH
        return e + p == 1 && sig == pow10(static_cast<boost::int128::uint128_t>(p - 1)) ? -std::numeric_limits<T>::infinity() : std::numeric_limits<T>::quiet_NaN();
        #else
        return T {0};
        #endif
    }
    // First tries for small |x|: log1p(x) = x R S(z^2) with R = 2 / (2 + x) and z = x R / 2, where S is atanh(z) / z.
    // R S is below 5 units of its last bit off in one word.
    BOOST_DECIMAL_IF_CONSTEXPR (fx_is(words == 2))
    {
        if (!decimal_at_least(sig, e, p, neg ? 10U : 11U, 0))
        {
            const auto v {fx_shr(fx_from_decimal<1>(sig, -e), 1U)};
            const auto r2 {fx_div(fx_one<1>(), neg ? fx_sub(fx_one<1>(), v) : fx_add(fx_one<1>(), v))};
            const auto z {fx_mul(v, r2)};
            const auto t {fx_mul(z, z)};
            T r {};
            if (exp_detail::exp_try_round(sig * 1000U, fx_mul(r2, decimal_tiny(sig, e) ? atanh_series(t) : fx_series<false, 8U, 4U, 8U>(t, fx_table::inv_odd)), e - 3, neg, 6U, r))
            {
                return r;
            }
        }
    }
    BOOST_DECIMAL_IF_CONSTEXPR (fx_is(words == 3))
    {
        // 1 / b in two words is below 2^-120 off, thus a window of 128 units
        if (e + p < 0 && decimal_tiny(sig, e, 64U))
        {
            const auto v {fx_shr(fx_from_decimal<2>(sig, -e), 1U)};
            const auto r2 {fx_div(fx_one<2>(), neg ? fx_sub(fx_one<2>(), v) : fx_add(fx_one<2>(), v))};
            const auto z {fx_mul(v, r2)};
            T r {};
            if (fx_try_scale2(sig, e, fx_mul(r2, fx_series<false, 9U, 4U, 8U>(fx_mul(z, z), fx_table::inv_odd)), neg, 128U, r))
            {
                return r;
            }
        }
    }
    if (decimal_tiny(sig, e))
    {
        return log_near<T>(sig, e, neg, log_base::e);
    }

    BOOST_DECIMAL_IF_CONSTEXPR (fx_is(words > 1))
    {
        // A first try in one word fewer for |x| >= 1/64 in two words and x >= 0.11 or x <= -0.1 in one word: below 64
        // units of its last bit, 32 for |x| < 0.1, or in [-9.1, 1.1] units of 2^-62 in one word: 4 more make it below 8
        T r {};
        if ((words == 3 ? !decimal_tiny(sig, e, 64U) : decimal_at_least(sig, e, p, neg ? 10U : 11U, 0))
            && fx_wide_try_round(log_center(log1p_wide<words == 3 ? 2 : 1>(sig, e, neg, p), words == 3 ? 0U : 4U), false, r, words == 3 ? (e + p < 0 ? 5U : 6U) : 3U))
        {
            return r;
        }
    }
    return fx_wide_round<T, words>(log1p_wide<words>(sig, e, neg, p), false);
}

} // namespace log_detail

template <typename T>
constexpr auto log1p_impl(const T x) noexcept
    BOOST_DECIMAL_REQUIRES(detail::is_decimal_floating_point_v, T)
{
    T result { };

    const auto fpc = fpclassify(x);

    if (fpc == FP_ZERO)
    {
        result = x;
    }
    #ifndef BOOST_DECIMAL_FAST_MATH
    else if (fpc != FP_NORMAL && fpc != FP_SUBNORMAL)
    {
        result =
        (
            ((fpc == FP_INFINITE) && signbit(x)) ? std::numeric_limits<T>::quiet_NaN() : detail::check_non_finite(x)
        );
    }
    #endif
    else
    {
        result = log_detail::log1p_fixed(x);
    }

    return result;
}

} // namespace detail

BOOST_DECIMAL_EXPORT template <typename T>
constexpr auto log1p(const T x) noexcept
    BOOST_DECIMAL_REQUIRES(detail::is_decimal_floating_point_v, T)
{
    using evaluation_type = detail::evaluation_type_t<T>;

    return static_cast<T>(detail::log1p_impl(static_cast<evaluation_type>(x)));
}

} // namespace decimal
} // namespace boost

#endif // BOOST_DECIMAL_DETAIL_CMATH_LOG1P_HPP
