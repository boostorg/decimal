// Copyright 2023 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_DECIMAL_DETAIL_UTILITIES_HPP
#define BOOST_DECIMAL_DETAIL_UTILITIES_HPP

#include <boost/decimal/detail/config.hpp>
#include <boost/decimal/detail/attributes.hpp>

#ifndef BOOST_DECIMAL_BUILD_MODULE
#include <cstddef>
#include <limits>
#endif

namespace boost {
namespace decimal {
namespace detail {

// Infinity, NaN, a non-canonical significand, or a new exponent out of range.
// A call out of line keeps these cases small in the caller, thus the fast path inlines.
template <typename T>
BOOST_DECIMAL_NO_INLINE BOOST_DECIMAL_CUDA_CONSTEXPR auto scalbln_out_of_line(const T num, const long exp) noexcept -> T
{
    #ifndef BOOST_DECIMAL_FAST_MATH
    if (!isfinite(num))
    {
        // IEEE 754-2019 7.2: an operation on a signaling NaN gives a quiet NaN with the same payload.
        return issignaling(num) ? nan_conversion(num) : num;
    }
    #endif

    // NVCC cannot read the uint128_t variable template max_significand_v, thus call impl::max_significand_v.
    constexpr auto max_sig {impl::max_significand_v<T>()};

    // IEEE 754-2019 3.5.2: a significand above 10^p - 1 is not canonical and has the value zero, and 5.3.3:
    // scaleB does not propagate a non-canonical encoding, thus the result is a canonical zero, even when exp is 0.
    auto sig {num.full_significand()};
    if (sig > static_cast<decltype(sig)>(max_sig))
    {
        sig = 0U;
    }

    // An exponent past the limit gives the same overflow or underflow as the limit,
    // and the sum stays in the range of int, to which the constructor casts the exponent.
    constexpr long exp_limit {2L * (max_biased_exp_v<T> + precision_v<T>)};
    const long clamped_exp {exp < -exp_limit ? -exp_limit : (exp > exp_limit ? exp_limit : exp)};
    return T {sig, num.biased_exponent() + clamped_exp, num.isneg()};
}

template <typename T>
BOOST_DECIMAL_CUDA_CONSTEXPR auto swap(T& x, T& y) noexcept -> void
{
    const T temp {x};
    x = y;
    y = temp;
}

template <typename T>
constexpr auto strlen(const T* str) noexcept -> std::size_t
{
    std::size_t i {};

    if (str == nullptr)
    {
        return i;
    }

    while (*str != '\0')
    {
        ++str;
        ++i;
    }

    return i;
}

template <typename T>
BOOST_DECIMAL_CUDA_CONSTEXPR auto (max)(const T& b, const T& a) noexcept -> const T&
{
    return (a < b) ? b : a;
}

// numeric_limits is not specialized for __float128 on the libstdc++ of GCC 15 and before,
// and the infinity of double takes its place there, because that conversion is exact
template <typename Float>
BOOST_DECIMAL_CUDA_CONSTEXPR auto infinity_value() noexcept -> Float
{
    return std::numeric_limits<Float>::is_specialized ?
           (std::numeric_limits<Float>::infinity)() :
           static_cast<Float>((std::numeric_limits<double>::infinity)());
}

// An ordered compare with a NaN raises FE_INVALID, and == or != draws -Wfloat-equal, whose
// diagnostic pragma a precompiled header loses, so the compilers which have the builtin take it
template <typename Float>
BOOST_DECIMAL_CUDA_CONSTEXPR auto is_nan_value(const Float val) noexcept -> bool
{
    #if (defined(__GNUC__) || defined(__clang__)) && !defined(BOOST_DECIMAL_ENABLE_CUDA)
    return static_cast<bool>(__builtin_isnan(val));
    #else
    return val != val;
    #endif
}

} // namespace detail
} // namespace decimal
} // namespace boost

#endif //BOOST_DECIMAL_DETAIL_UTILITIES_HPP
