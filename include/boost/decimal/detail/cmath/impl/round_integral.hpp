// Copyright 2026 Shen-Ta Hsieh
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_DECIMAL_DETAIL_CMATH_IMPL_ROUND_INTEGRAL_HPP
#define BOOST_DECIMAL_DETAIL_CMATH_IMPL_ROUND_INTEGRAL_HPP

#include <boost/decimal/detail/config.hpp>
#include <boost/decimal/detail/type_traits.hpp>
#include <boost/decimal/detail/attributes.hpp>
#include <boost/decimal/detail/power_tables.hpp>
#include <boost/decimal/detail/cmath/fpclassify.hpp>
#include <boost/decimal/detail/cmath/decompose.hpp>
#include <boost/decimal/detail/add_impl.hpp>
#include <boost/decimal/cfenv.hpp>

namespace boost {
namespace decimal {
namespace detail {

// Rounds c.sig * 10^c.exp, with c.exp < 0, to an integer in the mode round:
// one division by 10^-c.exp
template <typename T, typename Components>
BOOST_DECIMAL_FORCE_INLINE constexpr auto round_integral_sig(const Components& c, const rounding_mode round) noexcept -> typename T::significand_type
{
    using sig_type = typename T::significand_type;

    const auto shift {-static_cast<int>(c.exp)};
    sig_type q {0U};
    bool inc {};

    if (shift > precision_v<T>)
    {
        // 0 < |x| < 0.1
        inc = round == (c.sign ? rounding_mode::fe_dec_downward : rounding_mode::fe_dec_upward);
    }
    else
    {
        const auto p {pow10(static_cast<sig_type>(shift))};
        q = static_cast<sig_type>(c.sig / p);
        const auto r {static_cast<sig_type>(c.sig - q * p)};
        if (r == 0U)
        {
            return q;
        }

        switch (round)
        {
            case rounding_mode::fe_dec_upward:
                inc = !c.sign;
                break;
            case rounding_mode::fe_dec_downward:
                inc = c.sign;
                break;
            case rounding_mode::fe_dec_to_nearest_from_zero:
                inc = r >= p - r;
                break;
            case rounding_mode::fe_dec_to_nearest:
                inc = r > p - r || (r == p - r && (static_cast<std::uint32_t>(q) & 1U) == 1U);
                break;
            case rounding_mode::fe_dec_toward_zero:
            default:
                break;
        }
    }

    if (inc)
    {
        ++q;
    }

    return q;
}

// Rounds x to an integer in the mode round. The result has exponent 0, and a zero result
// keeps the sign of x.
template <typename T>
constexpr auto round_integral_impl(const T x, const rounding_mode round) noexcept -> T
{
    #ifndef BOOST_DECIMAL_FAST_MATH
    if (!isfinite(x))
    {
        return x;
    }
    #endif

    const auto c {decompose(x)};
    if (c.exp >= 0 || c.sig == 0U)
    {
        return x;
    }

    return pack_in_range<T>(round_integral_sig<T>(c, round), 0, c.sign);
}

} // namespace detail
} // namespace decimal
} // namespace boost

#endif // BOOST_DECIMAL_DETAIL_CMATH_IMPL_ROUND_INTEGRAL_HPP
