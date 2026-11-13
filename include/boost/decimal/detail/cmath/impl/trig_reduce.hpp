// Copyright 2026 Shen-Ta Hsieh
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_DECIMAL_DETAIL_CMATH_IMPL_TRIG_REDUCE_HPP
#define BOOST_DECIMAL_DETAIL_CMATH_IMPL_TRIG_REDUCE_HPP

#include <boost/decimal/fwd.hpp>
#include <boost/decimal/detail/config.hpp>
#include <boost/decimal/detail/int128.hpp>
#include <boost/decimal/detail/countl.hpp>
#include <boost/decimal/detail/power_tables.hpp>
#include <boost/decimal/detail/integer_search_trees.hpp>
#include <boost/decimal/detail/cmath/frexp10.hpp>
#include <boost/decimal/detail/cmath/impl/fixed_point.hpp>

#ifndef BOOST_DECIMAL_BUILD_MODULE
#include <cstddef>
#include <cstdint>
#include <limits>
#endif

namespace boost {
namespace decimal {
namespace detail {
namespace trig {

// Words of 2/pi in the product window, words of the fraction that go into r, and fixed-point words.
// The window keeps 9 * (window - 1 - frac) = 27, 45 and 81 digits for the leading zeros of the
// fraction: the worst cases have 9, 19 and 37.
// bin_frac: 64-bit fraction words for |x| < 10^19, where the fraction has at most 27, 58 and 117 leading zero bits.
template <typename T>
struct trig_traits;

template <>
struct trig_traits<decimal32_t> { static constexpr int window = 6; static constexpr int frac = 2; static constexpr int words = 1; static constexpr int bin_frac = 2; };

template <>
struct trig_traits<decimal_fast32_t> { static constexpr int window = 6; static constexpr int frac = 2; static constexpr int words = 1; static constexpr int bin_frac = 2; };

template <>
struct trig_traits<decimal64_t> { static constexpr int window = 9; static constexpr int frac = 3; static constexpr int words = 2; static constexpr int bin_frac = 3; };

template <>
struct trig_traits<decimal_fast64_t> { static constexpr int window = 9; static constexpr int frac = 3; static constexpr int words = 2; static constexpr int bin_frac = 3; };

template <>
struct trig_traits<decimal128_t> { static constexpr int window = 15; static constexpr int frac = 5; static constexpr int words = 3; static constexpr int bin_frac = 5; };

template <>
struct trig_traits<decimal_fast128_t> { static constexpr int window = 15; static constexpr int frac = 5; static constexpr int words = 3; static constexpr int bin_frac = 5; };

constexpr std::uint64_t word_base {UINT64_C(1000000000)};

// The most words of 9 digits in m * 10^shift, which has at most 38 + 8 digits
constexpr std::size_t m_words_max {6};

// The words of 2/pi in the table
constexpr int two_over_pi_size {static_cast<int>(sizeof(fx_table::two_over_pi) / sizeof(fx_table::two_over_pi[0]))};

// x = n*pi/2 + r (mod 2pi), with |r| <= pi/4.
template <int N>
struct trig_arg
{
    boost::int128::uint128_t sig; // |r| = sig * 10^-k, with sig of 38 digits
    int k;
    fx<N> rf;                     // |r| in fixed point, if fixed is set
    bool fixed;
    bool neg;                     // the sign of r, which includes the sign of x
    unsigned n;                   // the quadrant 0 to 3, which includes the sign of x
    bool zero;                    // x = 0: sig and k are not set
};

// |r| in fixed point: the binary reduction gives it, else it comes from sig and k.
template <int N>
constexpr auto fixed_r(const trig_arg<N>& r) noexcept -> fx<N>
{
    return r.fixed ? r.rf : fx_from_far<N>(r.sig, r.k);
}

// The significand in words of 9 digits, low word first; returns the count of words.
constexpr auto to_words(std::uint64_t m, const fx_span<std::uint64_t, m_words_max> out) noexcept -> int
{
    int n {};
    while (m != 0U)
    {
        out[n++] = m % word_base;
        m /= word_base;
    }
    return n;
}

constexpr auto to_words(const boost::int128::uint128_t& m, const fx_span<std::uint64_t, m_words_max> out) noexcept -> int
{
    constexpr std::uint64_t base2 {UINT64_C(1000000000000000000)};
    const auto hi {static_cast<std::uint64_t>(m / base2)};
    const auto lo {static_cast<std::uint64_t>(m % base2)};
    out[0] = lo % word_base;
    out[1] = lo / word_base;
    out[2] = hi % word_base;
    out[3] = hi / word_base;
    // Zero high words do not change the product.
    return 4;
}

// m * 10^shift in words of 9 digits, low word first, for 0 <= shift < 9; returns the words.
template <typename Significand>
constexpr auto scaled_words(const Significand m, const int shift, const fx_span<std::uint64_t, m_words_max> out) noexcept -> fx_span<std::uint64_t>
{
    int count {to_words(m, out)};
    const auto scale {pow10(static_cast<std::uint64_t>(shift))};
    std::uint64_t carry {};
    for (int i {}; i < count; ++i)
    {
        const std::uint64_t v {out[i] * scale + carry};
        out[i] = v % word_base;
        carry = v / word_base;
    }
    if (carry != 0U)
    {
        out[count++] = carry;
    }
    return out.first(count);
}

// The low Window words of m * 10^(9 * word_exp) * 2/pi, low word first. Word Window - 1 is the unit
// word, and 10^9 = 0 (mod 4), thus its low two bits are the quadrant and the higher words are not needed.
template <std::size_t Window>
constexpr auto times_two_over_pi(const fx_span<const std::uint64_t> m, const int word_exp, const fx_span<std::uint64_t, Window> out) noexcept -> void
{
    constexpr int window {static_cast<int>(Window)};
    const int count {static_cast<int>(m.size())};
    // two_over_pi[i] has the weight 10^(-9(i + 1)), thus word u of the window is table word Window - 2 - u + word_exp.
    std::uint64_t w[Window] {};
    for (int u {}; u < window; ++u)
    {
        const int i {window - 2 - u + word_exp};
        w[u] = (i >= 0 && i < two_over_pi_size) ? static_cast<std::uint64_t>(fx_table::two_over_pi[i]) : 0U;
    }

    std::uint64_t carry {};
    for (int col {}; col < window; ++col)
    {
        std::uint64_t acc {carry};
        for (int i {}; i < count && i <= col; ++i)
        {
            acc += m[i] * w[col - i];
        }
        out[col] = acc % word_base;
        carry = acc / word_base;
    }
}

// The fraction f in [0, 1) of Words words, high word first, to 1 - f when f >= 1/2; returns whether it did.
template <std::size_t Words>
constexpr auto fold_half(const fx_span<std::uint64_t, Words> f) noexcept -> bool
{
    constexpr int words {static_cast<int>(Words)};
    if (f[0] < word_base / 2U)
    {
        return false;
    }
    for (int i {}; i < words; ++i)
    {
        f[i] = word_base - 1U - f[i];
    }
    for (int i {words - 1}; i >= 0; --i)
    {
        if (++f[i] < word_base)
        {
            break;
        }
        f[i] = 0U;
    }
    return true;
}

// The Frac words of the fraction from word lead on, high word first, times pi/2: 2 * Frac + 1 words,
// low word first.
template <int Frac, std::size_t Words>
constexpr auto times_half_pi(const fx_span<std::uint64_t, Words> f, const int lead, const fx_span<std::uint64_t, static_cast<std::size_t>(2 * Frac + 1)> out) noexcept -> void
{
    std::uint64_t a[static_cast<std::size_t>(Frac)] {};
    std::uint64_t b[static_cast<std::size_t>(Frac + 1)] {};
    for (int u {}; u < Frac; ++u)
    {
        const int i {lead + Frac - 1 - u};
        a[u] = i < static_cast<int>(Words) ? f[i] : 0U;
    }
    for (int v {}; v <= Frac; ++v)
    {
        b[v] = static_cast<std::uint64_t>(fx_table::pio2[Frac - v]);
    }

    std::uint64_t carry {};
    for (int col {}; col <= 2 * Frac; ++col)
    {
        std::uint64_t acc {carry};
        for (int u {col - Frac < 0 ? 0 : col - Frac}; u < Frac && u <= col; ++u)
        {
            acc += a[u] * b[col - u];
        }
        out[col] = acc % word_base;
        carry = acc / word_base;
    }
}

// The first 38 digits of the nonzero words r, low word first, as sig * 10^exp10 with word 0 at 10^0.
constexpr auto first_38_digits(const fx_span<const std::uint64_t> r, int& exp10) noexcept -> boost::int128::uint128_t
{
    int top {static_cast<int>(r.size()) - 1};
    while (r[top] == 0U)
    {
        --top;
    }
    boost::int128::uint128_t sig {r[top]};
    int digits {num_digits(r[top])};
    exp10 = 9 * top;
    for (int i {top - 1}; i >= 0; --i)
    {
        if (digits + 9 <= 38)
        {
            sig = sig * word_base + r[i];
            digits += 9;
            exp10 -= 9;
        }
        else
        {
            const int take {38 - digits};
            const auto low {pow10(static_cast<std::uint64_t>(9 - take))};
            sig = sig * (word_base / low) + r[i] / low;
            exp10 -= take;
            break;
        }
    }

    const int sig_digits {num_digits(sig)};
    if (sig_digits < 38)
    {
        sig *= pow10(static_cast<boost::int128::uint128_t>(38 - sig_digits));
        exp10 -= 38 - sig_digits;
    }
    return sig;
}

// Payne-Hanek reduction in words of 9 decimal digits for x = m * 10^e: multiply m by the window of
// 2/pi which starts at the exponent, then the integer part mod 4 is the quadrant.
template <typename T, typename Significand>
constexpr auto trig_reduce(const Significand m, const int e, const bool xneg) noexcept -> trig_arg<trig_traits<T>::words>
{
    constexpr int words {trig_traits<T>::words};
    constexpr int window {trig_traits<T>::window};
    constexpr int frac {trig_traits<T>::frac};
    static_assert(window - 2 + std::numeric_limits<T>::max_exponent10 / 9 < two_over_pi_size, "2/pi must cover the largest exponent");

    // m * 10^e = (m * 10^shift) * 10^(9 * word_exp)
    const int shift {((e % 9) + 9) % 9};
    const int word_exp {(e - shift) / 9};
    std::uint64_t m_words[m_words_max] {};
    std::uint64_t prod[static_cast<std::size_t>(window)] {};
    times_two_over_pi(scaled_words(m, shift, fx_make_span(m_words)), word_exp, fx_make_span(prod));

    // The fraction, high word first. Above one half, go to the next quadrant with r < 0.
    std::uint64_t f[static_cast<std::size_t>(window - 1)] {};
    for (int i {}; i < window - 1; ++i)
    {
        f[i] = prod[window - 2 - i];
    }
    unsigned n {static_cast<unsigned>(prod[window - 1] & 3U)};
    bool rneg {fold_half(fx_make_span(f))};
    if (rneg)
    {
        n = (n + 1U) & 3U;
    }

    // -x = (-n)*pi/2 + (-r)
    if (xneg)
    {
        n = (4U - n) & 3U;
        rneg = !rneg;
    }

    trig_arg<words> out {};
    out.n = n;
    out.neg = rneg;

    // The fraction is not zero: its worst case has 9, 19 and 37 leading zero digits (see trig_traits).
    int lead {};
    while (lead < window - 1 && f[lead] == 0U)
    {
        ++lead;
    }

    std::uint64_t r[static_cast<std::size_t>(2 * frac + 1)] {};
    times_half_pi<frac>(fx_make_span(f), lead, fx_make_span(r));

    // Word 0 of r has the weight 10^-9(lead + 2 * frac).
    int exp10 {};
    out.sig = first_38_digits(fx_make_span(r), exp10);
    out.k = 9 * (lead + 2 * frac) - exp10;
    return out;
}

// The first count words of a times v, low word first, in place; returns the new count.
template <std::size_t M>
constexpr auto times_word(const fx_span<std::uint64_t, M> a, int count, const std::uint64_t v) noexcept -> int
{
    std::uint64_t carry {};
    for (int i {}; i < count; ++i)
    {
        a[i] = mul_add(a[i], v, 0U, carry);
    }
    if (carry != 0U)
    {
        a[count++] = carry;
    }
    return count;
}

// x = m * 10^e < 10^19 as Frac + 1 words, low word first, with word Frac as the integer part.
// For e < 0, x = (m * 10^b) * 10^(-9a) with 9a = b - e, and the table error is below 2^-300.
template <int Frac, typename Significand>
constexpr auto binary_words(const Significand m, const int e, const fx_span<std::uint64_t, static_cast<std::size_t>(Frac + 1)> x) noexcept -> void
{
    constexpr int cw {Frac + 2};
    const auto m128 {static_cast<boost::int128::uint128_t>(m)};
    if (e >= 0)
    {
        x[Frac] = m128.low * pow10(static_cast<std::uint64_t>(e));
        return;
    }

    const int a {(8 - e) / 9};
    std::uint64_t g[4] {m128.low, m128.high};
    const int count {times_word(fx_make_span(g), m128.high != 0U ? 2 : 1, pow10(static_cast<std::uint64_t>(9 * a + e)))};

    const auto& c {fx_table::pow10_neg9_bin[a - 1]};
    std::uint64_t p[static_cast<std::size_t>(cw + 3)] {};
    for (int i {}; i < count; ++i)
    {
        std::uint64_t carry {};
        for (int j {}; j < cw; ++j)
        {
            p[i + j] = mul_add(g[i], c[cw - 1 - j], p[i + j], carry);
        }
        p[i + cw] = carry;
    }
    for (int i {}; i <= Frac; ++i)
    {
        x[i] = p[cw - Frac + i];
    }
}

// Payne-Hanek reduction in words of 64 bits for |x| < 10^19. The relative errors of r are below 2^-64,
// 2^-128 and 2^-190, less than those of trig_reduce.
template <typename T, typename Significand>
constexpr auto trig_reduce_binary(const Significand m, const int e, const bool xneg) noexcept -> trig_arg<trig_traits<T>::words>
{
    constexpr int words {trig_traits<T>::words};
    constexpr int frac {trig_traits<T>::bin_frac};
    constexpr int tw {frac + 2};
    constexpr int rw {words + 1};

    std::uint64_t x[static_cast<std::size_t>(frac + 1)] {};
    binary_words<frac>(m, e, fx_make_span(x));
    trig_arg<words> out {};

    // q = x * 2/pi with tw words of 2/pi: word 2 * frac + 2 is the integer part. The columns below
    // frac only carry into words that are dropped.
    std::uint64_t q[static_cast<std::size_t>(2 * frac + 3)] {};
    for (int i {}; i <= frac; ++i)
    {
        std::uint64_t carry {};
        for (int j {frac - i < 0 ? 0 : frac - i}; j < tw; ++j)
        {
            q[i + j] = mul_add(x[i], fx_table::two_over_pi_bin[tw - 1 - j], q[i + j], carry);
        }
        q[i + tw] = carry;
    }

    // The fraction, high word first. Above one half, go to the next quadrant with r = -(1 - f).
    std::uint64_t f[static_cast<std::size_t>(frac)] {};
    for (int i {}; i < frac; ++i)
    {
        f[i] = q[2 * frac + 1 - i];
    }
    unsigned n {static_cast<unsigned>(q[2 * frac + 2] & 3U)};
    bool rneg {(f[0] >> 63U) != 0U};
    if (rneg)
    {
        std::uint64_t carry {1};
        for (int i {frac - 1}; i >= 0; --i)
        {
            f[i] = ~f[i] + carry;
            carry = (carry != 0U && f[i] == 0U) ? 1U : 0U;
        }
        n = (n + 1U) & 3U;
    }
    if (xneg)
    {
        n = (4U - n) & 3U;
        rneg = !rneg;
    }
    out.n = n;
    out.neg = rneg;

    // The fraction is not zero: its worst case has 27, 58 and 117 leading zero bits (see trig_traits).
    int lead {};
    while (lead < frac && f[lead] == 0U)
    {
        ++lead;
    }

    // r = (rw words of f from word lead) * pi/2 = (s from word rw) * 2^-64(lead + rw). The four spare
    // words take the scale by 10^k below.
    std::uint64_t s[static_cast<std::size_t>(2 * rw + 6)] {};
    for (int u {}; u < rw; ++u)
    {
        const int i {lead + rw - 1 - u};
        const std::uint64_t a {i < frac ? f[i] : 0U};
        std::uint64_t carry {};
        for (int v {}; v <= rw; ++v)
        {
            s[u + v] = mul_add(a, fx_table::pio2_bin[rw - v], s[u + v], carry);
        }
        s[u + rw + 1] = carry;
    }
    const auto r {fx_make_span<rw + 6>(s, rw)};

    // rf = r * 2^(64N - 2): drop lead + 1 words and 2 bits.
    for (int i {}; i < words; ++i)
    {
        const int w {i + lead + 1};
        const std::uint64_t lo {w <= rw ? r[w] : 0U};
        const std::uint64_t hi {w + 1 <= rw ? r[w + 1] : 0U};
        out.rf.w[i] = (lo >> 2U) | (hi << 62U);
    }
    out.fixed = true;

    // For r in [2^-(z + 1), 2^-z), sig = floor(r * 10^k) with k = 38 + floor(z * log10(2)) has 37 or 38 digits.
    int count {rw + 1};
    while (r[count - 1] == 0U)
    {
        --count;
    }
    const int z {64 * (lead + rw - count) + countl_zero(r[count - 1])};
    int k {38 + ((z * 78913) >> 18)};
    int left {k};
    while (left >= 19)
    {
        count = times_word(r, count, pow10(static_cast<std::uint64_t>(19)));
        left -= 19;
    }
    count = times_word(r, count, pow10(static_cast<std::uint64_t>(left)));
    const int unit {lead + rw};
    out.sig = boost::int128::uint128_t {unit + 1 < count ? r[unit + 1] : 0U, unit < count ? r[unit] : 0U};
    if (out.sig < pow10(static_cast<boost::int128::uint128_t>(37)))
    {
        out.sig *= 10U;
        ++k;
    }
    out.k = k;
    return out;
}

// x = sig * 10^-k with sig of 38 digits; for |x| <= pi/4 this is r itself.
template <typename T>
constexpr auto trig_prepare(const T x) noexcept -> trig_arg<trig_traits<T>::words>
{
    constexpr int words {trig_traits<T>::words};

    // floor(pi/4 * 10^38) = 78539816339744830961566084581987572104
    constexpr boost::int128::uint128_t quarter_pi {UINT64_C(4257651975108193236), UINT64_C(12340784068543502728)};

    trig_arg<words> out {};
    const bool neg {signbit(x)};
    int e {};
    const auto m {frexp10(neg ? -x : x, &e)};
    out.neg = neg;
    if (m == 0U)
    {
        out.zero = true;
        return out;
    }

    const auto digits {num_digits(m)};
    const auto sig {static_cast<boost::int128::uint128_t>(m) * pow10(static_cast<boost::int128::uint128_t>(38 - digits))};
    const int k {(38 - digits) - e};
    if (k > 38 || (k == 38 && sig <= quarter_pi))
    {
        out.sig = sig;
        out.k = k;
        return out;
    }
    // |x| < 10^digits * 10^e
    if (digits + e <= 19)
    {
        return trig_reduce_binary<T>(m, e, neg);
    }
    return trig_reduce<T>(m, e, neg);
}

} // namespace trig
} // namespace detail
} // namespace decimal
} // namespace boost

#endif // BOOST_DECIMAL_DETAIL_CMATH_IMPL_TRIG_REDUCE_HPP
