// Copyright 2023 - 2024 Matt Borland
// Copyright 2023 - 2024 Christopher Kormanyos
// Copyright 2025 - 2026 Justin Zhu
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_DECIMAL_DETAIL_CMATH_SQRT_HPP
#define BOOST_DECIMAL_DETAIL_CMATH_SQRT_HPP

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
#include <cstddef>
#include <cstdint>
#include <limits>
#endif

namespace boost {
namespace decimal {

namespace detail {

// recip: 2^31 / sqrt(1 + i/16) for i = 0 ... 48. bulge: 2^33 times the gap between the chord and the curve
// at the middle of each step. Both rounded.
template <bool b>
struct sqrt_table_imp
{
    static constexpr std::uint32_t recip[49] = {
        UINT32_C(2147483648), UINT32_C(2083365155), UINT32_C(2024667000), UINT32_C(1970666148), UINT32_C(1920767767), UINT32_C(1874477404),
        UINT32_C(1831380208), UINT32_C(1791125178), UINT32_C(1753413056), UINT32_C(1717986918), UINT32_C(1684624773), UINT32_C(1653133683),
        UINT32_C(1623345051), UINT32_C(1595110809), UINT32_C(1568300315), UINT32_C(1542797797), UINT32_C(1518500250), UINT32_C(1495315679),
        UINT32_C(1473161629), UINT32_C(1451963954), UINT32_C(1431655765), UINT32_C(1412176548), UINT32_C(1393471397), UINT32_C(1375490368),
        UINT32_C(1358187913), UINT32_C(1341522400), UINT32_C(1325455684), UINT32_C(1309952745), UINT32_C(1294981364), UINT32_C(1280511845),
        UINT32_C(1266516759), UINT32_C(1252970736), UINT32_C(1239850262), UINT32_C(1227133513), UINT32_C(1214800200), UINT32_C(1202831433),
        UINT32_C(1191209601), UINT32_C(1179918260), UINT32_C(1168942037), UINT32_C(1158266544), UINT32_C(1147878294), UINT32_C(1137764631),
        UINT32_C(1127913670), UINT32_C(1118314230), UINT32_C(1108955787), UINT32_C(1099828424), UINT32_C(1090922784), UINT32_C(1082230034),
        UINT32_C(1073741824),
    };
    static constexpr std::uint32_t bulge[48] = {
        UINT32_C(2914756), UINT32_C(2515849), UINT32_C(2189390), UINT32_C(1919300), UINT32_C(1693655), UINT32_C(1503475),
        UINT32_C(1341903), UINT32_C(1203635), UINT32_C(1084521), UINT32_C(981279), UINT32_C(891290), UINT32_C(812445),
        UINT32_C(743030), UINT32_C(681643), UINT32_C(627129), UINT32_C(578528), UINT32_C(535042), UINT32_C(495998),
        UINT32_C(460829), UINT32_C(429055), UINT32_C(400266), UINT32_C(374110), UINT32_C(350286), UINT32_C(328533),
        UINT32_C(308626), UINT32_C(290367), UINT32_C(273585), UINT32_C(258131), UINT32_C(243872), UINT32_C(230691),
        UINT32_C(218487), UINT32_C(207168), UINT32_C(196653), UINT32_C(186870), UINT32_C(177756), UINT32_C(169252),
        UINT32_C(161306), UINT32_C(153874), UINT32_C(146912), UINT32_C(140383), UINT32_C(134253), UINT32_C(128492),
        UINT32_C(123071), UINT32_C(117964), UINT32_C(113150), UINT32_C(108606), UINT32_C(104314), UINT32_C(100255),
    };
};

#if !(defined(__cpp_inline_variables) && __cpp_inline_variables >= 201606L) && (!defined(_MSC_VER) || _MSC_VER != 1900)

template <bool b> constexpr std::uint32_t sqrt_table_imp<b>::recip[49];
template <bool b> constexpr std::uint32_t sqrt_table_imp<b>::bulge[48];

#endif

using sqrt_table = sqrt_table_imp<true>;

// The root y = sqrt(a / 2^62) * 2^61 and its reciprocal t = 2^63 / sqrt(a / 2^62) for a in [2^62, 2^64)
struct sqrt_est
{
    std::uint64_t y;
    std::uint64_t t;
};

// t from a chord between two table points less a parabola (error < 2^-18), and y = a * t. Each Goldschmidt step
// y += y * e / 2 and t += t * e / 2 with e = 1 - y * t squares the error of both, with two products in a chain.
constexpr auto sqrt_estimate(const std::uint64_t a, const int steps) noexcept -> sqrt_est
{
    const std::uint64_t i { (a >> 58U) - 16U };
    const std::uint64_t f { (a >> 42U) & UINT64_C(0xFFFF) };
    // MSVC 19.44 crashes in constant evaluation if the table values convert without a cast
    const std::uint64_t k0 { static_cast<std::uint64_t>(sqrt_table::recip[i]) };
    const std::uint64_t k1 { static_cast<std::uint64_t>(sqrt_table::recip[i + 1U]) };
    std::uint64_t t { (k0 << 32U) - (((k0 - k1) * f) << 16U) - sqrt_table::bulge[i] * (f * (UINT64_C(0x10000) - f)) };
    std::uint64_t y { umul128(a, t).high };
    // 1 + 2^-10 in units of 2^-60, so e = 1 - y * t + 2^-10 > 0 needs no sign test. e in units of 2^-63 makes
    // the high word of k * e equal to k * e / 2, with the offset k * 2^-11 that the step takes away as k >> 11.
    constexpr std::uint64_t one { (UINT64_C(1) << 60U) + (UINT64_C(1) << 50U) };
    for (int n { 0 }; n < steps; ++n)
    {
        const std::uint64_t e { (one - umul128(y, t).high) << 3U };
        y += umul128(y, e).high - (y >> 11U);
        t += umul128(t, e).high - (t >> 11U);
    }
    return { y, t };
}

// IEEE 754-2019 4.3: the root of N = s * 10^k rounded once in the current mode. z = floor(sqrt(N)) from an
// estimate within one unit, then rem = N - z^2 picks the result. decimal32_t: N < 10^14, one word.
constexpr auto sqrt_root(const std::uint32_t s, const int k) noexcept -> std::uint32_t
{
    const std::uint64_t n { static_cast<std::uint64_t>(s) * pow10(static_cast<std::uint64_t>(k)) };
    // a = n * 2^sh in [2^62, 2^64) with an even sh, so y = sqrt(a) * 2^30 = sqrt(n) * 2^(30 + sh/2)
    const int sh { countl_zero(n) & ~1 };
    const std::uint64_t a { n << sh };
    // A check of all significands shows that the estimate is the floor or one below it
    std::uint64_t z { sqrt_estimate(a, 1).y >> (30 + sh / 2) };
    std::uint64_t rem { n - z * z };
    if (rem > 2U * z)
    {
        rem -= 2U * z + 1U;
        ++z;
    }
    return static_cast<std::uint32_t>(z + static_cast<std::uint64_t>(sqrt_steps_up(rem != 0U, rem > z)));
}

// decimal64_t: N < 10^32, a = the top 63 or 64 bits of N with an even shift r. z is within one unit of the floor, thus
// N - z^2 is small and its low word holds it.
constexpr auto sqrt_root(const std::uint64_t s, const int k) noexcept -> std::uint64_t
{
    const int128::uint128_t n { umul128(s, pow10(static_cast<std::uint64_t>(k))) };
    const int r { (65 - countl_zero(n.high)) & ~1 };
    const auto a { static_cast<std::uint64_t>(n >> r) };
    std::uint64_t z { sqrt_estimate(a, 2).y >> (30 - r / 2) };
    std::uint64_t rem { n.low - z * z };
    if ((rem >> 63U) != 0U)
    {
        --z;
        rem += 2U * z + 1U;
    }
    else if (rem > 2U * z)
    {
        rem -= 2U * z + 1U;
        ++z;
    }
    return z + static_cast<std::uint64_t>(sqrt_steps_up(rem != 0U, rem > z));
}

// The 64 bits of x from bit pos up, for pos in [0, 192), from two words: a general shift goes through memory
constexpr auto sqrt_bits(const u256& x, const int pos) noexcept -> std::uint64_t
{
    const auto w { static_cast<std::size_t>(pos / 64) };
    const int b { pos % 64 };
    return b == 0 ? x[w] : (x[w] >> b) | (x[w + 1U] << (64 - b));
}

// decimal128_t: N < 10^68. The 64-bit estimate gets one Newton step z += (N - z^2) / (2 * sqrt(N)), where
// 1 / (2 * sqrt(N)) = t * 2^-(95 + r/2), on 128 bits of N - z^2. y and t are within 2^-59.5, thus the step is
// within R * 2^-120 + R * 2^-119 < 0.024 of the root R < 2^113, and the fraction f decides the result unless the
// root is within 2^-5 of an integer or a half. Else z is within one unit, and the low 128 bits hold N - z^2.
constexpr auto sqrt_root(const int128::uint128_t s, const int k) noexcept -> int128::uint128_t
{
    const u256 n { umul256(s, pow10(static_cast<int128::uint128_t>(k))) };
    const int r { (193 - countl_zero(n[3])) & ~1 };
    const std::uint64_t a { sqrt_bits(n, r) };
    const auto est { sqrt_estimate(a, 2) };
    const std::uint64_t t { est.t };
    // z = y * 2^h with a 64-bit y and h in [48, 51], thus z^2 = y^2 * 2^(64 + b) with b in [32, 38]
    const std::uint64_t y { est.y };
    const int h { r / 2 - 30 };
    const int b { 2 * h - 64 };
    int128::uint128_t z { static_cast<int128::uint128_t>(y) << h };
    const int128::uint128_t yy { umul128(y, y) };
    // The u256 constructor takes the words high first
    const u256 zz { yy.high >> (64 - b), (yy.high << b) | (yy.low >> (64 - b)), yy.low << b, UINT64_C(0) };
    const bool below { n > zz };
    const u256 d { below ? n - zz : zz - n };
    // d * t * 2^-(95 + r/2) = (d >> sd) * t * 2^-64: df.high is the step and df.low its fraction
    const int sd { 31 + r / 2 };
    const int128::uint128_t df { umul128(sqrt_bits(d, sd), t) + int128::uint128_t { umul128(sqrt_bits(d, sd - 64), t).high } };
    std::uint64_t f { df.low };
    if (below)
    {
        z += df.high;
    }
    else
    {
        z -= df.high + static_cast<std::uint64_t>(f != 0U);
        f = UINT64_C(0) - f;
    }
    // f mod 2^64 is outside [-win, win) and outside [half - win, half + win)
    constexpr std::uint64_t win { UINT64_C(1) << 59U };
    constexpr std::uint64_t half { UINT64_C(1) << 63U };
    if (f + win >= 2U * win && f + win - half >= 2U * win)
    {
        return z + static_cast<int128::uint128_t>(sqrt_steps_up(true, f > half));
    }
    int128::uint128_t rem { int128::uint128_t { n[1], n[0] } - z * z };
    if ((rem.high >> 63U) != 0U)
    {
        --z;
        rem += (z << 1U) + 1U;
    }
    else if (rem > (z << 1U))
    {
        rem -= (z << 1U) + 1U;
        ++z;
    }
    return z + static_cast<int128::uint128_t>(sqrt_steps_up(rem != 0U, rem > z));
}

template <typename T>
constexpr auto sqrt_impl(const T x) noexcept
    BOOST_DECIMAL_REQUIRES(detail::is_decimal_floating_point_v, T)
{
    #ifndef BOOST_DECIMAL_FAST_MATH
    const auto fpc = fpclassify(x);
    if ((fpc == FP_NAN) || (fpc == FP_ZERO))
    {
        return x;
    }
    if (signbit(x))
    {
        return std::numeric_limits<T>::quiet_NaN();
    }
    if (fpc == FP_INFINITE)
    {
        return std::numeric_limits<T>::infinity();
    }
    #else
    if (signbit(x))
    {
        return T{0};
    }
    #endif

    // x = s * 10^e = N * 10^(e - k) with e - k even and N = s * 10^k of 2p - 1 or 2p digits
    constexpr int p { std::numeric_limits<T>::digits10 };
    int e { };
    const auto s { frexp10(x, &e) };
    #ifdef BOOST_DECIMAL_FAST_MATH
    // frexp10 gives 0 for any zero, and the root search needs a nonzero N
    if (s == 0U)
    {
        return T{0};
    }
    #endif
    const int k { (e - p + 1) % 2 == 0 ? p - 1 : p };

    // x = 10^(2m) gives 1 * 10^m, not a significand of p digits
    if (k == p - 1 && s == pow10(static_cast<decltype(s)>(p - 1)))
    {
        return T { 1, (e + p - 1) / 2 };
    }
    auto r { sqrt_root(s, k) };
    int qr { (e - k) / 2 };
    // A root that rounds up to 10^p
    if (r > max_significand_v<T>)
    {
        r /= 10U;
        ++qr;
    }
    return pack_in_range<T>(r, qr, false);
}

} // namespace detail

BOOST_DECIMAL_EXPORT template <typename T>
constexpr auto sqrt(T val) noexcept
    BOOST_DECIMAL_REQUIRES(detail::is_decimal_floating_point_v, T)
{
    using evaluation_type = detail::evaluation_type_t<T>;
    return static_cast<T>(detail::sqrt_impl(static_cast<evaluation_type>(val)));
}

} // namespace decimal
} // namespace boost

#endif // BOOST_DECIMAL_DETAIL_CMATH_SQRT_HPP
