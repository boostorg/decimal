// Copyright 2023 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_DECIMAL_DETAIL_CMATH_ILOGB_HPP
#define BOOST_DECIMAL_DETAIL_CMATH_ILOGB_HPP

#include <boost/decimal/fwd.hpp> // NOLINT(llvm-include-order)
#include <boost/decimal/detail/type_traits.hpp>
#include <boost/decimal/detail/concepts.hpp>
#include <boost/decimal/detail/config.hpp>

#ifndef BOOST_DECIMAL_BUILD_MODULE
#include <cmath>
#include <type_traits>
#endif

namespace boost {
namespace decimal {

BOOST_DECIMAL_EXPORT template <typename T>
constexpr auto ilogb(const T d) noexcept
    BOOST_DECIMAL_REQUIRES_RETURN(detail::is_decimal_floating_point_v, T, int)
{
    const auto fpc = fpclassify(d);

    int result { };

    if (fpc == FP_ZERO)
    {
        result = FP_ILOGB0;
    }
    #ifndef BOOST_DECIMAL_FAST_MATH
    else if (fpc == FP_INFINITE)
    {
        result = INT_MAX;
    }
    else if (fpc == FP_NAN)
    {
        result = FP_ILOGBNAN;
    }
    #endif
    else
    {
        // The exponent must be unbiased, and adjusted so that the significand is
        // interpreted as having a single digit before the decimal point
        const auto offset {detail::num_digits(d.full_significand()) - 1};

        result = static_cast<int>(d.biased_exponent()) + static_cast<int>(offset);
    }

    return result;
}

} // namespace decimal
} // namespace boost

#endif // BOOST_DECIMAL_DETAIL_CMATH_ILOGB_HPP
