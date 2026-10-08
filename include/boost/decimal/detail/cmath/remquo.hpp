// Copyright 2023 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_DECIMAL_DETAIL_CMATH_REMQUO_HPP
#define BOOST_DECIMAL_DETAIL_CMATH_REMQUO_HPP

#include <boost/decimal/fwd.hpp>
#include <boost/decimal/detail/type_traits.hpp>
#include <boost/decimal/detail/concepts.hpp>
#include <boost/decimal/detail/cmath/decompose.hpp>
#include <boost/decimal/detail/mod_impl.hpp>
#include <boost/decimal/detail/config.hpp>

#ifndef BOOST_DECIMAL_BUILD_MODULE
#include <type_traits>
#include <limits>
#include <cmath>
#endif

namespace boost {
namespace decimal {

template <typename T>
constexpr auto remquo(const T x, const T y, int* quo) noexcept
    BOOST_DECIMAL_REQUIRES(detail::is_decimal_floating_point_v, T)
{
    #ifndef BOOST_DECIMAL_FAST_MATH
    if (!isfinite(x) || !isfinite(y))
    {
        if (isinf(x))
        {
            return std::numeric_limits<T>::quiet_NaN();
        }
        else if (isnan(x))
        {
            return x;
        }
        else if (isnan(y))
        {
            return y;
        }

        // x is finite and y is infinite
        *quo = 0;
        return x;
    }
    #endif

    // Apple clang and MSVC use the last nibble.
    // Everyone else uses 3 bits.
    // Standard only specifies at least 3, so they are both correct
    #if defined(__APPLE__) || defined(_MSC_VER)
    constexpr auto unsigned_value_mask {0b1111U};
    #else
    constexpr auto unsigned_value_mask {0b111U};
    #endif

    // The exact integer remainder holds the low bits of the rounded quotient
    const auto x_components {decompose(x)};
    const auto y_components {decompose(y)};

    if (y_components.sig == 0U)
    {
        return detail::mod_by_zero<T>();
    }

    const auto result {detail::exact_mod<T, unsigned_value_mask + 1U>(x_components.sig, x_components.exp, y_components.sig, y_components.exp)};

    *quo = static_cast<int>(result.quotient & unsigned_value_mask);
    *quo = x_components.sign != y_components.sign ? -*quo : *quo;

    return detail::pack_in_range<T>(result.coeff, result.exp, x_components.sign != result.flip_sign);
}

} // namespace decimal
} // namespace boost

#endif // BOOST_DECIMAL_DETAIL_CMATH_REMQUO_HPP
