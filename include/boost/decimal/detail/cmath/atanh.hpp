// Copyright 2023 Matt Borland
// Copyright 2023 Christopher Kormanyos
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_DECIMAL_DETAIL_CMATH_ATANH_HPP
#define BOOST_DECIMAL_DETAIL_CMATH_ATANH_HPP

#include <boost/decimal/fwd.hpp> // NOLINT(llvm-include-order)
#include <boost/decimal/detail/type_traits.hpp>
#include <boost/decimal/detail/cmath/impl/exp_impl.hpp>
#include <boost/decimal/detail/cmath/log.hpp>
#include <boost/decimal/detail/concepts.hpp>
#include <boost/decimal/detail/config.hpp>

#ifndef BOOST_DECIMAL_BUILD_MODULE
#include <cstdint>
#include <limits>
#include <type_traits>
#endif

namespace boost {
namespace decimal {

namespace detail {

namespace log_detail {

// atanh(x) = ln(a / b) / 2 for 1/256 <= |x| < 1 with the integers a = 10^-e + sig and b = 10^-e - sig, as the
// scale 10^e cancels. With a = va * 2^ba and b = vb * 2^bb for va and vb in [1, 2), a / b = (va / vb) * 2^(ba - bb).
template <int N>
constexpr auto atanh_ln(const boost::int128::uint128_t& sig, const int e) noexcept -> fx_wide<N>
{
    const auto one {pow10(static_cast<boost::int128::uint128_t>(-e))};
    int ba {};
    int bb {};
    const auto va {log_normalize<N>(one + sig, ba)};
    const auto vb {log_normalize<N>(one - sig, bb)};
    auto v {fx_div(va, vb)};
    int n2 {ba - bb};
    if (v.w[N - 1] < (UINT64_C(1) << 62U))
    {
        v = fx_shl(v, 1U);
        --n2;
    }
    auto r {log_wide_of(v, n2, 0, log_base::e)};

    // r / 2, for r > 0
    for (int i {}; i < N; ++i)
    {
        r.w[i] = (r.w[i] >> 1U) | (r.w[i + 1] << 63U);
    }
    r.w[N] >>= 1U;
    return r;
}

// atanh of a finite x != 0, rounded once in the current mode
template <typename T>
constexpr auto atanh_fixed(const T x) noexcept -> T
{
    constexpr int p {std::numeric_limits<T>::digits10};
    constexpr int words {fx_words(p)};

    int e {};
    const auto sig {static_cast<boost::int128::uint128_t>(frexp10(x, &e))};
    const bool neg {signbit(x)};

    // A tiny x: atanh(x) is next above |x|
    if (e + p <= -fx_tiny_digits(p))
    {
        return fx_round_next_to_x<T>(sig, e, neg, true);
    }

    // |x| >= 1 has e + p > 0: atanh(+-1) = +-infinity, and NaN above
    if (e + p > 0)
    {
        #ifndef BOOST_DECIMAL_FAST_MATH
        if (e + p == 1 && sig == pow10(static_cast<boost::int128::uint128_t>(p - 1)))
        {
            return neg ? -std::numeric_limits<T>::infinity() : std::numeric_limits<T>::infinity();
        }
        return std::numeric_limits<T>::quiet_NaN();
        #else
        return T {0};
        #endif
    }

    // atanh(x) = x (1 + x^2 R(x^2)) with R = 1/3 + x^2/5 + ... for |x| < 0.1
    T series_r {};
    if (exp_detail::odd_series_small(sig, e, neg, fx_table::inv_odd, series_r))
    {
        return series_r;
    }

    BOOST_DECIMAL_IF_CONSTEXPR (fx_is(words > 1))
    {
        // A first try in one word fewer for |x| >= 0.1, or 1/64 in two words, below 128 units of its last bit, 32 for
        // |x| < 0.1, or in (-5.6, 0] units of 2^-62 in one word, where all steps truncate: 3 units more make it below 4
        T r {};
        if ((words == 3 ? !decimal_tiny(sig, e, 64U) : decimal_at_least(sig, e, p, 10U, 0))
            && fx_wide_try_round(log_center(atanh_ln<words == 3 ? 2 : 1>(sig, e), words == 3 ? 0U : 3U), neg, r, words == 3 ? (e + p < 0 ? 5U : 7U) : 2U))
        {
            return r;
        }
    }
    return fx_wide_round<T, words>(atanh_ln<words>(sig, e), neg);
}

} // namespace log_detail

template <typename T>
constexpr auto atanh_impl(const T x) noexcept
    BOOST_DECIMAL_REQUIRES(detail::is_decimal_floating_point_v, T)
{
    T result { };

    const auto fpc {fpclassify(x)};

    if (fpc != FP_NORMAL && fpc != FP_SUBNORMAL)
    {
        // An infinity is outside [-1, 1]
        result = isinf(x) ? std::numeric_limits<T>::quiet_NaN() : (issignaling(x) ? nan_conversion(x) : x);
    }
    else
    {
        result = log_detail::atanh_fixed(x);
    }

    return result;
}

} // namespace detail

BOOST_DECIMAL_EXPORT template <typename T>
constexpr auto atanh(const T x) noexcept
    BOOST_DECIMAL_REQUIRES(detail::is_decimal_floating_point_v, T)
{
    using evaluation_type = detail::evaluation_type_t<T>;

    return static_cast<T>(detail::atanh_impl(static_cast<evaluation_type>(x)));
}

} // namespace decimal
} // namespace boost

#endif // BOOST_DECIMAL_DETAIL_CMATH_ATANH_HPP
