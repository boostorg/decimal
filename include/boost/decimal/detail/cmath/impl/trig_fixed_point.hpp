// Copyright 2026 Shen-Ta Hsieh
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_DECIMAL_DETAIL_CMATH_IMPL_TRIG_FIXED_POINT_HPP
#define BOOST_DECIMAL_DETAIL_CMATH_IMPL_TRIG_FIXED_POINT_HPP

#include <boost/decimal/fwd.hpp>
#include <boost/decimal/detail/config.hpp>
#include <boost/decimal/detail/int128.hpp>
#include <boost/decimal/detail/power_tables.hpp>

#ifndef BOOST_DECIMAL_BUILD_MODULE
#include <cstddef>
#include <cstdint>
#endif

namespace boost {
namespace decimal {
namespace detail {
namespace trig {

#ifdef _MSC_VER
#  pragma warning(push)
#  pragma warning(disable : 4127) // Conditional expression is constant
#endif

// Fixed point with N words of 64 bits, low word first: the value is the integer divided by
// 2^(64N - 2), in the range [0, 4).
template <int N>
struct fx
{
    std::uint64_t w[static_cast<std::size_t>(N)];
};

template <bool b>
struct fixed_table_imp
{
    // floor(2^382 / 10^(46 + 9a)) for a = 0 to 7. Its top N + 1 words are floor(2^(64N + 190) / 10^(46 + 9a)).
    static constexpr fx<4> pow10_neg[8] =
    {
        {{UINT64_C(0x51BA47FEE249CE5F), UINT64_C(0x329C3A0F675D7764), UINT64_C(0xAAC1C372ACE584C1), UINT64_C(0x00000024899C4858)}},
        {{UINT64_C(0x9CF16DDC1CC486D3), UINT64_C(0x464DD69685606BAB), UINT64_C(0xED737BB6C4183D55), UINT64_C(0x000000000000009C)}},
        {{UINT64_C(0xCD8890A87FB90364), UINT64_C(0xE7A694FC8E635D1E), UINT64_C(0x000002A1FFA89E94), UINT64_C(0x0000000000000000)}},
        {{UINT64_C(0x8AEB6360B1AF3451), UINT64_C(0xCD5F01A4AA8281E3), UINT64_C(0x0000000000000B4E), UINT64_C(0x0000000000000000)}},
        {{UINT64_C(0xC086FEFA16EB73A6), UINT64_C(0x0000309114B688A6), UINT64_C(0x0000000000000000), UINT64_C(0x0000000000000000)}},
        {{UINT64_C(0xAD07A71F26B27E20), UINT64_C(0x000000000000D097), UINT64_C(0x0000000000000000), UINT64_C(0x0000000000000000)}},
        {{UINT64_C(0x00037FE5DC91C0A5), UINT64_C(0x0000000000000000), UINT64_C(0x0000000000000000), UINT64_C(0x0000000000000000)}},
        {{UINT64_C(0x00000000000F07DA), UINT64_C(0x0000000000000000), UINT64_C(0x0000000000000000), UINT64_C(0x0000000000000000)}},
    };
};

#if !(defined(__cpp_inline_variables) && __cpp_inline_variables >= 201606L) && (!defined(_MSC_VER) || _MSC_VER != 1900)

template <bool b>
constexpr fx<4> fixed_table_imp<b>::pow10_neg[8];

#endif

using fixed_table = fixed_table_imp<true>;

template <int N>
constexpr auto fx_one() noexcept -> fx<N>
{
    fx<N> r {};
    r.w[N - 1] = UINT64_C(1) << 62U;
    return r;
}

template <int N>
constexpr auto fx_is_one(const fx<N>& a) noexcept -> bool
{
    for (int i {}; i < N - 1; ++i)
    {
        if (a.w[i] != 0U)
        {
            return false;
        }
    }
    return a.w[N - 1] == (UINT64_C(1) << 62U);
}

// a * b, truncated.
template <int N>
constexpr auto fx_mul(const fx<N>& a, const fx<N>& b) noexcept -> fx<N>
{
    std::uint64_t p[static_cast<std::size_t>(2 * N)] {};
    for (int i {}; i < N; ++i)
    {
        std::uint64_t c {};
        for (int j {}; j < N; ++j)
        {
            const auto t {static_cast<boost::int128::uint128_t>(a.w[i]) * b.w[j] + p[i + j] + c};
            p[i + j] = t.low;
            c = t.high;
        }
        p[i + N] = c;
    }
    fx<N> r {};
    for (int i {}; i < N; ++i)
    {
        r.w[i] = (p[N - 1 + i] >> 62U) | (p[N + i] << 2U);
    }
    return r;
}

// a - b, for a >= b.
template <int N>
constexpr auto fx_sub(const fx<N>& a, const fx<N>& b) noexcept -> fx<N>
{
    fx<N> r {};
    std::uint64_t borrow {};
    for (int i {}; i < N; ++i)
    {
        const std::uint64_t t {a.w[i] - b.w[i] - borrow};
        borrow = (a.w[i] < b.w[i] || (a.w[i] == b.w[i] && borrow != 0U)) ? 1U : 0U;
        r.w[i] = t;
    }
    return r;
}

// min(a, 1 - 2^-(64N - 2)): a value that must be below one stays below one.
template <int N>
constexpr auto fx_clamp_below_one(const fx<N>& a) noexcept -> fx<N>
{
    if (a.w[N - 1] < (UINT64_C(1) << 62U))
    {
        return a;
    }
    fx<N> r {};
    for (int i {}; i < N - 1; ++i)
    {
        r.w[i] = ~UINT64_C(0);
    }
    r.w[N - 1] = (UINT64_C(1) << 62U) - 1U;
    return r;
}

// |c0| - z*(|c1| - z*(|c2| - ...)): the coefficients of the sin and cos kernels alternate in sign.
template <int N, std::size_t M>
constexpr auto fx_alternating(const fx<N>& z, const fx<N> (&c)[M]) noexcept -> fx<N>
{
    fx<N> s {c[M - 1]};
    for (std::size_t i {M - 1}; i-- > 0U;)
    {
        s = fx_sub(c[i], fx_mul(z, s));
    }
    return s;
}

// The same, with the terms from z^M on in one word scaled by 2^(62 + Shift): their weight is small
// enough that 64 bits of them keep the error of the sum below 2^-(64N - 2).
template <int Shift, int N, std::size_t M, std::size_t K>
constexpr auto fx_alternating_split(const fx<N>& z, const fx<N> (&head)[M], const std::uint64_t (&tail)[K]) noexcept -> fx<N>
{
    static_assert(0 <= Shift && Shift < 64 * (N - 1), "the tail must fit in the N words");

    const std::uint64_t z1 {z.w[N - 1]};
    std::uint64_t t {tail[K - 1]};
    for (std::size_t i {K - 1}; i-- > 0U;)
    {
        t = tail[i] - ((static_cast<boost::int128::uint128_t>(z1) * t).high << 2U);
    }

    // t * 2^-(62 + Shift) in N words
    fx<N> s {};
    constexpr int pos {64 * (N - 1) - Shift};
    constexpr int word {pos / 64};
    constexpr int bit {pos % 64};
    s.w[word] = t << bit;
    if (bit != 0 && word + 1 < N)
    {
        s.w[word + 1] = t >> (64 - bit);
    }
    for (std::size_t i {M}; i-- > 0U;)
    {
        s = fx_sub(head[i], fx_mul(z, s));
    }
    return s;
}

// The high 128 bits of a * b.
constexpr auto mul_high(const boost::int128::uint128_t& a, const boost::int128::uint128_t& b) noexcept -> boost::int128::uint128_t
{
    const auto ll {static_cast<boost::int128::uint128_t>(a.low) * b.low};
    const auto lh {static_cast<boost::int128::uint128_t>(a.low) * b.high};
    const auto hl {static_cast<boost::int128::uint128_t>(a.high) * b.low};
    const auto hh {static_cast<boost::int128::uint128_t>(a.high) * b.high};
    const auto mid {static_cast<boost::int128::uint128_t>(ll.high) + lh.low + hl.low};
    return hh + lh.high + hl.high + mid.high;
}

// For three words: the terms from z^M on in 128 bits scaled by 2^(126 + MidShift), and the terms
// after those in one word scaled by 2^(62 + TailShift).
template <int MidShift, int TailShift, std::size_t M, std::size_t K, std::size_t L>
constexpr auto fx_alternating_split(const fx<3>& z, const fx<3> (&head)[M], const boost::int128::uint128_t (&mid)[K],
                                    const std::uint64_t (&tail)[L]) noexcept -> fx<3>
{
    static_assert(0 < MidShift && MidShift < 64, "the middle terms must fit in the three words");
    static_assert(0 <= 64 + MidShift - TailShift && 64 + MidShift - TailShift < 64, "the tail must fit in 128 bits");

    const std::uint64_t z1 {z.w[2]};
    std::uint64_t t {tail[L - 1]};
    for (std::size_t i {L - 1}; i-- > 0U;)
    {
        t = tail[i] - ((static_cast<boost::int128::uint128_t>(z1) * t).high << 2U);
    }

    const boost::int128::uint128_t z2 {z.w[2], z.w[1]};
    auto u {static_cast<boost::int128::uint128_t>(t) << (64 + MidShift - TailShift)};
    for (std::size_t i {K}; i-- > 0U;)
    {
        u = mid[i] - (mul_high(z2, u) << 2U);
    }

    // u * 2^-(126 + MidShift) in three words
    constexpr int pos {64 - MidShift};
    fx<3> s {};
    s.w[0] = u.low << pos;
    s.w[1] = (u.low >> (64 - pos)) | (u.high << pos);
    s.w[2] = u.high >> (64 - pos);
    for (std::size_t i {M}; i-- > 0U;)
    {
        s = fx_sub(head[i], fx_mul(z, s));
    }
    return s;
}

// sig * 10^-k as fixed point, for 38 <= k < 110, else 0. With k = 38 + 9a + b, sig * 10^(8 - b) is exact in
// three words; times 2^(64N + 190) / 10^(46 + 9a) it is sig * 10^-k * 2^(64N + 190): drop 192 bits.
template <int N>
constexpr auto fx_from(const boost::int128::uint128_t& sig, const int k) noexcept -> fx<N>
{
    constexpr int rows {static_cast<int>(sizeof(fixed_table::pow10_neg) / sizeof(fixed_table::pow10_neg[0]))};

    fx<N> r {};
    if (k < 38 || k >= 38 + 9 * rows)
    {
        return r;
    }
    const auto f {pow10(static_cast<std::uint64_t>(8 - (k - 38) % 9))};
    const auto lo {static_cast<boost::int128::uint128_t>(sig.low) * f};
    const auto hi {static_cast<boost::int128::uint128_t>(sig.high) * f + lo.high};
    const std::uint64_t g[3] {lo.low, hi.low, hi.high};
    const auto& q {fixed_table::pow10_neg[(k - 38) / 9]};

    std::uint64_t p[static_cast<std::size_t>(N + 4)] {};
    for (int i {}; i < 3; ++i)
    {
        std::uint64_t c {};
        for (int j {}; j < N + 1; ++j)
        {
            const auto t {static_cast<boost::int128::uint128_t>(g[i]) * q.w[j + 3 - N] + p[i + j] + c};
            p[i + j] = t.low;
            c = t.high;
        }
        p[i + N + 1] = c;
    }
    for (int i {}; i < N; ++i)
    {
        r.w[i] = p[i + 3];
    }
    return r;
}

// floor(sig * f / 2^(64N - 2)), for f <= 1.5 so that the result has at most 39 digits.
template <int N>
constexpr auto fx_scale(const boost::int128::uint128_t& sig, const fx<N>& f) noexcept -> boost::int128::uint128_t
{
    std::uint64_t p[static_cast<std::size_t>(N + 2)] {};
    const std::uint64_t g[2] {sig.low, sig.high};
    for (int i {}; i < 2; ++i)
    {
        std::uint64_t c {};
        for (int j {}; j < N; ++j)
        {
            const auto t {static_cast<boost::int128::uint128_t>(g[i]) * f.w[j] + p[i + j] + c};
            p[i + j] = t.low;
            c = t.high;
        }
        p[i + N] = c;
    }
    return boost::int128::uint128_t {(p[N] >> 62U) | (p[N + 1] << 2U), (p[N - 1] >> 62U) | (p[N] << 2U)};
}

// sig * 10^e rounded once in the current mode, for a sig of 37 or 38 digits which truncates a value that
// is never exact. The value is in (sig, sig + 1), thus if the last digit is 0 or 5, sig + 1 rounds the same.
template <typename T>
constexpr auto trig_round(boost::int128::uint128_t sig, int e, const bool neg) noexcept -> T
{
    if (sig < pow10(static_cast<boost::int128::uint128_t>(37)))
    {
        sig *= 10U;
        --e;
    }

    // The constructor sees a 0 as exact and, where it drops one digit only, a 5 as a tie.
    // 2^64 = 1 (mod 5), thus sig = high + low (mod 5).
    if ((sig.high % 5U + sig.low % 5U) % 5U == 0U)
    {
        sig += 1U;
    }
    return T {sig, e, neg};
}

#ifdef _MSC_VER
#  pragma warning(pop)
#endif

} // namespace trig
} // namespace detail
} // namespace decimal
} // namespace boost

#endif // BOOST_DECIMAL_DETAIL_CMATH_IMPL_TRIG_FIXED_POINT_HPP
