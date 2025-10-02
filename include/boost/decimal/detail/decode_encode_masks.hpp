// Copyright 2025 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_DECIMAL_DETAIL_DECODE_ENCODE_MASKS
#define BOOST_DECIMAL_DETAIL_DECODE_ENCODE_MASKS

#include <boost/decimal/detail/config.hpp>

namespace boost {
namespace decimal {
namespace detail {

// See IEEE 754 section 3.5.2
BOOST_DECIMAL_INLINE_CONSTEXPR_VARIABLE std::uint32_t d32_inf_mask  = UINT32_C(0x78000000);
BOOST_DECIMAL_INLINE_CONSTEXPR_VARIABLE std::uint32_t d32_nan_mask  = UINT32_C(0x7C000000);
BOOST_DECIMAL_INLINE_CONSTEXPR_VARIABLE std::uint32_t d32_snan_mask = UINT32_C(0x7E000000);

//    Comb.  Exponent          Significand
// s         eeeeeeee     ttttttttttttttttttttttt - sign + 2 steering bits concatenate to 6 bits of exponent (8 total) + 23 bits of significand like float
// s   11    eeeeeeee    [100] + ttttttttttttttttttttt - sign + 2 steering bits + 8 bits of exponent + 21 bits of significand (0b100 + 21 bits)
//
// Only is the type different in steering 11 which yields significand 100 + 21 bits giving us our 24 total bits of precision

BOOST_DECIMAL_INLINE_CONSTEXPR_VARIABLE std::uint32_t d32_sign_mask = UINT32_C(0b10000000000000000000000000000000);
BOOST_DECIMAL_INLINE_CONSTEXPR_VARIABLE std::uint32_t d32_combination_field_mask = UINT32_C(0b01100000000000000000000000000000);

BOOST_DECIMAL_INLINE_CONSTEXPR_VARIABLE std::uint32_t d32_comb_11_mask = UINT32_C(0b0'11000'000000'0000000000'0000000000);

BOOST_DECIMAL_INLINE_CONSTEXPR_VARIABLE std::uint32_t d32_not_11_exp_mask = UINT32_C(0b01111111100000000000000000000000);
BOOST_DECIMAL_INLINE_CONSTEXPR_VARIABLE std::uint32_t d32_not_11_exp_shift = UINT32_C(23);
BOOST_DECIMAL_INLINE_CONSTEXPR_VARIABLE std::uint32_t d32_11_exp_mask = UINT32_C(0b00011111111000000000000000000000);
BOOST_DECIMAL_INLINE_CONSTEXPR_VARIABLE std::uint32_t d32_11_exp_shift = UINT32_C(21);

BOOST_DECIMAL_INLINE_CONSTEXPR_VARIABLE std::uint32_t d32_not_11_significand_mask = UINT32_C(0b00000000011111111111111111111111);
BOOST_DECIMAL_INLINE_CONSTEXPR_VARIABLE std::uint32_t d32_11_significand_mask = UINT32_C(0b00000000000111111111111111111111);

BOOST_DECIMAL_INLINE_CONSTEXPR_VARIABLE std::uint32_t d32_biggest_no_combination_significand = UINT32_C(0b11111111111111111111111); // 23 bits

BOOST_DECIMAL_INLINE_CONSTEXPR_VARIABLE std::uint32_t d32_max_biased_exponent = UINT32_C(191);
BOOST_DECIMAL_INLINE_CONSTEXPR_VARIABLE std::uint32_t d32_max_significand_value = UINT32_C(9'999'999);

} // namespace detail
} // namespace decimal
} // namespace boost

#endif // BOOST_DECIMAL_DETAIL_DECODE_ENCODE_MASKS
