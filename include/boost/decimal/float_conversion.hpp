// Copyright 2026 Pavel P
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_DECIMAL_FLOAT_CONVERSION_HPP
#define BOOST_DECIMAL_FLOAT_CONVERSION_HPP

#include <boost/decimal/decimal32_t.hpp>
#include <boost/decimal/decimal64_t.hpp>
#include <boost/decimal/decimal128_t.hpp>
#include <boost/decimal/decimal_fast32_t.hpp>
#include <boost/decimal/decimal_fast64_t.hpp>
#include <boost/decimal/decimal_fast128_t.hpp>
#include <boost/decimal/detail/concepts.hpp>
#include <boost/decimal/detail/binary_to_decimal.hpp>

namespace boost {
namespace decimal {

// IEEE 754 convertFormat (5.4.2) from binary64: the exact value of val correctly rounded to DecimalType in the
// current rounding mode, as Intel's binary64_to_bid64 computes it. An exact result takes the exponent closest to 0, an
// inexact one all the digits. The constructor DecimalType {val} differs: it keeps the shortest decimal that reads back
// as val, so 0.07 gives 0.07 there and 0.07000000000000001 here (as decimal64_t).
BOOST_DECIMAL_EXPORT template <typename DecimalType>
BOOST_DECIMAL_CXX20_CONSTEXPR auto from_binary(const double val) noexcept
    BOOST_DECIMAL_REQUIRES(detail::is_decimal_floating_point_v, DecimalType)
{
    return detail::binary_to_decimal::from_binary64<DecimalType>(val);
}

// The same from binary32, whose value a double holds exactly
BOOST_DECIMAL_EXPORT template <typename DecimalType>
BOOST_DECIMAL_CXX20_CONSTEXPR auto from_binary(const float val) noexcept
    BOOST_DECIMAL_REQUIRES(detail::is_decimal_floating_point_v, DecimalType)
{
    return detail::binary_to_decimal::from_binary64<DecimalType>(static_cast<double>(val));
}

} // namespace decimal
} // namespace boost

#endif // BOOST_DECIMAL_FLOAT_CONVERSION_HPP
