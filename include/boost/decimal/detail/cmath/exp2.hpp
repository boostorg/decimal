// Copyright 2023 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_DECIMAL_DETAIL_CMATH_EXP2_HPP
#define BOOST_DECIMAL_DETAIL_CMATH_EXP2_HPP

#include <boost/decimal/detail/type_traits.hpp>
#include <boost/decimal/detail/concepts.hpp>
#include <boost/decimal/detail/check_non_finite.hpp>
#include <boost/decimal/detail/config.hpp>
#include <boost/decimal/detail/cmath/exp.hpp>
#include <boost/decimal/detail/cmath/fpclassify.hpp>

namespace boost {
namespace decimal {

namespace detail {

namespace exp_detail {

// exp2(x) for a finite x != 0, rounded once in the current mode. 2^n for an integer n is exact
// where it fits in 128 bits as 2^n or as 5^-n * 10^n. Else exp2(x) is never exact.
template <typename T>
constexpr auto exp2_fixed(const T x) noexcept -> T
{
    int e {};
    const auto sig {static_cast<boost::int128::uint128_t>(frexp10(x, &e))};
    const int k {-e};
    // x = sig * 10^-k is an integer only if 2^k divides sig, a test with no division
    if (k > 0 && k < 39 && (sig.low & ((UINT64_C(1) << static_cast<unsigned>(k)) - 1U)) == 0U)
    {
        const auto scale {pow10(static_cast<boost::int128::uint128_t>(k))};
        const auto n {sig / scale};
        if (n * scale == sig)
        {
            if (!signbit(x) && n < 128U)
            {
                return T {boost::int128::uint128_t {1U} << static_cast<unsigned>(n), 0};
            }
            if (signbit(x) && n < 56U)
            {
                boost::int128::uint128_t pow5 {1U};
                for (unsigned i {}; i < static_cast<unsigned>(n); ++i)
                {
                    pow5 *= 5U;
                }
                return T {pow5, -static_cast<int>(n)};
            }
        }
    }
    return exp_fixed<T, true>(x);
}

} // namespace exp_detail

template <typename T>
constexpr auto exp2_impl(const T num) noexcept
    BOOST_DECIMAL_REQUIRES(detail::is_decimal_floating_point_v, T)
{
    const auto fpc = fpclassify(num);

    constexpr T zero { 0, 0 };
    constexpr T one  { 1, 0 };

    auto result = zero;

    if (fpc == FP_ZERO)
    {
        result = one;
    }
    #ifndef BOOST_DECIMAL_FAST_MATH
    else if (fpc != FP_NORMAL && fpc != FP_SUBNORMAL)
    {
        if (fpc == FP_INFINITE)
        {
            result = (signbit(num) ? zero : num);
        }
        else
        {
            result = detail::check_non_finite(num);
        }
    }
    #endif
    else
    {
        result = exp_detail::exp2_fixed(num);
    }

    return result;
}

} //namespace detail

BOOST_DECIMAL_EXPORT template <typename T>
constexpr auto exp2(const T num) noexcept
    BOOST_DECIMAL_REQUIRES(detail::is_decimal_floating_point_v, T)
{
    using evaluation_type = detail::evaluation_type_t<T>;

    return static_cast<T>(detail::exp2_impl(static_cast<evaluation_type>(num)));
}

} //namespace decimal
} //namespace boost

#endif //BOOST_DECIMAL_DETAIL_CMATH_EXP2_HPP
