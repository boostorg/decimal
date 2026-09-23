// Copyright 2024 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_DECIMAL_BID_CONVERSION_HPP
#define BOOST_DECIMAL_BID_CONVERSION_HPP

#include <boost/decimal/decimal32_t.hpp>
#include <boost/decimal/decimal64_t.hpp>
#include <boost/decimal/decimal128_t.hpp>
#include <boost/decimal/decimal_fast32_t.hpp>
#include <boost/decimal/decimal_fast64_t.hpp>
#include <boost/decimal/decimal_fast128_t.hpp>
#include <boost/decimal/charconv.hpp>
#include <boost/decimal/detail/concepts.hpp>
#include "detail/int128.hpp"

namespace boost {
namespace decimal {

#if defined(__GNUC__) && __GNUC__ == 7
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wconversion"
#endif

// from_bid picks the fast type by overload, not by an explicit specialization, because clang 15 to 18
// drop an explicit specialization of a constrained template which comes from a precompiled header

BOOST_DECIMAL_EXPORT BOOST_DECIMAL_CUDA_CONSTEXPR auto to_bid_d32(const decimal32_t val) noexcept -> std::uint32_t
{
    return val.bits_;
}

// IEEE 754-2008 3.5.2: a binary integer significand encoding whose coefficient is larger
// than the format can hold is non canonical, and represents zero with the sign and the
// quantum exponent that it does encode. Only the 11 steering form can carry such a
// coefficient here, since the other form holds 23 bits and 2^23 - 1 is below 9'999'999.
BOOST_DECIMAL_EXPORT BOOST_DECIMAL_CUDA_CONSTEXPR auto from_bid_d32(const std::uint32_t bits) noexcept -> decimal32_t
{
    if ((bits & detail::d32_inf_mask) != detail::d32_inf_mask &&
        (bits & detail::d32_comb_11_mask) == detail::d32_comb_11_mask)
    {
        constexpr std::uint32_t implied_bit {UINT32_C(0x800000)};
        const std::uint32_t significand {implied_bit | (bits & detail::d32_11_significand_mask)};

        if (significand > detail::d32_max_significand_value)
        {
            const auto biased_exp {(bits & detail::d32_11_exp_mask) >> detail::d32_11_exp_shift};
            const auto exp {static_cast<int>(biased_exp) - static_cast<int>(detail::bias_v<decimal32_t>)};
            return decimal32_t {UINT32_C(0), exp, (bits & detail::d32_sign_mask) != UINT32_C(0)};
        }
    }

    return from_bits(bits);
}

BOOST_DECIMAL_EXPORT BOOST_DECIMAL_CUDA_CONSTEXPR auto to_bid_d32f(const decimal_fast32_t val) noexcept -> std::uint32_t
{
    const decimal32_t compliant_val {val};
    return to_bid_d32(compliant_val);
}

BOOST_DECIMAL_EXPORT BOOST_DECIMAL_CUDA_CONSTEXPR auto from_bid_d32f(const std::uint32_t bits) noexcept -> decimal_fast32_t
{
    const auto compliant_val {from_bid_d32(bits)};
    const decimal_fast32_t val {compliant_val};
    return val;
}

BOOST_DECIMAL_EXPORT BOOST_DECIMAL_CUDA_CONSTEXPR auto to_bid_d64(const decimal64_t val) noexcept -> std::uint64_t
{
    return val.bits_;
}

// As above: 2^53 - 1 is below 9'999'999'999'999'999, so only the 11 steering form can
// encode an out of range coefficient
BOOST_DECIMAL_EXPORT BOOST_DECIMAL_CUDA_CONSTEXPR auto from_bid_d64(const std::uint64_t bits) noexcept -> decimal64_t
{
    if ((bits & detail::d64_inf_mask) != detail::d64_inf_mask &&
        (bits & detail::d64_combination_field_mask) == detail::d64_combination_field_mask)
    {
        constexpr std::uint64_t implied_bit {UINT64_C(0x20000000000000)};
        const std::uint64_t significand {implied_bit | (bits & detail::d64_11_significand_mask)};

        if (significand > detail::d64_max_significand_value)
        {
            const auto biased_exp {(bits & detail::d64_11_exp_mask) >> detail::d64_11_exp_shift};
            const auto exp {static_cast<int>(biased_exp) - static_cast<int>(detail::bias_v<decimal64_t>)};
            return decimal64_t {UINT64_C(0), exp, (bits & detail::d64_sign_mask) != UINT64_C(0)};
        }
    }

    return from_bits(bits);
}

BOOST_DECIMAL_EXPORT BOOST_DECIMAL_CUDA_CONSTEXPR auto to_bid_d64f(const decimal_fast64_t val) noexcept -> std::uint64_t
{
    const decimal64_t compliant_val {val};
    return to_bid_d64(compliant_val);
}

BOOST_DECIMAL_EXPORT BOOST_DECIMAL_CUDA_CONSTEXPR auto from_bid_d64f(const std::uint64_t bits) noexcept -> decimal_fast64_t
{
    const auto compliant_val {from_bid_d64(bits)};
    const decimal_fast64_t val {compliant_val};
    return val;
}

BOOST_DECIMAL_EXPORT BOOST_DECIMAL_CUDA_CONSTEXPR auto to_bid_d128(const decimal128_t val) noexcept -> int128::uint128_t
{
    return val.bits_;
}

// Unlike the narrower formats, decimal128 can encode an out of range coefficient from
// either steering form: the wider form holds 113 bits and 2^113 - 1 is above 10^34 - 1,
// while the 11 form starts at 2^113 and so is never canonical
BOOST_DECIMAL_EXPORT BOOST_DECIMAL_CUDA_CONSTEXPR auto from_bid_d128(const int128::uint128_t bits) noexcept -> decimal128_t
{
    if ((bits.high & detail::d128_inf_mask.high) != detail::d128_inf_mask.high)
    {
        const bool comb_11 {(bits.high & detail::d128_combination_field_mask) == detail::d128_combination_field_mask};

        constexpr int128::uint128_t implied_bit {UINT64_C(0x2000000000000), 0};
        const auto significand {comb_11 ? (implied_bit | (bits & detail::d128_11_significand_mask))
                                        : (bits & detail::d128_not_11_significand_mask)};

        if (significand > detail::d128_max_significand_value)
        {
            const auto biased_exp {comb_11 ? ((bits.high & detail::d128_11_exp_mask) >> detail::d128_11_exp_high_word_shift)
                                           : ((bits.high & detail::d128_not_11_exp_mask) >> detail::d128_not_11_exp_high_word_shift)};
            const auto exp {static_cast<int>(biased_exp) - static_cast<int>(detail::bias_v<decimal128_t>)};
            return decimal128_t {int128::uint128_t{0, 0}, exp, (bits.high & detail::d128_sign_mask) != UINT64_C(0)};
        }
    }

    return from_bits(bits);
}

#ifdef BOOST_DECIMAL_HAS_INT128
BOOST_DECIMAL_EXPORT BOOST_DECIMAL_CUDA_CONSTEXPR auto from_bid_d128(const detail::builtin_uint128_t bits) noexcept -> decimal128_t
{
    return from_bid_d128(int128::uint128_t{bits});
}
#endif

BOOST_DECIMAL_EXPORT BOOST_DECIMAL_CUDA_CONSTEXPR auto to_bid_d128f(const decimal_fast128_t& val) noexcept -> int128::uint128_t
{
    const decimal128_t compliant_val {val};
    return to_bid_d128(compliant_val);
}

BOOST_DECIMAL_EXPORT BOOST_DECIMAL_CUDA_CONSTEXPR auto from_bid_d128f(const int128::uint128_t bits) noexcept -> decimal_fast128_t
{
    const auto compliant_val {from_bid_d128(bits)};
    const decimal_fast128_t val {compliant_val};
    return val;
}

#ifdef BOOST_DECIMAL_HAS_INT128
BOOST_DECIMAL_EXPORT BOOST_DECIMAL_CUDA_CONSTEXPR auto from_bid_d128f(const detail::builtin_uint128_t bits) noexcept -> decimal_fast128_t
{
    const auto compliant_val {from_bid_d128(bits)};
    const decimal_fast128_t val {compliant_val};
    return val;
}
#endif

BOOST_DECIMAL_EXPORT BOOST_DECIMAL_CUDA_CONSTEXPR auto to_bid(const decimal32_t val) noexcept -> std::uint32_t
{
    return to_bid_d32(val);
}

BOOST_DECIMAL_EXPORT BOOST_DECIMAL_CUDA_CONSTEXPR auto to_bid(const decimal_fast32_t val) noexcept -> std::uint32_t
{
    return to_bid_d32f(val);
}

BOOST_DECIMAL_EXPORT BOOST_DECIMAL_CUDA_CONSTEXPR auto to_bid(const decimal64_t val) noexcept -> std::uint64_t
{
    return to_bid_d64(val);
}

BOOST_DECIMAL_EXPORT BOOST_DECIMAL_CUDA_CONSTEXPR auto to_bid(const decimal_fast64_t val) noexcept -> std::uint64_t
{
    return to_bid_d64f(val);
}

BOOST_DECIMAL_EXPORT BOOST_DECIMAL_CUDA_CONSTEXPR auto to_bid(const decimal128_t val) noexcept -> int128::uint128_t
{
    return to_bid_d128(val);
}

BOOST_DECIMAL_EXPORT BOOST_DECIMAL_CUDA_CONSTEXPR auto to_bid(const decimal_fast128_t& val) noexcept -> int128::uint128_t
{
    return to_bid_d128f(val);
}

BOOST_DECIMAL_EXPORT template <typename T>
BOOST_DECIMAL_CUDA_CONSTEXPR auto to_bid(const T val) noexcept
{
    return to_bid(val);
}

BOOST_DECIMAL_EXPORT template <typename T = decimal32_t>
BOOST_DECIMAL_CUDA_CONSTEXPR auto from_bid(const std::uint32_t bits) noexcept
    BOOST_DECIMAL_REQUIRES(detail::is_ieee_type_v, T)
{
    return from_bid_d32(bits);
}

BOOST_DECIMAL_EXPORT template <typename T>
BOOST_DECIMAL_CUDA_CONSTEXPR auto from_bid(const std::uint32_t bits) noexcept
    BOOST_DECIMAL_REQUIRES_TWO_RETURN(detail::is_decimal_floating_point_v, T, detail::is_fast_type_v, T, T)
{
    return from_bid_d32f(bits);
}

BOOST_DECIMAL_EXPORT template <typename T = decimal64_t>
BOOST_DECIMAL_CUDA_CONSTEXPR auto from_bid(const std::uint64_t bits) noexcept
    BOOST_DECIMAL_REQUIRES(detail::is_ieee_type_v, T)
{
    return from_bid_d64(bits);
}

BOOST_DECIMAL_EXPORT template <typename T>
BOOST_DECIMAL_CUDA_CONSTEXPR auto from_bid(const std::uint64_t bits) noexcept
    BOOST_DECIMAL_REQUIRES_TWO_RETURN(detail::is_decimal_floating_point_v, T, detail::is_fast_type_v, T, T)
{
    return from_bid_d64f(bits);
}

BOOST_DECIMAL_EXPORT template <typename T = decimal128_t>
BOOST_DECIMAL_CUDA_CONSTEXPR auto from_bid(const int128::uint128_t bits) noexcept
    BOOST_DECIMAL_REQUIRES(detail::is_ieee_type_v, T)
{
    return from_bid_d128(bits);
}

BOOST_DECIMAL_EXPORT template <typename T>
BOOST_DECIMAL_CUDA_CONSTEXPR auto from_bid(const int128::uint128_t bits) noexcept
    BOOST_DECIMAL_REQUIRES_TWO_RETURN(detail::is_decimal_floating_point_v, T, detail::is_fast_type_v, T, T)
{
    return from_bid_d128f(bits);
}

#if defined(__GNUC__) && __GNUC__ == 7
#  pragma GCC diagnostic pop
#endif

} // namespace decimal
} // namespace boost

#endif //BOOST_BID_CONVERSION_HPP
