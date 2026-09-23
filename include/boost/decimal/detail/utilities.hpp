// Copyright 2023 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_DECIMAL_DETAIL_UTILITIES_HPP
#define BOOST_DECIMAL_DETAIL_UTILITIES_HPP

#include <boost/decimal/detail/config.hpp>

#ifndef BOOST_DECIMAL_BUILD_MODULE
#include <cstddef>
#include <limits>
#endif

namespace boost {
namespace decimal {
namespace detail {

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
