// Copyright 2023 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_DECIMAL_DETAIL_CMATH_LDEXP_HPP
#define BOOST_DECIMAL_DETAIL_CMATH_LDEXP_HPP

#include <boost/decimal/fwd.hpp>
#include <boost/decimal/detail/type_traits.hpp>
#include <boost/decimal/detail/concepts.hpp>
#include <boost/decimal/detail/config.hpp>
#include <boost/decimal/detail/power_tables.hpp>
#include <boost/decimal/detail/cmath/frexp.hpp>

#ifndef BOOST_DECIMAL_BUILD_MODULE
#include <type_traits>
#include <cmath>
#endif

namespace boost {
namespace decimal {

namespace detail {

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4127 4324)
#endif

// floor(L * log10(2) / 2^16), or one less, for |L| < 2^33. Each branch multiplies by the bound
// of log10(2) * 2^30 that keeps the product below the exact value.
constexpr auto ldexp_floor_log10_2(const std::int64_t L) noexcept -> int
{
    constexpr std::int64_t c_down { 323228496 };
    constexpr std::int64_t c_up { 323228497 };
    // For L < 0, floor(-a) = -ceil(a), so that no negative value is shifted.
    return L >= 0 ? static_cast<int>((L * c_down) >> 46) :
                    -static_cast<int>((-L * c_up + ((INT64_C(1) << 46) - 1)) >> 46);
}

// An overflow is the largest finite value or an infinity, by the rounding mode and the sign.
template <typename T, typename S>
constexpr auto ldexp_overflow(const bool neg) noexcept -> T
{
    // numeric_limits<T>::max calls the constructor of T, and pack_in_range of constants does not.
    constexpr int qmax { std::numeric_limits<T>::max_exponent10 - std::numeric_limits<T>::digits10 + 1 };
    if (overflow_is_finite(neg))
    {
        return pack_in_range<T>(static_cast<S>(max_significand_v<T>), qmax, neg);
    }
    return neg ? -std::numeric_limits<T>::infinity() : std::numeric_limits<T>::infinity();
}

// Packs sig with the exponent q. For an IEEE type, the clamps let the compiler drop the constructor
// that pack_in_range calls for an exponent out of range. A fast type keeps that constructor.
template <typename T, typename S>
constexpr auto ldexp_pack(const S sig, int q, const bool neg) noexcept -> T
{
    constexpr int p { std::numeric_limits<T>::digits10 };
    constexpr int qmin { std::numeric_limits<T>::min_exponent10 - p + 1 };
    constexpr int qmax { std::numeric_limits<T>::max_exponent10 - p + 1 };
    if (!detail::is_fast_type_v<T>)
    {
        if (q > qmax)
        {
            return ldexp_overflow<T, S>(neg);
        }
        q = q < qmin ? qmin : q;
    }
    return pack_in_range<T>(sig, q, neg);
}

// Drops the last of the n + 1 digits that the wide pass found when r has D0 + 1 digits.
constexpr auto ldexp_drop_digit(frexp_digits<std::uint64_t> dg) noexcept -> frexp_digits<std::uint64_t>
{
    dg.sticky = dg.sticky || dg.val % 10U != 0U;
    dg.val /= 10U;
    return dg;
}

constexpr auto ldexp_drop_digit(frexp_digits<int128::uint128_t> dg) noexcept -> frexp_digits<int128::uint128_t>
{
    // val is the parity of top and the digit for the bits below top, as frexp_split gives it.
    const auto d { static_cast<unsigned>(dg.top % 10U) };
    const auto below { static_cast<unsigned>(dg.val % 10U) != 0U };
    dg.top /= 10U;
    dg.val = (dg.top.low & 1U) * 10U + frexp_lift(d, below);
    return dg;
}

template <BOOST_DECIMAL_DECIMAL_FLOATING_TYPE T>
constexpr auto ldexp_impl(const T v, const int e2) noexcept -> T
{
    #ifndef BOOST_DECIMAL_FAST_MATH
    if (isnan(v) || isinf(v)) { return v; }
    #endif

    const auto comp { decompose(v) };
    if (comp.sig == 0U || e2 == 0) { return v; }

    constexpr int p { std::numeric_limits<T>::digits10 };
    constexpr int min10 { std::numeric_limits<T>::min_exponent10 };
    constexpr int max10 { std::numeric_limits<T>::max_exponent10 };
    constexpr bool subnormals { !detail::is_fast_type_v<T> };
    constexpr int qmin { min10 - p + 1 };
    // Past x_lim every result overflows or underflows, because 2^4 > 10.
    constexpr int x_lim { 4 * (max10 - min10 + 2 * p) };

    using sig_type = decltype(comp.sig);
    const bool neg { signbit(v) };
    const int q { static_cast<int>(comp.exp) };
    const int x { e2 > x_lim ? x_lim : e2 < -x_lim ? -x_lim : e2 };
    const int bits { static_cast<int>(sizeof(comp.sig) * 8U) - countl_zero(comp.sig) };

    // A short s and a small x give an exact r = s * 2^x below 10^p: s shifted left with the
    // exponent q, or s * 5^-x with the exponent q + x. A fast type has p digits in s, so it skips this.
    if (!detail::is_fast_type_v<T>)
    {
        constexpr int sbits { static_cast<int>(sizeof(comp.sig) * 8U) };
        // pow5 has the powers 5^0 to 5^27.
        if (x > 0 && bits + x < sbits)
        {
            const auto prod { static_cast<sig_type>(comp.sig << x) };
            if (prod <= max_significand_v<T>) { return ldexp_pack<T>(prod, q, neg); }
        }
        else if (x < 0 && x >= -27 && q + x >= qmin && bits + frexp_pow5_bits(-x) < sbits)
        {
            const auto prod { static_cast<sig_type>(comp.sig * static_cast<sig_type>(frexp_table::pow5[-x])) };
            if (prod <= max_significand_v<T>) { return ldexp_pack<T>(prod, q + x, neg); }
        }
    }

    // log2(s) >= bits - 1 + t, with t the 16 bits below the leading bit, because log2(1 + t) >= t.
    // r = v * 2^x has D digits before the decimal point, with D0 <= D <= D0 + 1.
    const auto t16 { static_cast<std::int64_t>((bits > 17 ? static_cast<std::uint64_t>(comp.sig >> (bits - 17)) :
                                                            static_cast<std::uint64_t>(comp.sig) << (17 - bits)) & 0xFFFFU) };
    const int D0 { ldexp_floor_log10_2(static_cast<std::int64_t>(bits - 1 + x) * 65536 + t16) + q + 1 };
    if (D0 > max10 + 1)
    {
        return ldexp_overflow<T, sig_type>(neg);
    }

    // The passes find n = p + 1 digits (decimal32, decimal64) or p digits (decimal128).
    constexpr int n_extra { p <= 16 ? 1 : 0 };
    constexpr int n { p + n_extra };

    // te is the exponent of the lowest digit that the passes find. A subnormal r keeps the digits
    // down to qmin, so te is at least qmin - n_extra. If D = D0 + 1, the rounding drops one more digit.
    int te { D0 - n };
    if (subnormals && D0 - p < qmin) { te = qmin - n_extra; }

    if (D0 < (subnormals ? qmin : min10) - 2)
    {
        // r is below 10^(qmin - 1): 0 or the smallest subnormal, by the rounding mode.
        // A fast type has no subnormals, and r is 0.
        std::uint64_t val { subnormals ? 1U : 0U };
        fenv_round<T>(val, neg, false);
        return ldexp_pack<T>(static_cast<sig_type>(val), subnormals ? qmin : 0, neg);
    }

    // Factors of 5 in s cancel 5^(k5 - n_extra) < 1. The power of 2 stays k5 + x.
    auto s { comp.sig };
    int k5 { q - te };
    const int k2 { k5 + x };
    frexp_cancel5<p>(s, k5, n_extra);
    const int bits_s { static_cast<int>(sizeof(s) * 8U) - countl_zero(s) };

    // The wide pass uses a word of 64 bits (decimal32), 128 bits (decimal64) or 256 bits (decimal128).
    using word = std::conditional_t<(p <= 7), std::uint64_t, std::conditional_t<(p <= 16), int128::uint128_t, u256>>;
    constexpr int w { static_cast<int>(sizeof(word) * 8U) };

    // decimal64 and decimal128 first try p digits in a word of half the width, as frexp does.
    if (p > 7)
    {
        using nword = std::conditional_t<(p <= 16), std::uint64_t, int128::uint128_t>;
        constexpr int nw { static_cast<int>(sizeof(nword) * 8U) };
        constexpr nword ten_p { pow10(static_cast<nword>(p)) };

        const int k5n { k5 - n_extra };
        nword Mn { frexp_load(s, bits_s, nword {}) };
        int En { bits_s - nw };
        // The products are exact if s * 5^k5n fits with one spare bit: k5n < 56 needs at most two entries,
        // and their bit lengths add up to at most bits(5^k5n) + 1.
        const bool exact { k5n >= 0 && bits_s + frexp_pow5_bits(k5n) < nw };
        const unsigned nm { frexp_mul_pow5_narrow<word>(Mn, En, k5n) };

        // N = Mn * 2^-shn holds the p digits. Mn is below the exact value by less than 6 * nm + 1 units.
        // nm <= 4, so 6 * nm + 1 < 2^5 and shn > 5 keeps the margin in the word.
        const int shn { -(En + k2 - n_extra) };
        if (shn > 5 && shn < nw)
        {
            const nword top { Mn >> shn };
            const nword frac { Mn << (nw - shn) };
            const nword half { nword { 1U } << (nw - 1) };
            const nword margin { nword { 6U * nm + 1U } << (nw - shn) };
            const bool extra { top >= ten_p };

            // The bits below the digits must not be near 0 or a carry, nor near one half
            // if the digits are p. With p + 1 digits, fenv_round drops the last digit.
            if (exact || (frac != nword { 0U } && frac < nword { 0U } - margin && (extra || frac > half || frac <= half - margin)))
            {
                if (extra)
                {
                    auto val { top };
                    const int r { fenv_round<T>(val, neg, frac != nword { 0U }) };
                    return ldexp_pack<T>(static_cast<sig_type>(val), te + n_extra + r, neg);
                }
                const nword sig { frexp_round_narrow<T>(top, frac, exact, neg) };
                const bool carry { sig == ten_p };
                return ldexp_pack<T>(static_cast<sig_type>(carry ? ten_p / 10U : sig), te + n_extra + (carry ? 1 : 0), neg);
            }
        }
    }

    word M { frexp_load(s, bits_s, word {}) };
    int E { bits_s - w };
    frexp_mul_pow5(M, E, k5);

    // c_bits is the bit length of 10^n, and c_norm is 10^n * 2^(w - c_bits).
    constexpr int c_bits { frexp_ten_n_bits(p) };
    constexpr word c_norm { frexp_cut(frexp_ten_n_256(p), word {}) };

    // N = M * 2^(E + k2) holds the digits, so it is M shifted right by sh.
    const int sh { -(E + k2) };
    const bool extra { w - sh > c_bits || (w - sh == c_bits && M >= c_norm) };

    if (sh >= w)
    {
        // 0 < N < 1. Without the extra digit, the digit below N is 1 (below one half),
        // 5 (one half) or 6 (above one half), and N >= 1/2 only if sh == w.
        constexpr word half_word { frexp_cut(u256 { UINT64_C(0x8000000000000000), 0U, 0U, 0U }, word {}) };
        std::uint64_t val { n_extra != 0 ? 0U : sh > w ? 1U : M == half_word ? 5U : 6U };
        fenv_round<T>(val, neg, n_extra != 0);
        return ldexp_pack<T>(static_cast<sig_type>(val), te + n_extra, neg);
    }

    auto dg { frexp_split(M, sh) };
    if (extra)
    {
        dg = ldexp_drop_digit(dg);
    }
    te += static_cast<int>(extra);
    auto val { dg.val };
    const int r { fenv_round<T>(val, neg, dg.sticky) };
    auto sig { dg.top };
    // If the digits rounded up to 10^p, they become 10^(p - 1) with te + 1.
    using sig_t = decltype(sig);
    constexpr sig_t ten_pm1 { pow10(static_cast<sig_t>(p - 1)) };
    const bool carry { frexp_finish(dg, val, r, sig) };
    return ldexp_pack<T>(static_cast<sig_type>(carry ? ten_pm1 : sig), te + n_extra + (carry ? 1 : 0), neg);
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif

} // namespace detail

BOOST_DECIMAL_EXPORT template <typename T>
constexpr auto ldexp(const T v, const int e2) noexcept
    BOOST_DECIMAL_REQUIRES(detail::is_decimal_floating_point_v, T)
{
    // A wider evaluation type rounds v * 2^e2 twice, so ldexp computes in T.
    return detail::ldexp_impl(v, e2);
}

} // namespace decimal
} // namespace boost

#endif // BOOST_DECIMAL_DETAIL_CMATH_LDEXP_HPP
