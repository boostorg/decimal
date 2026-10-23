// Copyright 2023 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_DECIMAL_DETAIL_CMATH_IMPL_COS_IMPL_HPP
#define BOOST_DECIMAL_DETAIL_CMATH_IMPL_COS_IMPL_HPP

#include <boost/decimal/fwd.hpp>
#include <boost/decimal/detail/config.hpp>
#include <boost/decimal/detail/int128.hpp>
#include <boost/decimal/detail/power_tables.hpp>
#include <boost/decimal/detail/cmath/impl/trig_fixed_point.hpp>
#include <boost/decimal/detail/cmath/impl/trig_reduce.hpp>

#ifndef BOOST_DECIMAL_BUILD_MODULE
#include <cstdint>
#endif

namespace boost {
namespace decimal {
namespace detail {
namespace trig {

template <bool b>
struct cos_table_imp
{
    // cos(r) = 1 - z * (1/2 - z * C(z)) with z = r^2 <= (pi/4)^2; the table has |coefficient| of C.
    static constexpr fx<1> d32[5] =
    {
        // cos degree 4 on [0,0.61685] max rel err 9.2329e-17
        {{UINT64_C(0x02AAAAAAAAA5BB5B)}},
        {{UINT64_C(0x0016C16C16721120)}},
        {{UINT64_C(0x000068067E97FEFC)}},
        {{UINT64_C(0x00000127E00B90DA)}},
        {{UINT64_C(0x00000002377D1C31)}},
    };

    // From z^2 on, one word scaled by 2^(62 + d64_shift) is enough for decimal64.
    static constexpr int d64_shift {17};
    static constexpr fx<2> d64_head[2] =
    {
        {{UINT64_C(0x66A629652876004D), UINT64_C(0x02AAAAAAAAAAAAAA)}},
        {{UINT64_C(0x6CDA6F0ECA1D2ECE), UINT64_C(0x0016C16C16C16C0F)}},
    };

    static constexpr std::uint64_t d64_tail[5] = { UINT64_C(0xD00D00D00C6653EB), UINT64_C(0x024FC9F6EBD7192B), UINT64_C(0x00047BB632432CE0), UINT64_C(0x0000064E4C907AB4), UINT64_C(0x00000006AAF461CB) };

    // From z^2 on, 128 bits scaled by 2^(126 + d128_mid_shift), and from z^10 on, one word scaled by
    // 2^(62 + d128_tail_shift).
    static constexpr int d128_mid_shift {17};
    static constexpr int d128_tail_shift {81};
    static constexpr fx<3> d128_head[2] =
    {
        {{UINT64_C(0xEA45AD4A0D5AF6A0), UINT64_C(0xAAAAAAAAAAAAAA7A), UINT64_C(0x02AAAAAAAAAAAAAA)}},
        {{UINT64_C(0xB80DD449D4ED2C61), UINT64_C(0xC16C16C16C16B484), UINT64_C(0x0016C16C16C16C16)}},
    };

    static constexpr boost::int128::uint128_t d128_mid[8] =
    {
        boost::int128::uint128_t {UINT64_C(0xD00D00D00D00D00D), UINT64_C(0x00D00CFE0B2E1E72)},
        boost::int128::uint128_t {UINT64_C(0x024FC9F6EF13EB8E), UINT64_C(0x5DE02D7E98F54548)},
        boost::int128::uint128_t {UINT64_C(0x00047BB63BFE3625), UINT64_C(0xED51352B37C62468)},
        boost::int128::uint128_t {UINT64_C(0x0000064E5D2A301F), UINT64_C(0x274825BC9C08DEA7)},
        boost::int128::uint128_t {UINT64_C(0x00000006B9FCF9CC), UINT64_C(0xEE079F20A4359338)},
        boost::int128::uint128_t {UINT64_C(0x0000000005A09E18), UINT64_C(0xEE5EF9C706842ACD)},
        boost::int128::uint128_t {UINT64_C(0x000000000003CA85), UINT64_C(0x747F6661142C8245)},
        boost::int128::uint128_t {UINT64_C(0x0000000000000219), UINT64_C(0xC72C8B0FDE1153D3)},
    };

    static constexpr std::uint64_t d128_tail[2] = { UINT64_C(0xF96673E127BADABC), UINT64_C(0x0061AC811D606933) };
};

#if !(defined(__cpp_inline_variables) && __cpp_inline_variables >= 201606L) && (!defined(_MSC_VER) || _MSC_VER != 1900)

template <bool b>
constexpr fx<1> cos_table_imp<b>::d32[5];

template <bool b>
constexpr int cos_table_imp<b>::d64_shift;

template <bool b>
constexpr int cos_table_imp<b>::d128_mid_shift;

template <bool b>
constexpr int cos_table_imp<b>::d128_tail_shift;

template <bool b>
constexpr fx<2> cos_table_imp<b>::d64_head[2];

template <bool b>
constexpr std::uint64_t cos_table_imp<b>::d64_tail[5];

template <bool b>
constexpr fx<3> cos_table_imp<b>::d128_head[2];

template <bool b>
constexpr boost::int128::uint128_t cos_table_imp<b>::d128_mid[8];

template <bool b>
constexpr std::uint64_t cos_table_imp<b>::d128_tail[2];

#endif

using cos_table = cos_table_imp<true>;

constexpr auto cos_poly(const fx<1>& z) noexcept -> fx<1>
{
    return fx_alternating(z, cos_table::d32);
}

constexpr auto cos_poly(const fx<2>& z) noexcept -> fx<2>
{
    return fx_alternating_split<cos_table::d64_shift>(z, cos_table::d64_head, cos_table::d64_tail);
}

constexpr auto cos_poly(const fx<3>& z) noexcept -> fx<3>
{
    return fx_alternating_split<cos_table::d128_mid_shift, cos_table::d128_tail_shift>(z, cos_table::d128_head, cos_table::d128_mid, cos_table::d128_tail);
}

// cos(r) for z = r^2, which is below 1 for any r != 0.
template <int N>
constexpr auto cos_fx(const fx<N>& z) noexcept -> fx<N>
{
    fx<N> half {};
    half.w[N - 1] = UINT64_C(1) << 61U;
    const auto inner {fx_sub(half, fx_mul(z, cos_poly(z)))};
    return fx_clamp_below_one(fx_sub(fx_one<N>(), fx_mul(z, inner)));
}

// cos(r) with the sign neg, for r != 0.
template <typename T>
constexpr auto cos_of(const trig_arg<trig_traits<T>::words>& r, const bool neg) noexcept -> T
{
    const auto rf {fixed_r(r)};
    const auto f {cos_fx(fx_mul(rf, rf))};
    return trig_round<T>(fx_scale(pow10(static_cast<boost::int128::uint128_t>(38)), f), -38, neg);
}

} // namespace trig
} // namespace detail
} // namespace decimal
} // namespace boost

#endif // BOOST_DECIMAL_DETAIL_CMATH_IMPL_COS_IMPL_HPP
