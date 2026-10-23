// Copyright 2023 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_DECIMAL_DETAIL_CMATH_REMAINDER_HPP
#define BOOST_DECIMAL_DETAIL_CMATH_REMAINDER_HPP

#include <boost/decimal/fwd.hpp>
#include <boost/decimal/detail/cmath/decompose.hpp>
#include <boost/decimal/detail/mod_impl.hpp>
#include <boost/decimal/detail/type_traits.hpp>
#include <boost/decimal/detail/concepts.hpp>
#include <boost/decimal/detail/config.hpp>

#ifndef BOOST_DECIMAL_BUILD_MODULE
#include <type_traits>
#include <limits>
#include <cmath>
#endif

namespace boost {
namespace decimal {

BOOST_DECIMAL_EXPORT template <typename T>
constexpr auto remainder(const T x, const T y) noexcept
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
        return x;
    }
    #endif

    // The integer remainder is exact, and it rounds the quotient half to even
    const auto x_components {decompose(x)};
    const auto y_components {decompose(y)};

    if (y_components.sig == 0U)
    {
        return detail::mod_by_zero<T>();
    }

    const auto result {detail::exact_mod<T, 2U>(x_components.sig, x_components.exp, y_components.sig, y_components.exp)};

    return detail::pack_in_range<T>(result.coeff, result.exp, x_components.sign != result.flip_sign);
}

} // namespace decimal
} // namespace boost

#endif // BOOST_DECIMAL_DETAIL_CMATH_REMAINDER_HPP
