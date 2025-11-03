// Copyright 2023 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_DECIMAL_DETAIL_CMATH_NAN_HPP
#define BOOST_DECIMAL_DETAIL_CMATH_NAN_HPP

#include <boost/decimal/fwd.hpp>
#include <boost/decimal/detail/config.hpp>
#include <boost/decimal/detail/type_traits.hpp>
#include <boost/decimal/detail/concepts.hpp>
#include <boost/decimal/detail/utilities.hpp>
#include <boost/decimal/cstdlib.hpp>

#ifndef BOOST_DECIMAL_BUILD_MODULE
#include <limits>
#endif

#if !defined(BOOST_DECIMAL_DISABLE_CLIB)

namespace boost {
namespace decimal {

namespace detail {

template <BOOST_DECIMAL_DECIMAL_FLOATING_TYPE TargetDecimalType, bool is_snan>
constexpr auto nan_impl(const char* arg) noexcept -> TargetDecimalType
{
    using sig_type = typename TargetDecimalType::significand_type;

    constexpr TargetDecimalType nan_type {is_snan ? std::numeric_limits<TargetDecimalType>::signaling_NaN() :
                                                    std::numeric_limits<TargetDecimalType>::quiet_NaN()};

    constexpr std::uint32_t significand_field_bits {sizeof(TargetDecimalType) == sizeof(std::uint32_t) ? 23U :
                                                    sizeof(TargetDecimalType) == sizeof(std::uint64_t) ? 53U : 110U};

    constexpr sig_type max_payload_value {(static_cast<sig_type>(1) << (significand_field_bits + 1U)) - 1U};
    constexpr TargetDecimalType zero {};
    constexpr TargetDecimalType zero_bits {zero ^ zero};

    sig_type value {};
    const auto r {from_chars_integer_impl<sig_type, sig_type>(arg, arg + detail::strlen(arg), value, 10)};

    if (!r || value > max_payload_value)
    {
        return nan_type;
    }
    else
    {
        return (zero_bits | value) | nan_type;
    }
}

} //namespace detail

BOOST_DECIMAL_EXPORT template <typename T>
constexpr auto nan(const char* arg) noexcept
    BOOST_DECIMAL_REQUIRES(detail::is_ieee_type_v, T)
{
    return detail::nan_impl<T, false>(arg);
}

BOOST_DECIMAL_EXPORT constexpr auto nand32(const char* arg) noexcept -> decimal32_t
{
    return detail::nan_impl<decimal32_t, false>(arg);
}

BOOST_DECIMAL_EXPORT constexpr auto nand64(const char* arg) noexcept -> decimal64_t
{
    return detail::nan_impl<decimal64_t, false>(arg);
}

BOOST_DECIMAL_EXPORT constexpr auto nand128(const char* arg) noexcept -> decimal128_t
{
    return detail::nan_impl<decimal128_t, false>(arg);
}

} //namespace decimal
} //namespace boost

#endif //#if !defined(BOOST_DECIMAL_DISABLE_CLIB)

#endif //BOOST_DECIMAL_DETAIL_CMATH_NAN_HPP
