// Copyright 2023-2024 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_DECIMAL_DETAIL_CMATH_IMPL_SIN_IMPL_HPP
#define BOOST_DECIMAL_DETAIL_CMATH_IMPL_SIN_IMPL_HPP

#include <boost/decimal/fwd.hpp>
#include <boost/decimal/detail/config.hpp>
#include <boost/decimal/detail/int128.hpp>
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
struct sin_table_imp
{
    // sin(r) = r * (1 - z * S(z)) with z = r^2 <= (pi/4)^2; the table has |coefficient| of S.
    static constexpr fx<1> d32[5] =
    {
        // sin degree 4 on [0,0.61685] max rel err 4.9991e-15
        {{UINT64_C(0x0AAAAAAAAA911C91)}},
        {{UINT64_C(0x0088888886439978)}},
        {{UINT64_C(0x00034033F272D0EC)}},
        {{UINT64_C(0x00000B8EBB68B293)}},
        {{UINT64_C(0x0000001A961A1733)}},
    };

    // From z^2 on, one word scaled by 2^(62 + d64_shift) is enough for decimal64.
    static constexpr int d64_shift {14};
    static constexpr fx<2> d64_head[2] =
    {
        {{UINT64_C(0xAA7D60D8D4E5483D), UINT64_C(0x0AAAAAAAAAAAAAAA)}},
        {{UINT64_C(0x7F5132D35AC08297), UINT64_C(0x0088888888888888)}},
    };

    static constexpr std::uint64_t d64_tail[6] = { UINT64_C(0xD00D00D00D00A6F5), UINT64_C(0x02E3BC74AAD78393), UINT64_C(0x0006B99159F6B038), UINT64_C(0x00000B0922F75776), UINT64_C(0x0000000D73DC0530), UINT64_C(0x000000000C8F4EE3) };

    // From z^3 on, 128 bits scaled by 2^(126 + d128_mid_shift), and from z^10 on, one word scaled by
    // 2^(62 + d128_tail_shift).
    static constexpr int d128_mid_shift {20};
    static constexpr int d128_tail_shift {76};
    static constexpr fx<3> d128_head[3] =
    {
        {{UINT64_C(0x0DD7D16E06FD9B5F), UINT64_C(0xAAAAAAAAAAAAAA05), UINT64_C(0x0AAAAAAAAAAAAAAA)}},
        {{UINT64_C(0x6725275D2EE91D9D), UINT64_C(0x888888888888419A), UINT64_C(0x0088888888888888)}},
        {{UINT64_C(0x717E4EB88E02AEE2), UINT64_C(0x4034034033F8A598), UINT64_C(0x0003403403403403)}},
    };

    static constexpr boost::int128::uint128_t d128_mid[7] =
    {
        boost::int128::uint128_t {UINT64_C(0xB8EF1D2AB6399C7D), UINT64_C(0x560E37B7D30EF961)},
        boost::int128::uint128_t {UINT64_C(0x01AE64567F544E38), UINT64_C(0xFE73EEF2F07C2439)},
        boost::int128::uint128_t {UINT64_C(0x0002C248C2750DA1), UINT64_C(0x2F906C4EE1A467D8)},
        boost::int128::uint128_t {UINT64_C(0x0000035CFE7CE677), UINT64_C(0x03CF050EDDAE0588)},
        boost::int128::uint128_t {UINT64_C(0x000000032A58EE06), UINT64_C(0x156A97CEBE58298E)},
        boost::int128::uint128_t {UINT64_C(0x00000000025E9368), UINT64_C(0xCF9BEE177CFEF916)},
        boost::int128::uint128_t {UINT64_C(0x00000000000171B8), UINT64_C(0xEE9A64E2B76E0BF3)},
    };

    static constexpr std::uint64_t d128_tail[2] = { UINT64_C(0xBB0CD32F8671EFEA), UINT64_C(0x004F5B056DD15B7F) };
};

#if !(defined(__cpp_inline_variables) && __cpp_inline_variables >= 201606L) && (!defined(_MSC_VER) || _MSC_VER != 1900)

template <bool b>
constexpr fx<1> sin_table_imp<b>::d32[5];

template <bool b>
constexpr int sin_table_imp<b>::d64_shift;

template <bool b>
constexpr int sin_table_imp<b>::d128_mid_shift;

template <bool b>
constexpr int sin_table_imp<b>::d128_tail_shift;

template <bool b>
constexpr fx<2> sin_table_imp<b>::d64_head[2];

template <bool b>
constexpr std::uint64_t sin_table_imp<b>::d64_tail[6];

template <bool b>
constexpr fx<3> sin_table_imp<b>::d128_head[3];

template <bool b>
constexpr boost::int128::uint128_t sin_table_imp<b>::d128_mid[7];

template <bool b>
constexpr std::uint64_t sin_table_imp<b>::d128_tail[2];

#endif

using sin_table = sin_table_imp<true>;

constexpr auto sin_poly(const fx<1>& z) noexcept -> fx<1>
{
    return fx_alternating(z, sin_table::d32);
}

constexpr auto sin_poly(const fx<2>& z) noexcept -> fx<2>
{
    return fx_alternating_split<sin_table::d64_shift>(z, sin_table::d64_head, sin_table::d64_tail);
}

constexpr auto sin_poly(const fx<3>& z) noexcept -> fx<3>
{
    return fx_alternating_split<sin_table::d128_mid_shift, sin_table::d128_tail_shift>(z, sin_table::d128_head, sin_table::d128_mid, sin_table::d128_tail);
}

// sin(r) / r for z = r^2, which is below 1 for any r != 0.
template <int N>
constexpr auto sinc_fx(const fx<N>& z) noexcept -> fx<N>
{
    return fx_clamp_below_one(fx_sub(fx_one<N>(), fx_mul(z, sin_poly(z))));
}

// sin(|r|) with the sign neg, for r != 0.
template <typename T>
constexpr auto sin_of(const trig_arg<trig_traits<T>::words>& r, const bool neg) noexcept -> T
{
    const auto rf {fixed_r(r)};
    const auto f {sinc_fx(fx_mul(rf, rf))};
    return trig_round<T>(fx_scale(r.sig, f), -r.k, neg);
}

} // namespace trig
} // namespace detail
} // namespace decimal
} // namespace boost

#endif // BOOST_DECIMAL_DETAIL_CMATH_IMPL_SIN_IMPL_HPP
