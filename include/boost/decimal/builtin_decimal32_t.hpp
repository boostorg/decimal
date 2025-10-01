// Copyright 2025 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_DECIMAL_BUILTIN_DECIMAL32_T_HPP
#define BOOST_DECIMAL_BUILTIN_DECIMAL32_T_HPP

#include <boost/decimal/detail/hardware_wrapper_template.hpp>
#include <boost/decimal/fwd.hpp>

template <>
#ifdef _MSC_VER
class std::numeric_limits<boost::decimal::builtin_decimal32_t>
#else
struct std::numeric_limits<boost::decimal::builtin_decimal32_t>
#endif
{

#ifdef _MSC_VER
public:
#endif

    static constexpr bool is_specialized = true;
    static constexpr bool is_signed = true;
    static constexpr bool is_integer = false;
    static constexpr bool is_exact = false;
    static constexpr bool has_infinity = true;
    static constexpr bool has_quiet_NaN = true;
    static constexpr bool has_signaling_NaN = true;

    // These members were deprecated in C++23
    #if ((!defined(_MSC_VER) && (__cplusplus <= 202002L)) || (defined(_MSC_VER) && (_MSVC_LANG <= 202002L)))
    static constexpr std::float_denorm_style has_denorm = std::denorm_present;
    static constexpr bool has_denorm_loss = true;
    #endif

    static constexpr std::float_round_style round_style = std::round_indeterminate;
    static constexpr bool is_iec559 = true;
    static constexpr bool is_bounded = true;
    static constexpr bool is_modulo = false;
    static constexpr int digits = 7;
    static constexpr int digits10 = digits;
    static constexpr int max_digits10 = digits;
    static constexpr int radix = 10;
    static constexpr int min_exponent = -95;
    static constexpr int min_exponent10 = min_exponent;
    static constexpr int max_exponent = 96;
    static constexpr int max_exponent10 = max_exponent;
    static constexpr bool traps = numeric_limits<std::uint32_t>::traps;
    static constexpr bool tinyness_before = true;

    static auto (min)        () -> boost::decimal::builtin_decimal32_t { return {UINT32_C(1), min_exponent}; }
    static auto (max)        () -> boost::decimal::builtin_decimal32_t { return {boost::decimal::detail::d32_max_significand_value, max_exponent - digits + 1}; }
    static auto lowest       () -> boost::decimal::builtin_decimal32_t { return {boost::decimal::detail::d32_max_significand_value, max_exponent - digits + 1, true}; }
    static auto epsilon      () -> boost::decimal::builtin_decimal32_t { return {UINT32_C(1), -digits + 1}; }
    static auto round_error  () -> boost::decimal::builtin_decimal32_t { return epsilon(); }

    // The values below used for inf, qnan, and snan do not depend on DPD vs BID formatting

    static auto infinity     () -> boost::decimal::builtin_decimal32_t
    {
        return boost::decimal::builtin_decimal32_t{std::numeric_limits<float>::infinity()};
    }

    static auto quiet_NaN    () -> boost::decimal::builtin_decimal32_t
    {
        return boost::decimal::builtin_decimal32_t{std::numeric_limits<float>::quiet_NaN()};
    }

    static auto signaling_NaN() -> boost::decimal::builtin_decimal32_t
    {
        return boost::decimal::builtin_decimal32_t{std::numeric_limits<float>::signaling_NaN()};
    }

    static auto denorm_min   () -> boost::decimal::builtin_decimal32_t { return {UINT32_C(1), boost::decimal::detail::etiny}; }

};

#endif // BOOST_DECIMAL_BUILTIN_DECIMAL32_T_HPP
