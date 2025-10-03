// Copyright 2025 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_DECIMAL_BUILTIN_DECIMAL128_T_HPP
#define BOOST_DECIMAL_BUILTIN_DECIMAL128_T_HPP

#include <boost/decimal/detail/hardware_wrapper_template.hpp>
#include <boost/decimal/fwd.hpp>

template <>
#ifdef _MSC_VER
class std::numeric_limits<boost::decimal::builtin_decimal128_t>
#else
struct std::numeric_limits<boost::decimal::builtin_decimal128_t>
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

    #if !defined(__PPC64__) && !defined(__powerpc64__)
    static constexpr bool has_signaling_NaN = true;
    #else
    static constexpr bool has_signaling_NaN = false;
    #endif

    // These members were deprecated in C++23
    #if ((!defined(_MSC_VER) && (__cplusplus <= 202002L)) || (defined(_MSC_VER) && (_MSVC_LANG <= 202002L)))
    static constexpr std::float_denorm_style has_denorm = std::denorm_present;
    static constexpr bool has_denorm_loss = true;
    #endif

    static constexpr std::float_round_style round_style = std::round_indeterminate;
    static constexpr bool is_iec559 = false;
    static constexpr bool is_bounded = true;
    static constexpr bool is_modulo = false;
    static constexpr int  digits = 34;
    static constexpr int  digits10 = digits;
    static constexpr int  max_digits10 = digits;
    static constexpr int  radix = 10;
    static constexpr int  min_exponent = -6143;
    static constexpr int  min_exponent10 = min_exponent;
    static constexpr int  max_exponent = 6144;
    static constexpr int  max_exponent10 = max_exponent;
    static constexpr bool traps = numeric_limits<std::uint64_t>::traps;
    static constexpr bool tinyness_before = true;

    // Member functions
    static auto (min)        () -> boost::decimal::builtin_decimal128_t { return {UINT32_C(1), min_exponent}; }

    #if defined(__GNUC__) && __GNUC__ >= 8
    #  pragma GCC diagnostic push
    #  pragma GCC diagnostic ignored "-Wclass-memaccess"
    #endif

    static auto (max)        () -> boost::decimal::builtin_decimal128_t
    {
        constexpr boost::int128::uint128_t max_bits {boost::decimal::detail::_is_dpd ?
            BOOST_DECIMAL_DETAIL_INT128_UINT128_C(0U) :
            BOOST_DECIMAL_DETAIL_INT128_UINT128_C(127605503001634854143508416794254639103)};

        boost::decimal::builtin_decimal128_t result;
        std::memcpy(&result, &max_bits, sizeof(max_bits));
        return result;
    }

    #if defined(__GNUC__) && __GNUC__ >= 8
    #  pragma GCC diagnostic pop
    #endif

    static auto lowest       () -> boost::decimal::builtin_decimal128_t { return -(max)(); }
    static auto epsilon      () -> boost::decimal::builtin_decimal128_t { return {UINT32_C(1), -digits + 1}; }
    static auto round_error  () -> boost::decimal::builtin_decimal128_t { return epsilon(); }
    static auto infinity     () -> boost::decimal::builtin_decimal128_t { return boost::decimal::builtin_decimal128_t{std::numeric_limits<float>::infinity()}; }
    static auto quiet_NaN    () -> boost::decimal::builtin_decimal128_t { return boost::decimal::builtin_decimal128_t{std::numeric_limits<float>::quiet_NaN()}; }
    static auto signaling_NaN() -> boost::decimal::builtin_decimal128_t { return boost::decimal::builtin_decimal128_t{std::numeric_limits<float>::signaling_NaN()}; }
    static auto denorm_min   () -> boost::decimal::builtin_decimal128_t { return {UINT32_C(1), boost::decimal::detail::etiny_v<boost::decimal::decimal128_t>}; }
};

#endif // BOOST_DECIMAL_BUILTIN_DECIMAL128_T_HPP
