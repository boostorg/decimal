// Copyright 2025 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_DECIMAL_DETAIL_INT128_CSTDLIB_HPP
#define BOOST_DECIMAL_DETAIL_INT128_CSTDLIB_HPP

#include <boost/decimal/detail/int128/int128.hpp>

namespace boost {
namespace int128 {

BOOST_DECIMAL_DETAIL_INT128_EXPORT struct u128div_t
{
    uint128 quot;
    uint128 rem;
};

BOOST_DECIMAL_DETAIL_INT128_EXPORT struct i128div_t
{
    int128 quot;
    int128 rem;
};

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr u128div_t div(const uint128 x, const uint128 y) noexcept
{
    if (y == 0U)
    {
        BOOST_DECIMAL_DETAIL_INT128_UNREACHABLE;
    }

    if (x < y)
    {
        return u128div_t{0U, x};
    }
    else if (y.high != 0U)
    {
        u128div_t res {};
        res.quot = detail::knuth_div(x, y, res.rem);
        return res;
    }
    else
    {
        if (x.high == 0U)
        {
            return u128div_t{x.low / y.low, x.low % y.low};
        }
        else
        {
            u128div_t res {};
            detail::one_word_div(x, y.low, res.quot, res.rem);
            return res;
        }
    }
}

BOOST_DECIMAL_DETAIL_INT128_EXPORT BOOST_DECIMAL_DETAIL_INT128_HOST_DEVICE constexpr i128div_t div(const int128 x, const int128 y) noexcept
{
    if (y == 0)
    {
        BOOST_DECIMAL_DETAIL_INT128_UNREACHABLE;
    }

    const auto abs_lhs {static_cast<uint128>(abs(x))};
    const auto abs_rhs {static_cast<uint128>(abs(y))};

    if (abs_rhs > abs_lhs)
    {
        return {0, x};
    }

    const auto negative_quot {(x.signed_high() < 0) != (y.signed_high() < 0)};
    const auto negative_rem {x.signed_high() < 0};

    #if defined(BOOST_DECIMAL_DETAIL_INT128_HAS_INT128)

    if (abs_rhs.high != 0)
    {
        const auto builtin_x {static_cast<detail::builtin_i128>(x)};
        const auto builtin_y {static_cast<detail::builtin_i128>(y)};
        return i128div_t{static_cast<int128>(builtin_x / builtin_y),
                         static_cast<int128>(builtin_x % builtin_y)};
    }

    #endif

    const auto unsigned_res {div(abs_lhs, abs_rhs)};

    i128div_t res {static_cast<int128>(unsigned_res.quot), static_cast<int128>(unsigned_res.rem)};

    res.quot = negative_quot ? -res.quot : res.quot;
    res.rem = negative_rem ? -res.rem : res.rem;

    return res;
}

} // namespace int128
} // namespace boost

#endif // BOOST_DECIMAL_DETAIL_INT128_CSTDLIB_HPP
