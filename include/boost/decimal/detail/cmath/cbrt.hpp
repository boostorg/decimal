// Copyright 2023 - 2026 Matt Borland
// Copyright 2023 - 2026 Christopher Kormanyos
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_DECIMAL_DETAIL_CMATH_CBRT_HPP
#define BOOST_DECIMAL_DETAIL_CMATH_CBRT_HPP

#include <boost/decimal/fwd.hpp>
#include <boost/decimal/detail/type_traits.hpp>
#include <boost/decimal/detail/concepts.hpp>
#include <boost/decimal/detail/config.hpp>
#include <boost/decimal/detail/int128.hpp>
#include <boost/decimal/detail/countl.hpp>
#include <boost/decimal/detail/power_tables.hpp>
#include <boost/decimal/detail/u256.hpp>
#include <boost/decimal/detail/fenv_rounding.hpp>
#include <boost/decimal/detail/cmath/frexp10.hpp>
#include <boost/decimal/detail/add_impl.hpp>

#ifndef BOOST_DECIMAL_BUILD_MODULE
#include <type_traits>
#include <cstdint>
#include <cmath>
#include <limits>
#endif

namespace boost {
namespace decimal {

namespace detail {

// recip: 2^31 / cbrt(1 + i/8) for i = 0 ... 56. bulge: 2^33 times the gap between the chord and the curve
// at the middle of each step. Both rounded.
template <bool b>
struct cbrt_table_imp
{
    static constexpr std::uint32_t recip[57] = {
        UINT32_C(2147483648), UINT32_C(2064804912), UINT32_C(1993547224), UINT32_C(1931207619), UINT32_C(1875999763), UINT32_C(1826608232),
        UINT32_C(1782038911), UINT32_C(1741523911), UINT32_C(1704458901), UINT32_C(1670360536), UINT32_C(1638836745), UINT32_C(1609565523),
        UINT32_C(1582279480), UINT32_C(1556754380), UINT32_C(1532800503), UINT32_C(1510256044), UINT32_C(1488981999), UINT32_C(1468858154),
        UINT32_C(1449779914), UINT32_C(1431655765), UINT32_C(1414405221), UINT32_C(1397957157), UINT32_C(1382248444), UINT32_C(1367222814),
        UINT32_C(1352829926), UINT32_C(1339024576), UINT32_C(1325766036), UINT32_C(1313017496), UINT32_C(1300745587), UINT32_C(1288919972),
        UINT32_C(1277513002), UINT32_C(1266499411), UINT32_C(1255856056), UINT32_C(1245561691), UINT32_C(1235596770), UINT32_C(1225943276),
        UINT32_C(1216584566), UINT32_C(1207505238), UINT32_C(1198691017), UINT32_C(1190128646), UINT32_C(1181805796), UINT32_C(1173710981),
        UINT32_C(1165833489), UINT32_C(1158163310), UINT32_C(1150691080), UINT32_C(1143408030), UINT32_C(1136305934), UINT32_C(1129377067),
        UINT32_C(1122614168), UINT32_C(1116010402), UINT32_C(1109559331), UINT32_C(1103254881), UINT32_C(1097091317), UINT32_C(1091063221),
        UINT32_C(1085165467), UINT32_C(1079393201), UINT32_C(1073741824),
    };
    static constexpr std::uint32_t bulge[56] = {
        UINT32_C(6487520), UINT32_C(5002337), UINT32_C(3959235), UINT32_C(3201240), UINT32_C(2634762), UINT32_C(2201346),
        UINT32_C(1863046), UINT32_C(1594409), UINT32_C(1377876), UINT32_C(1201033), UINT32_C(1054919), UINT32_C(932936),
        UINT32_C(830149), UINT32_C(742808), UINT32_C(668027), UINT32_C(603554), UINT32_C(547617), UINT32_C(498802),
        UINT32_C(455974), UINT32_C(418213), UINT32_C(384765), UINT32_C(355012), UINT32_C(328441), UINT32_C(304621),
        UINT32_C(283195), UINT32_C(263859), UINT32_C(246355), UINT32_C(230464), UINT32_C(215998), UINT32_C(202795),
        UINT32_C(190716), UINT32_C(179638), UINT32_C(169458), UINT32_C(160082), UINT32_C(151430), UINT32_C(143431),
        UINT32_C(136022), UINT32_C(129148), UINT32_C(122760), UINT32_C(116814), UINT32_C(111270), UINT32_C(106096),
        UINT32_C(101258), UINT32_C(96729), UINT32_C(92484), UINT32_C(88501), UINT32_C(84758), UINT32_C(81237),
        UINT32_C(77921), UINT32_C(74796), UINT32_C(71846), UINT32_C(69060), UINT32_C(66426), UINT32_C(63933),
        UINT32_C(61571), UINT32_C(59333),
    };
};

#if !(defined(__cpp_inline_variables) && __cpp_inline_variables >= 201606L) && (!defined(_MSC_VER) || _MSC_VER != 1900)
template <bool b> constexpr std::uint32_t cbrt_table_imp<b>::recip[57];
template <bool b> constexpr std::uint32_t cbrt_table_imp<b>::bulge[56];
#endif

using cbrt_table = cbrt_table_imp<true>;

// a * b with int128::detail::umul: GCC keeps it in registers, but moves a class product through memory
constexpr auto cbrt_mul(const std::uint64_t a, const std::uint64_t b) noexcept -> int128::uint128_t
{
    std::uint64_t hi { };
    const std::uint64_t lo { int128::detail::umul(a, b, hi) };
    return { hi, lo };
}

// a * b mod 2^128
constexpr auto cbrt_mul(const int128::uint128_t a, const std::uint64_t b) noexcept -> int128::uint128_t
{
    return cbrt_mul(a.low, b) + int128::uint128_t { a.high * b, 0U };
}

// (a * b) >> s, for s in [1, 64]: the low 64 bits.
constexpr auto cbrt_mulshift(const std::uint64_t a, const std::uint64_t b, const int s) noexcept -> std::uint64_t
{
    const int128::uint128_t r { cbrt_mul(a, b) };
    return s == 64 ? r.high : (r.high << (64 - s)) | (r.low >> s);
}

// 2^63 / cbrt(a) for a * 2^61 with a in [1, 8): a chord between two table points less a parabola (error < 2^-16).
constexpr auto cbrt_guess(const std::uint64_t a) noexcept -> std::uint64_t
{
    const std::uint64_t i { (a >> 58U) - 8U };
    const std::uint64_t f { (a >> 42U) & UINT64_C(0xFFFF) };
    // MSVC 19.44 crashes in constant evaluation if the table values convert without a cast
    const std::uint64_t k0 { static_cast<std::uint64_t>(cbrt_table::recip[i]) };
    const std::uint64_t k1 { static_cast<std::uint64_t>(cbrt_table::recip[i + 1U]) };
    return (k0 << 32U) - (((k0 - k1) * f) << 16U) - cbrt_table::bulge[i] * (f * (UINT64_C(0x10000) - f));
}

// cbrt(a) * 2^62 from a * 2^61. A Newton step t += t * (1 - a * t^3) / 3 squares the error of the guess, and the
// last step gives y = a * t^2 * (1 + 2 * (1 - a * t^3) / 3) straight away. t2 gets t^2 with the same step.
struct cbrt_est
{
    std::uint64_t y;
    std::uint64_t t2;
};

constexpr auto cbrt_estimate(const std::uint64_t a) noexcept -> cbrt_est
{
    // e = 1 - a * t^3 + 2^-13 * 2^63 > 0 as |1 - a * t^3| < 2^-14, thus a product k * e needs no sign: k * 2^-13 is
    // its offset. Selects on the sign of 1 - a * t^3 became jumps, and the sign is random.
    constexpr std::uint64_t one { (UINT64_C(1) << 63U) + (UINT64_C(1) << 50U) };
    std::uint64_t t { cbrt_guess(a) };
    // Two passes: a Newton step on t, then the last step
    for (int n { 0 }; ; ++n)
    {
        // a * t * 2^60 and t^2 * 2^63 side by side, then a * t^3 * 2^63
        const auto at { cbrt_mulshift(a, t, 64) };
        const auto t2 { cbrt_mulshift(t, t, 63) };
        const auto t_3 { t / 3U };
        const auto e { one - cbrt_mulshift(at, t2, 60) };
        if (n == 1)
        {
            const auto ky { cbrt_mulshift(at, t_3, 60) };
            const auto kt { cbrt_mulshift(t, t_3, 62) };
            return { cbrt_mulshift(at, t, 61) + cbrt_mulshift(ky, e, 63) - (ky >> 13U),
                     t2 + cbrt_mulshift(kt, e, 63) - (kt >> 13U) };
        }
        t += cbrt_mulshift(t_3, e, 63) - (t_3 >> 13U);
    }
}

// IEEE 754-2019 4.3: the result is the root rounded once in the current mode. A cube root is never a tie.
// v = floor(2 * r) for an estimate r of the root with |r - cbrt N| < 1/2, so one boundary z / 2 at most lies
// near the root: the half-integer next to r in the nearest modes, the integer next to r in the others.
// The sign of 8N - z^3 (and its zero) picks the result. Words are mod 2^w with |8N - z^3| < 2^(w-1).
struct cbrt_mode
{
    bool nearest;
    bool up;
};

BOOST_DECIMAL_CUDA_CONSTEXPR auto cbrt_get_mode(const bool neg) noexcept -> cbrt_mode
{
    auto round {_boost_decimal_global_rounding_mode};
    #ifndef BOOST_DECIMAL_NO_CONSTEVAL_DETECTION
    if (!BOOST_DECIMAL_IS_CONSTANT_EVALUATED(neg))
    {
        round = _boost_decimal_global_runtime_rounding_mode;
    }
    #endif
    return { round == rounding_mode::fe_dec_to_nearest || round == rounding_mode::fe_dec_to_nearest_from_zero,
             round == (neg ? rounding_mode::fe_dec_downward : rounding_mode::fe_dec_upward) };
}

template <typename R>
constexpr auto cbrt_boundary(const R v, const cbrt_mode md) noexcept -> R
{
    return md.nearest ? (v | 1U) : ((v + 1U) & ~R { 1U });
}

template <typename R>
constexpr auto cbrt_pick(const R z, const bool below, const bool exact, const cbrt_mode md) noexcept -> R
{
    // nearest: the root is above z / 2 unless below; else floor is z / 2 or one less. below excludes exact. Sums of
    // the flags, as the side of the root is random and a jump on it is mispredicted half of the time.
    const R h { z >> 1U };
    if (md.nearest)
    {
        return h + static_cast<R>(!below);
    }
    return h - static_cast<R>(below) + static_cast<R>(md.up && !exact);
}

// m * 2^(61 - 3c) in [2^61, 2^64). m = s * 10^d has bit length bl(s) + 3d or one more, thus c0 = d + bl(s) / 3
// puts the top bit of m << (61 - 3c0) at 60 to 63 without a count of m: a top bit at 60 takes one more step of 3.
struct cbrt_scaled
{
    std::uint64_t a;
    int c;
};

constexpr auto cbrt_scale(const std::uint64_t m, const int c0) noexcept -> cbrt_scaled
{
    const std::uint64_t a0 { m << (61 - 3 * c0) };
    const bool low { (a0 >> 61U) == 0U };
    return { a0 << (low ? 3U : 0U), c0 - static_cast<int>(low) };
}

// m = s * 10^d < 10^(p + 2): an estimate of cbrt N for N = m * 10^(2p - 2), then the rounded root.
// decimal32_t: p = 7, w = 64.
constexpr auto cbrt_root(const std::uint32_t s, const int d, const bool neg) noexcept -> std::uint32_t
{
    const std::uint64_t m { static_cast<std::uint64_t>(s) * pow10(static_cast<std::uint32_t>(d)) };
    // The fallback of countl_zero counts a 32-bit value in 64 bits
    const auto sc { cbrt_scale(m, d + (64 - countl_zero(static_cast<std::uint64_t>(s))) / 3) };
    const int c { sc.c };
    const std::uint64_t a { sc.a };
    // The last step of cbrt_estimate on 32-bit words, as a has 30 bits and the root needs 2^-25: a * 2^29,
    // t * 2^31, a * t * 2^60, t^2 * 2^62, a * t^3 * 2^62 and y = a * t^2 * (1 + 2e / 3) * 2^62.
    const std::uint64_t t { cbrt_guess(a) >> 32U };
    const std::uint64_t at { (a >> 32U) * t };
    const std::uint64_t t2 { t * t };
    const std::uint64_t e { (UINT64_C(1) << 62U) + (UINT64_C(1) << 49U) - (at >> 30U) * (t2 >> 30U) };
    const std::uint64_t at2 { (at >> 29U) * t };
    const std::uint64_t ky { (at2 / 3U) >> 31U };
    const std::uint64_t y { at2 + ((ky * (e >> 17U)) >> 13U) - (ky << 19U) };
    // y * 2^-31 * 10^4 = cbrt(m) * 10^4 * 2^(31 - c)
    const std::uint64_t v { ((y >> 31U) * UINT64_C(10000)) >> (30 - c) };
    const auto md { cbrt_get_mode(neg) };
    const std::uint64_t z { cbrt_boundary(v, md) };
    constexpr std::uint64_t p12x8 { UINT64_C(8000000000000) };
    const std::uint64_t rem { m * p12x8 - z * z * z };
    return static_cast<std::uint32_t>(cbrt_pick(z, (rem >> 63U) != 0U, rem == 0U, md));
}

// decimal64_t: p = 16, w = 128.
constexpr auto cbrt_root(const std::uint64_t s, const int d, const bool neg) noexcept -> std::uint64_t
{
    const std::uint64_t m { s * pow10(static_cast<std::uint32_t>(d)) };
    const auto sc { cbrt_scale(m, d + (64 - countl_zero(s)) / 3) };
    const int c { sc.c };
    const std::uint64_t a { sc.a };
    // A first step on 32-bit words as for decimal32_t gives t * 2^63 within 2^-30
    const std::uint64_t t0 { cbrt_guess(a) >> 32U };
    const std::uint64_t at0 { (a >> 31U) * t0 };
    const std::uint64_t e0 { (UINT64_C(1) << 62U) + (UINT64_C(1) << 49U) - (at0 >> 31U) * ((t0 * t0) >> 30U) };
    const std::uint64_t t0_3 { t0 / 3U };
    const std::uint64_t t1 { (t0 << 32U) + ((t0_3 * (e0 >> 17U)) >> 13U) - (t0_3 << 19U) };
    // The last step as in cbrt_estimate, with |1 - a * t^3| < 2^-27: the small term on 32-bit words
    const std::uint64_t at { cbrt_mulshift(a, t1, 64) };
    const std::uint64_t e { (UINT64_C(1) << 63U) + (UINT64_C(1) << 37U) - cbrt_mulshift(at, cbrt_mulshift(t1, t1, 63), 60) };
    const std::uint64_t ky { (((at >> 30U) * (t1 >> 31U)) / 3U) >> 32U };
    const std::uint64_t y { cbrt_mulshift(at, t1, 61) + ((ky * (e >> 4U)) >> 26U) - (ky << 7U) };
    // y = cbrt(m) * 2^(62 - c), and y * 10^10 * 2^30 / 2^64 = cbrt(m) * 10^10 * 2^(28 - c)
    const std::uint64_t v { cbrt_mulshift(y, UINT64_C(10000000000) << 30U, 64) >> (27 - c) };
    const auto md { cbrt_get_mode(neg) };
    const std::uint64_t z { cbrt_boundary(v, md) };
    constexpr int128::uint128_t p30x8 { UINT64_C(0x64f964e682), UINT64_C(0x33a76f5200000000) };
    const int128::uint128_t rem { p30x8 * m - cbrt_mul(z, z) * z };
    return cbrt_pick(z, (rem >> 127U) != 0U, rem == 0U, md);
}

// 2 * cbrt(m * 10^66) = v + f / 2^64 for 10^33 <= m < 10^36 and a = m * 2^(125 - 3c) in [2^125, 2^128).
// The 64-bit estimate gets one more Newton step on y in 128 bits.
struct cbrt_est128
{
    int128::uint128_t v;
    std::uint64_t f;
};

constexpr auto cbrt_estimate128(const int128::uint128_t a, const int c) noexcept -> cbrt_est128
{
    const auto est { cbrt_estimate(a.high) };
    const std::uint64_t y { est.y };
    // y += (a - y^3) * t^2 / 3, a - y^3 in units of 2^-125
    const int128::uint128_t yy { cbrt_mul(y, y) };
    const int128::uint128_t y3s { (cbrt_mul(yy.high, y) << 3U) + (cbrt_mul(yy.low, y) >> 61U) };
    // |a - y^3| < 2^72, thus a - y^3 + 2^73 > 0 and the step needs no sign: t2_3 * 2^11 is the offset
    const int128::uint128_t res { a - y3s + (static_cast<int128::uint128_t>(1U) << 73U) };
    const std::uint64_t t2_3 { est.t2 / 3U };
    const int128::uint128_t corr { (cbrt_mul(res.high, t2_3) << 2U) + (cbrt_mul(res.low, t2_3) >> 62U) };
    const int128::uint128_t yc { int128::uint128_t { y, 0U } + corr - (static_cast<int128::uint128_t>(t2_3) << 11U) };
    // yc = cbrt(m) * 2^(126 - c), and yc * 5^22 / 2^(103 - c) = 2 * cbrt(m) * 10^22
    constexpr std::uint64_t p5_22 { UINT64_C(0x878678326eac9) };
    const int128::uint128_t lo { cbrt_mul(yc.low, p5_22) };
    const int128::uint128_t hi { cbrt_mul(yc.high, p5_22) + (lo >> 64U) };
    // Two shifts for the fraction, as sh can be 0
    const int sh { 39 - c };
    return { { hi.high >> sh, ((hi.high << 1U) << (63 - sh)) | (hi.low >> sh) },
             ((hi.low << 1U) << (63 - sh)) | (lo.low >> sh) };
}

// decimal128_t: p = 34, w = 256.
constexpr auto cbrt_root(const int128::uint128_t& s, const int d, const bool neg) noexcept -> int128::uint128_t
{
    const int128::uint128_t m { cbrt_mul(s, pow10(static_cast<std::uint32_t>(d))) };
    // s >= 10^33 > 2^109, thus the high word is not zero. As for 64 bits, c0 puts the top bit of a0 at 124 to 127.
    const int c0 { d + (128 - countl_zero(s.high)) / 3 };
    const int128::uint128_t a0 { m << (125 - 3 * c0) };
    const bool low { (a0 >> 125U) == 0U };
    const int c { c0 - static_cast<int>(low) };
    const auto est { cbrt_estimate128(a0 << (low ? 3U : 0U), c) };
    const auto md { cbrt_get_mode(neg) };
    const int128::uint128_t z { cbrt_boundary(est.v, md) };
    // k = z - v is 0 or 1: the estimate is above z by f, or below z by 2^64 - f. Its error is below 2^-4.5, thus
    // at more than 1/16 from z it gives the side of the root. Sums again, as the side is random.
    const std::uint64_t k { (z - est.v).low };
    const std::uint64_t dist { (est.f ^ (0U - k)) + k };
    if (dist > (UINT64_C(1) << 60U))
    {
        return cbrt_pick(z, k != 0U, false, md);
    }
    // 8 * m * 10^66 - z^3 mod 2^256
    constexpr int128::uint128_t p66x8_lo { UINT64_C(0x71e5dad62ba0e320), 0U };
    constexpr int128::uint128_t p66x8_hi { UINT64_C(0x4bf6ec38), UINT64_C(0xe7ed1d2b4bbac5f8) };
    const u256 n { umul256(m, p66x8_lo) + u256 { m * p66x8_hi, 0U } };
    const u256 zz { umul256(z, z) };
    const u256 rem { n - (umul256(static_cast<int128::uint128_t>(zz), z) + u256 { int128::uint128_t { zz[3], zz[2] } * z, 0U }) };
    return cbrt_pick(z, (rem[3] >> 63U) != 0U, rem == u256 { }, md);
}

template <typename T>
constexpr auto cbrt_impl(const T x) noexcept
    BOOST_DECIMAL_REQUIRES(detail::is_decimal_floating_point_v, T)
{
    #ifndef BOOST_DECIMAL_FAST_MATH
    if (isnan(x) || isinf(x))
    {
        return x;
    }
    #endif

    int e { };
    const auto s { frexp10(x, &e) };
    if (s == 0U)
    {
        return x;
    }

    // |x| = s * 10^e = m * 10^(3q + 2p - 2) with m = s * 10^d, so cbrt|x| = cbrt(m * 10^(2p - 2)) * 10^q.
    constexpr int p { std::numeric_limits<T>::digits10 };
    static_assert((2 * p - 2) % 3 == 0, "10^(2p - 2) must be a cube");
    // e + 3 * 2^12 > 0 for each type, and an unsigned divide by 3 is one multiply
    const auto u { static_cast<unsigned>(e + 3 * 4096) };
    const auto d { static_cast<int>(u % 3U) };
    const int q { static_cast<int>(u / 3U) - 4096 - (2 * p - 2) / 3 };
    const bool neg { signbit(x) };
    auto r { cbrt_root(s, d, neg) };
    int qr { q };
    // A root that rounds up to 10^p
    if (r > max_significand_v<T>)
    {
        r /= 10U;
        ++qr;
    }
    return pack_in_range<T>(r, qr, neg);
}

} //namespace detail

BOOST_DECIMAL_EXPORT template <typename T>
constexpr auto cbrt(const T val) noexcept // LCOV_EXCL_LINE
    BOOST_DECIMAL_REQUIRES(detail::is_decimal_floating_point_v, T)
{
    using evaluation_type = detail::evaluation_type_t<T>;

    return static_cast<T>(detail::cbrt_impl(static_cast<evaluation_type>(val)));
}

} //namespace decimal
} //namespace boost

#endif //BOOST_DECIMAL_DETAIL_CMATH_CBRT_HPP
