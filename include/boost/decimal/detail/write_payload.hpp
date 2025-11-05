// Copyright 2025 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_DECIMAL_DETAIL_WRITE_PAYLOAD_HPP
#define BOOST_DECIMAL_DETAIL_WRITE_PAYLOAD_HPP

#include <boost/decimal/detail/config.hpp>
#include <boost/decimal/detail/concepts.hpp>
#include <boost/decimal/detail/promotion.hpp>

namespace boost {
namespace decimal {
namespace detail {

template <typename TargetDecimalType, bool is_snan>
constexpr auto write_payload(typename TargetDecimalType::significand_type payload_value)
    BOOST_DECIMAL_REQUIRES(detail::is_fast_type_v, TargetDecimalType)
{
    constexpr TargetDecimalType nan_type {is_snan ? std::numeric_limits<TargetDecimalType>::signaling_NaN() :
                                                    std::numeric_limits<TargetDecimalType>::quiet_NaN()};

    TargetDecimalType return_value {nan_type};
    return_value.significand_ |= payload_value;
    return return_value;
}

template <typename TargetDecimalType, bool is_snan>
constexpr auto write_payload(typename TargetDecimalType::significand_type payload_value)
    BOOST_DECIMAL_REQUIRES(detail::is_ieee_type_v, TargetDecimalType)
{
    constexpr TargetDecimalType nan_type {is_snan ? std::numeric_limits<TargetDecimalType>::signaling_NaN() :
                                                        std::numeric_limits<TargetDecimalType>::quiet_NaN()};

    constexpr TargetDecimalType zero {};
    constexpr TargetDecimalType zero_bits {zero ^ zero};

    return (zero_bits | payload_value) | nan_type;
}

} // namespace detail
} // namespace decimal
} // namespace boost

#endif // BOOST_DECIMAL_DETAIL_WRITE_PAYLOAD_HPP
