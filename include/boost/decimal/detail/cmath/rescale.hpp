// Copyright 2025 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_DECIMAL_DETAIL_CMATH_RESCALE_HPP
#define BOOST_DECIMAL_DETAIL_CMATH_RESCALE_HPP

#include <boost/decimal/fwd.hpp>
#include <boost/decimal/detail/type_traits.hpp>
#include <boost/decimal/detail/concepts.hpp>
#include <boost/decimal/detail/config.hpp>
#include <boost/decimal/detail/attributes.hpp>
#include <boost/decimal/detail/fenv_rounding.hpp>
#include <boost/decimal/detail/quantize_impl.hpp>
#include <boost/decimal/detail/cmath/floor.hpp>
#include <boost/decimal/detail/cmath/ceil.hpp>
#include <boost/decimal/detail/cmath/frexp10.hpp>
#include <boost/decimal/detail/cmath/trunc.hpp>

#ifndef BOOST_DECIMAL_BUILD_MODULE
#include <type_traits>
#include <limits>
#endif

namespace boost {
namespace decimal {

// Rounds val to precision digits after the decimal point using the active rounding mode.
// A negative precision rounds to the corresponding power of ten, and a value that already
// has no more than precision fractional digits is returned unchanged.
BOOST_DECIMAL_EXPORT template <typename T>
constexpr auto rescale(const T val, const int precision = 0) noexcept
    BOOST_DECIMAL_REQUIRES(detail::is_decimal_floating_point_v, T)
{
    if (isnan(val) || isinf(val) || abs(val) == 0)
    {
        return val;
    }

    // precision counts digits after the decimal point, so the quantum we want is 10^-precision.
    // Saturate to the exponents the format can encode so the shift below stays in range.
    constexpr auto max_precision {detail::bias_v<T>};
    constexpr auto min_precision {detail::bias_v<T> - detail::max_biased_exp_v<T>};

    const auto clamped_precision {precision > max_precision ? max_precision :
                                  (precision < min_precision ? min_precision : precision)};
    const auto target_exp {-clamped_precision};

    int exp {};
    auto sig {frexp10(val, &exp)};
    const auto isneg {val < 0};
    const auto delta {exp - target_exp};

    // val cannot carry any more fractional digits than it already holds
    if (delta >= 0)
    {
        return val;
    }

    // Only ever shrinks the significand here, so this cannot fail
    detail::quantize_rescale<T>(sig, delta, isneg);

    return {sig, target_exp, isneg};
}

} // namespace decimal
} // namespace boost

#endif //BOOST_DECIMAL_DETAIL_CMATH_RESCALE_HPP
