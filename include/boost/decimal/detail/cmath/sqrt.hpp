// Copyright 2023 - 2024 Matt Borland
// Copyright 2023 - 2024 Christopher Kormanyos
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_DECIMAL_DETAIL_CMATH_SQRT_HPP
#define BOOST_DECIMAL_DETAIL_CMATH_SQRT_HPP

#include <boost/decimal/fwd.hpp>
#include <boost/decimal/detail/type_traits.hpp>
#include <boost/decimal/detail/concepts.hpp>
#include <boost/decimal/detail/config.hpp>
#include <boost/decimal/detail/to_float.hpp>

#ifndef BOOST_DECIMAL_BUILD_MODULE
#include <type_traits>
#include <cstdint>
#include <cmath>
#endif

namespace boost {
namespace decimal {

namespace detail {

// "Simple" approach: Convert decimal to binary float, use hardware sqrt, convert back
// This leverages the highly optimized binary floating-point sqrt implementations
template <typename T>
constexpr auto sqrt_impl(const T x) noexcept
    BOOST_DECIMAL_REQUIRES(detail::is_decimal_floating_point_v, T)
{
    const auto fpc = fpclassify(x);

    T result { };

    #ifndef BOOST_DECIMAL_FAST_MATH
    if ((fpc == FP_NAN) || (fpc == FP_ZERO))
    {
        result = x;
    }
    else if (signbit(x))
    {
        result = std::numeric_limits<T>::quiet_NaN();
    }
    else if (fpc == FP_INFINITE)
    {
        result = std::numeric_limits<T>::infinity();
    }
    #else
    if (signbit(x))
    {
        result = T{0};
    }
    #endif
    else
    {
        // Convert to binary floating-point (double has enough precision for all decimal types)
        // Use double for all types - it has 53 bits of precision which exceeds:
        // - decimal32: 7 decimal digits (~23 bits)
        // - decimal64: 16 decimal digits (~53 bits) 
        // - decimal128: 34 decimal digits (~113 bits) - may lose precision but sqrt will still be accurate enough
        const double x_binary = boost::decimal::to_float<T, double>(x);
        
        // Use hardware-optimized sqrt
        const double sqrt_binary = std::sqrt(x_binary);
        
        // Convert back to decimal
        result = T(sqrt_binary);
    }

    return result;
}

} //namespace detail

BOOST_DECIMAL_EXPORT template <typename T>
constexpr auto sqrt(const T val) noexcept
    BOOST_DECIMAL_REQUIRES(detail::is_decimal_floating_point_v, T)
{
    using evaluation_type = detail::evaluation_type_t<T>;

    return static_cast<T>(detail::sqrt_impl(static_cast<evaluation_type>(val)));
}

} //namespace decimal
} //namespace boost

#endif //BOOST_DECIMAL_DETAIL_CMATH_SQRT_HPP

