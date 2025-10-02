// Copyright 2025 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_DECIMAL_DETAIL_DECODE_ENCODE_MASKS
#define BOOST_DECIMAL_DETAIL_DECODE_ENCODE_MASKS

#include <boost/decimal/detail/config.hpp>

namespace boost {
namespace decimal {
namespace detail {

// Decimal32
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

// Decimal64
// See IEEE 754 section 3.5.2
BOOST_DECIMAL_INLINE_CONSTEXPR_VARIABLE std::uint64_t d64_inf_mask  = UINT64_C(0x7800000000000000);
BOOST_DECIMAL_INLINE_CONSTEXPR_VARIABLE std::uint64_t d64_nan_mask  = UINT64_C(0x7C00000000000000);
BOOST_DECIMAL_INLINE_CONSTEXPR_VARIABLE std::uint64_t d64_snan_mask = UINT64_C(0x7E00000000000000);

//    Comb.  Exponent          Significand
// s        eeeeeeeeee    [ttt][tttttttttt][tttttttttt][tttttttttt][tttttttttt][tttttttttt]
// s   11   eeeeeeeeee   [100t][tttttttttt][tttttttttt][tttttttttt][tttttttttt][tttttttttt]

BOOST_DECIMAL_INLINE_CONSTEXPR_VARIABLE std::uint64_t d64_sign_mask = UINT64_C(0b1'00000'00000000'0000000000'0000000000'0000000000'0000000000'0000000000);
BOOST_DECIMAL_INLINE_CONSTEXPR_VARIABLE std::uint64_t d64_combination_field_mask = UINT64_C(0b0'11'00000000'000'0000000000'0000000000'0000000000'0000000000'0000000000);

BOOST_DECIMAL_INLINE_CONSTEXPR_VARIABLE std::uint64_t d64_not_11_exp_mask = UINT64_C(0b0'11'11111111'000'0000000000'0000000000'0000000000'0000000000'0000000000);
BOOST_DECIMAL_INLINE_CONSTEXPR_VARIABLE std::uint64_t d64_not_11_exp_shift = UINT64_C(53);
BOOST_DECIMAL_INLINE_CONSTEXPR_VARIABLE std::uint64_t d64_11_exp_mask = UINT64_C(0b0'00'1111111111'0'0000000000'0000000000'0000000000'0000000000'0000000000);
BOOST_DECIMAL_INLINE_CONSTEXPR_VARIABLE std::uint64_t d64_11_exp_shift = UINT64_C(51);

BOOST_DECIMAL_INLINE_CONSTEXPR_VARIABLE std::uint64_t d64_not_11_significand_mask = UINT64_C(0b0'00'00000000'111'1111111111'1111111111'1111111111'1111111111'1111111111);
BOOST_DECIMAL_INLINE_CONSTEXPR_VARIABLE std::uint64_t d64_11_significand_mask = UINT64_C(0b0'00'0000000000'1'1111111111'1111111111'1111111111'1111111111'1111111111);

BOOST_DECIMAL_INLINE_CONSTEXPR_VARIABLE std::uint64_t d64_biggest_no_combination_significand = d64_not_11_significand_mask;

BOOST_DECIMAL_INLINE_CONSTEXPR_VARIABLE std::uint64_t d64_max_biased_exponent = UINT64_C(767);
BOOST_DECIMAL_INLINE_CONSTEXPR_VARIABLE std::uint64_t d64_max_significand_value = UINT64_C(9'999'999'999'999'999);


} // namespace detail
} // namespace decimal
} // namespace boost

#endif // BOOST_DECIMAL_DETAIL_DECODE_ENCODE_MASKS
