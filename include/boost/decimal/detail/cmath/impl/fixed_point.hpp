// Copyright 2026 Shen-Ta Hsieh
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_DECIMAL_DETAIL_CMATH_IMPL_FIXED_POINT_HPP
#define BOOST_DECIMAL_DETAIL_CMATH_IMPL_FIXED_POINT_HPP

#include <boost/decimal/fwd.hpp>
#include <boost/decimal/detail/config.hpp>
#include <boost/decimal/detail/int128.hpp>
#include <boost/decimal/detail/power_tables.hpp>
#include <boost/decimal/detail/attributes.hpp>
#include <boost/decimal/detail/fenv_rounding.hpp>
#include <boost/decimal/detail/add_impl.hpp>
#include <boost/decimal/detail/cmath/impl/fixed_point_tables.hpp>

#ifndef BOOST_DECIMAL_BUILD_MODULE
#include <cstddef>
#include <cstdint>
#include <type_traits>
#endif

// GCC unrolls the short loops over the words only at -O3, which makes -O2 two times slower, and Clang without the
// request keeps some of their products in memory
#if defined(__GNUC__) && !defined(__clang__) && __GNUC__ >= 8
#  define BOOST_DECIMAL_FX_UNROLL _Pragma("GCC unroll 8")
#elif defined(__clang__)
#  define BOOST_DECIMAL_FX_UNROLL _Pragma("clang loop unroll(full)")
#else
#  define BOOST_DECIMAL_FX_UNROLL
#endif

namespace boost {
namespace decimal {
namespace detail {

#ifdef _MSC_VER
#  pragma warning(push)
#  pragma warning(disable : 4127) // Conditional expression is constant
#endif

constexpr std::size_t fx_dynamic_extent {static_cast<std::size_t>(-1)};

// The words of fx for a precision of p digits
constexpr auto fx_words(const int p) noexcept -> int { return p <= 7 ? 1 : p <= 16 ? 2 : 3; }

// One word gives 19 digits of a result, below 10^(scale10 + 1), and two or three words give 38
constexpr auto fx_scale10(const int words) noexcept -> int { return words == 1 ? 18 : 37; }

template <int words>
using fx_digits_t = std::conditional_t<words == 1, std::uint64_t, boost::int128::uint128_t>;

// A view of Extent elements of an array, as std::span: the size is in the type, or held for fx_dynamic_extent
template <typename Element, std::size_t Extent = fx_dynamic_extent>
class fx_span
{
private:
    Element* data_;

public:
    constexpr explicit fx_span(Element* data) noexcept : data_ {data} {}

    constexpr auto data() const noexcept -> Element* { return data_; }
    static constexpr auto size() noexcept -> std::size_t { return Extent; }

    template <typename Index>
    constexpr auto operator[](const Index i) const noexcept -> Element& { return data_[i]; }

    // The first count elements
    constexpr auto first(const int count) const noexcept -> fx_span<Element> { return {data_, static_cast<std::size_t>(count)}; }
};

template <typename Element>
class fx_span<Element, fx_dynamic_extent>
{
private:
    Element* data_;
    std::size_t size_;

public:
    constexpr fx_span(Element* data, const std::size_t size) noexcept : data_ {data}, size_ {size} {}

    // The same elements with the size held, and as const
    template <typename Other, std::size_t OtherExtent, std::enable_if_t<std::is_convertible<Other*, Element*>::value, bool> = true>
    constexpr fx_span(const fx_span<Other, OtherExtent> other) noexcept : data_ {other.data()}, size_ {other.size()} {}

    constexpr auto data() const noexcept -> Element* { return data_; }
    constexpr auto size() const noexcept -> std::size_t { return size_; }

    template <typename Index>
    constexpr auto operator[](const Index i) const noexcept -> Element& { return data_[i]; }
};

// The Count elements of the array a from offset, or all of them
template <int Count, typename Element, std::size_t M>
constexpr auto fx_make_span(Element (&a)[M], const int offset = 0) noexcept -> fx_span<Element, static_cast<std::size_t>(Count)>
{
    static_assert(0 <= Count && static_cast<std::size_t>(Count) <= M, "Count fits in the array");
    BOOST_DECIMAL_ASSERT(0 <= offset && static_cast<std::size_t>(offset + Count) <= M);
    return fx_span<Element, static_cast<std::size_t>(Count)> {a + offset};
}

template <typename Element, std::size_t M>
constexpr auto fx_make_span(Element (&a)[M]) noexcept -> fx_span<Element, M>
{
    return fx_span<Element, M> {a};
}

// The count elements of the array a from offset, with the count known at run time
template <typename Element, std::size_t M>
constexpr auto fx_make_span(Element (&a)[M], const int offset, const int count) noexcept -> fx_span<Element>
{
    BOOST_DECIMAL_ASSERT(0 <= offset && 0 <= count && static_cast<std::size_t>(offset + count) <= M);
    return fx_span<Element> {a + offset, static_cast<std::size_t>(count)};
}

// a * b + x + carry: returns the low word and puts the high word in carry. GCC keeps these words in registers,
// but not always the two halves of a uint128_t.
constexpr auto mul_add(const std::uint64_t a, const std::uint64_t b, const std::uint64_t x, std::uint64_t& carry) noexcept -> std::uint64_t
{
    std::uint64_t hi {};
    std::uint64_t lo {boost::int128::detail::umul(a, b, hi)};
    lo += x;
    hi += lo < x ? 1U : 0U;
    lo += carry;
    hi += lo < carry ? 1U : 0U;
    carry = hi;
    return lo;
}

// The four words of a * b, low word first, with mul_add: GCC keeps them in registers, but not those of umul256
constexpr auto mul_words(const boost::int128::uint128_t& a, const boost::int128::uint128_t& b, std::uint64_t (&r)[4]) noexcept -> void
{
    const std::uint64_t x[2] {a.low, a.high};
    for (int i {}; i < 2; ++i)
    {
        std::uint64_t carry {};
        r[i] = mul_add(x[i], b.low, i == 0 ? 0U : r[i], carry);
        r[i + 1] = mul_add(x[i], b.high, i == 0 ? 0U : r[i + 1], carry);
        r[i + 2] = carry;
    }
}

// p += a * b, and the carry out of the last word of p. Store the carry in a statement of its own: GCC 8 at -O1
// and above gives wrong words for p[k] = fx_mul_add(p, ...).
template <std::size_t Extent, typename Word>
BOOST_DECIMAL_FORCE_INLINE constexpr auto fx_mul_add(const fx_span<std::uint64_t, Extent> p, const fx_span<Word, Extent> b, const std::uint64_t a) noexcept -> std::uint64_t
{
    std::uint64_t c {};
    BOOST_DECIMAL_FX_UNROLL
    for (std::size_t j {}; j < Extent; ++j)
    {
        p[j] = mul_add(a, b[j], p[j], c);
    }
    return c;
}

// p += b, and the carry out of the last word of p
template <std::size_t Extent, typename Word>
BOOST_DECIMAL_FORCE_INLINE constexpr auto fx_add_to(const fx_span<std::uint64_t, Extent> p, const fx_span<Word, Extent> b) noexcept -> std::uint64_t
{
    std::uint64_t c {};
    BOOST_DECIMAL_FX_UNROLL
    for (std::size_t j {}; j < Extent; ++j)
    {
        const std::uint64_t t {p[j] + b[j]};
        const std::uint64_t u {t + c};
        c = (t < p[j] ? 1U : 0U) + (u < t ? 1U : 0U);
        p[j] = u;
    }
    return c;
}

// r = the words of a with the word top above them, shifted right by 0 < s < 64. The two parts of a word have no
// common bits, and a sum in place of an or keeps GCC from shrd, which is slower.
template <std::size_t Extent, typename Word>
BOOST_DECIMAL_FORCE_INLINE constexpr auto fx_shr_words(const fx_span<std::uint64_t, Extent> r, const fx_span<Word, Extent> a, const std::uint64_t top, const unsigned s) noexcept -> void
{
    BOOST_DECIMAL_ASSERT(0U < s && s < 64U);
    BOOST_DECIMAL_FX_UNROLL
    for (std::size_t i {}; i < Extent; ++i)
    {
        r[i] = (a[i] >> s) + ((i + 1U < r.size() ? a[i + 1U] : top) << (64U - s));
    }
}

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

// a * b, truncated. The products below word N - 1 are left out, which takes off less
// than 4(N - 1) units in the last place.
template <int N>
BOOST_DECIMAL_FORCE_INLINE constexpr auto fx_mul_body(const fx<N>& a, const fx<N>& b) noexcept -> fx<N>
{
    // One word without the array p, which GCC can keep in memory when it inlines this into a long function
    if (N == 1)
    {
        std::uint64_t hi {};
        const auto lo {boost::int128::detail::umul(a.w[0], b.w[0], hi)};
        fx<N> r {};
        r.w[0] = (lo >> 62U) + (hi << 2U);
        return r;
    }
    std::uint64_t p[static_cast<std::size_t>(2 * N)] {};
    BOOST_DECIMAL_FX_UNROLL
    for (int i {}; i < N; ++i)
    {
        const int j {i < N - 1 ? N - 1 - i : 0};
        std::uint64_t c {};
        BOOST_DECIMAL_FX_UNROLL
        for (int k {j}; k < N; ++k)
        {
            p[i + k] = mul_add(a.w[i], b.w[k], p[i + k], c);
        }
        p[i + N] = c;
    }
    fx<N> r {};
    fx_shr_words(fx_make_span(r.w), fx_make_span<N>(p, N - 1), p[2 * N - 1], 62U);
    return r;
}

// One and two words go in registers, so they are passed by value and not forced inline.
// Three words go through memory, so they are passed by reference and inlined.
template <int N>
constexpr auto fx_mul(const fx<N> a, const fx<N> b) noexcept -> std::enable_if_t<(N < 3), fx<N>>
{
    return fx_mul_body(a, b);
}

template <int N>
BOOST_DECIMAL_FORCE_INLINE constexpr auto fx_mul(const fx<N>& a, const fx<N>& b) noexcept -> std::enable_if_t<(N >= 3), fx<N>>
{
    return fx_mul_body(a, b);
}

// a - b modulo 2^(64N), which is a - b for a >= b.
template <int N>
BOOST_DECIMAL_FORCE_INLINE constexpr auto fx_sub_body(const fx<N>& a, const fx<N>& b) noexcept -> fx<N>
{
    fx<N> r {};
    std::uint64_t borrow {};
    BOOST_DECIMAL_FX_UNROLL
    for (int i {}; i < N; ++i)
    {
        const std::uint64_t t {a.w[i] - b.w[i] - borrow};
        borrow = (a.w[i] < b.w[i] || (a.w[i] == b.w[i] && borrow != 0U)) ? 1U : 0U;
        r.w[i] = t;
    }
    return r;
}

// The arguments are passed as for fx_mul.
template <int N>
constexpr auto fx_sub(const fx<N> a, const fx<N> b) noexcept -> std::enable_if_t<(N < 3), fx<N>>
{
    return fx_sub_body(a, b);
}

template <int N>
BOOST_DECIMAL_FORCE_INLINE constexpr auto fx_sub(const fx<N>& a, const fx<N>& b) noexcept -> std::enable_if_t<(N >= 3), fx<N>>
{
    return fx_sub_body(a, b);
}

// a + b modulo 2^(64N), which is a + b for a + b < 4.
template <int N>
BOOST_DECIMAL_FORCE_INLINE constexpr auto fx_add_body(const fx<N>& a, const fx<N>& b) noexcept -> fx<N>
{
    fx<N> r {a};
    fx_add_to(fx_make_span(r.w), fx_make_span(b.w));
    return r;
}

// The arguments are passed as for fx_mul.
template <int N>
constexpr auto fx_add(const fx<N> a, const fx<N> b) noexcept -> std::enable_if_t<(N < 3), fx<N>>
{
    return fx_add_body(a, b);
}

template <int N>
BOOST_DECIMAL_FORCE_INLINE constexpr auto fx_add(const fx<N>& a, const fx<N>& b) noexcept -> std::enable_if_t<(N >= 3), fx<N>>
{
    return fx_add_body(a, b);
}

// a * 2^s, for a * 2^s < 4 and 0 < s < 64.
template <int N>
constexpr auto fx_shl(fx<N> a, const unsigned s) noexcept -> fx<N>
{
    BOOST_DECIMAL_ASSERT(0U < s && s < 64U);
    for (int i {N - 1}; i > 0; --i)
    {
        a.w[i] = (a.w[i] << s) | (a.w[i - 1] >> (64U - s));
    }
    a.w[0] <<= s;
    return a;
}

// The low N words of the words a
template <int N, std::size_t M>
constexpr auto fx_low(const std::uint64_t (&a)[M]) noexcept -> fx<N>
{
    static_assert(static_cast<std::size_t>(N) <= M, "a has N words");
    fx<N> r {};
    for (int i {}; i < N; ++i)
    {
        r.w[i] = a[i];
    }
    return r;
}

// The fraction of the value in the words a with the scale of fx<N>, below 1
template <int N, std::size_t M>
constexpr auto fx_frac(const std::uint64_t (&a)[M]) noexcept -> fx<N>
{
    auto r {fx_low<N>(a)};
    r.w[N - 1] &= (UINT64_C(1) << 62U) - 1U;
    return r;
}

// a * n, truncated to N words, for a * n < 4
template <int N>
constexpr auto fx_mul_small(const fx<N>& a, const std::uint64_t n) noexcept -> fx<N>
{
    fx<N> r {};
    fx_mul_add(fx_make_span(r.w), fx_make_span(a.w), n);
    return r;
}

// a < b
template <int N>
constexpr auto fx_less(const fx<N>& a, const fx<N>& b) noexcept -> bool
{
    for (int i {N - 1}; i > 0; --i)
    {
        if (a.w[i] != b.w[i])
        {
            return a.w[i] < b.w[i];
        }
    }
    return a.w[0] < b.w[0];
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

// |c0| - z*(|c1| - z*(|c2| - ...)), for coefficients that alternate in sign.
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

// The alternating sum of the tail terms by Horner's rule in one word, for the top word z1 of z
template <std::size_t K>
constexpr auto fx_tail_sum(const std::uint64_t z1, const std::uint64_t (&tail)[K]) noexcept -> std::uint64_t
{
    std::uint64_t t {tail[K - 1]};
    for (std::size_t i {K - 1}; i-- > 0U;)
    {
        t = tail[i] - ((static_cast<boost::int128::uint128_t>(z1) * t).high << 2U);
    }
    return t;
}

// fx_alternating, with the terms from z^M on in one word scaled by 2^(62 + Shift): their weight is small
// enough that 64 bits of them keep the error of the sum below 2^-(64N - 2).
template <int Shift, int N, std::size_t M, std::size_t K>
constexpr auto fx_alternating_split(const fx<N>& z, const fx<N> (&head)[M], const std::uint64_t (&tail)[K]) noexcept -> fx<N>
{
    static_assert(0 <= Shift && Shift < 64 * (N - 1), "the tail must fit in the N words");

    const std::uint64_t t {fx_tail_sum(z.w[N - 1], tail)};

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
constexpr auto fx_mul_high(const boost::int128::uint128_t& a, const boost::int128::uint128_t& b) noexcept -> boost::int128::uint128_t
{
    std::uint64_t ll {};
    std::uint64_t lh {};
    std::uint64_t hl {};
    std::uint64_t hh {};
    boost::int128::detail::umul(a.low, b.low, ll);
    const auto mid {mul_add(a.low, b.high, ll, lh)};
    mul_add(a.high, b.low, mid, hl);
    const auto low {mul_add(a.high, b.high, lh, hh)};
    const auto sum {low + hl};
    return {hh + (sum < hl ? 1U : 0U), sum};
}

// For three words: the terms from z^M on in 128 bits scaled by 2^(126 + MidShift), and the terms
// after those in one word scaled by 2^(62 + TailShift).
template <int MidShift, int TailShift, std::size_t M, std::size_t K, std::size_t L>
constexpr auto fx_alternating_split(const fx<3>& z, const fx<3> (&head)[M], const boost::int128::uint128_t (&mid)[K],
                                    const std::uint64_t (&tail)[L]) noexcept -> fx<3>
{
    static_assert(0 < MidShift && MidShift < 64, "the middle terms must fit in the three words");
    static_assert(0 <= 64 + MidShift - TailShift && 64 + MidShift - TailShift < 64, "the tail must fit in 128 bits");

    const std::uint64_t t {fx_tail_sum(z.w[2], tail)};

    const boost::int128::uint128_t z2 {z.w[2], z.w[1]};
    auto u {static_cast<boost::int128::uint128_t>(t) << (64 + MidShift - TailShift)};
    for (std::size_t i {K}; i-- > 0U;)
    {
        u = mid[i] - (fx_mul_high(z2, u) << 2U);
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
constexpr auto fx_from_far(const boost::int128::uint128_t& sig, const int k) noexcept -> fx<N>
{
    constexpr int rows {static_cast<int>(sizeof(fx_table::pow10_neg) / sizeof(fx_table::pow10_neg[0]))};
    static_assert(N <= 3, "the table rows have N + 1 words");

    fx<N> r {};
    if (k < 38 || k >= 38 + 9 * rows)
    {
        return r;
    }
    const auto f {pow10(static_cast<std::uint64_t>(8 - (k - 38) % 9))};
    const auto lo {static_cast<boost::int128::uint128_t>(sig.low) * f};
    const auto hi {static_cast<boost::int128::uint128_t>(sig.high) * f + lo.high};
    const std::uint64_t g[3] {lo.low, hi.low, hi.high};
    const auto& q {fx_table::pow10_neg[(k - 38) / 9]};

    std::uint64_t p[static_cast<std::size_t>(N + 4)] {};
    BOOST_DECIMAL_FX_UNROLL
    for (int i {}; i < 3; ++i)
    {
        const auto carry {fx_mul_add(fx_make_span<N + 1>(p, i), fx_make_span<N + 1>(q.w, 3 - N), g[i])};
        p[i + N + 1] = carry;
    }
    for (int i {}; i < N; ++i)
    {
        r.w[i] = p[i + 3];
    }
    return r;
}

// sig * 10^-k as fixed point, for sig * 10^-k < 1 and k < 110, else 0 for k >= 110
template <int N>
constexpr auto fx_from_decimal(boost::int128::uint128_t sig, int k) noexcept -> fx<N>
{
    // One word, k <= 19: g = sig * 10^(19 - k) < 10^19 times ceil(2^190 / 10^19) / 2^128 is above sig * 2^62 / 10^k
    // by less than 2^-64, and a fraction of sig * 2^(62 - k) / 5^k is at least 5^-19: the floor is exact
    BOOST_DECIMAL_IF_CONSTEXPR (N == 1)
    {
        if (k <= 19)
        {
            const auto g {sig.low * pow10(static_cast<std::uint64_t>(19 - k))};
            const auto lo {static_cast<boost::int128::uint128_t>(g) * UINT64_C(0x9598F4F1E8361973)};
            fx<N> r {};
            r.w[0] = (static_cast<boost::int128::uint128_t>(g) * UINT64_C(0x760F253EDB4AB0D2) + lo.high).high;
            return r;
        }
    }
    if (k < 38)
    {
        sig *= pow10(static_cast<boost::int128::uint128_t>(38 - k));
        k = 38;
    }
    return fx_from_far<N>(sig, k);
}

// floor(sig * f / 2^(64N - 2)), for f <= 1.5 so that the result has at most 39 digits.
template <int N>
constexpr auto fx_scale(const boost::int128::uint128_t& sig, const fx<N>& f) noexcept -> boost::int128::uint128_t
{
    std::uint64_t p[static_cast<std::size_t>(N + 2)] {};
    const std::uint64_t g[2] {sig.low, sig.high};
    BOOST_DECIMAL_FX_UNROLL
    for (int i {}; i < 2; ++i)
    {
        const auto carry {fx_mul_add(fx_make_span<N>(p, i), fx_make_span(f.w), g[i])};
        p[i + N] = carry;
    }
    std::uint64_t r[2] {};
    fx_shr_words(fx_make_span(r), fx_make_span<2>(p, N - 1), p[N + 1], 62U);
    return boost::int128::uint128_t {r[1], r[0]};
}

// q * 10^e with precision + 1 digits in q and more digits below them that are not all zero, rounded in the
// current mode, for a result in the normal range
template <typename T, typename U>
constexpr auto fx_round_kept(const U q, int e, const bool neg) noexcept -> T
{
    // With a rest that is not zero, the nearest modes round up from a last digit of 5, and the directed modes by the
    // sign
    const auto d {impl::divmod10(static_cast<typename T::significand_type>(q))};
    auto s {d.quotient};
    // Not const, as current_rounding_mode says
    auto mode {current_rounding_mode()};
    if (mode == rounding_mode::fe_dec_upward ? !neg
        : mode == rounding_mode::fe_dec_downward ? neg : mode != rounding_mode::fe_dec_toward_zero && d.remainder >= 5U)
    {
        if (s == max_significand_v<T>)
        {
            s = max_significand_v<T> / 10U;
            ++e;
        }
        ++s;
    }
    return pack_in_range<T>(s, e + 1, neg);
}

// sig * 10^e rounded once, for a sig of 37 to 39 digits that truncates an inexact value: precision + 1 digits and
// a sticky rest. Near the subnormals the constructor rounds, and sig + 1 rounds the same where sig ends in 0 or 5.
template <typename T>
constexpr auto fx_round(boost::int128::uint128_t sig, int e, const bool neg) noexcept -> T
{
    constexpr int p {precision_v<T>};
    if (p >= 16 && sig >= pow10(static_cast<boost::int128::uint128_t>(36)) && e + 37 - p >= etiny_v<T>)
    {
        const int drop {(sig >= pow10(static_cast<boost::int128::uint128_t>(38)) ? 39 : sig >= pow10(static_cast<boost::int128::uint128_t>(37)) ? 38 : 37) - p - 1};
        return fx_round_kept<T>(impl::divmod_pow10_dispatch(sig, drop, pow10(static_cast<boost::int128::uint128_t>(drop))).quotient, e + drop, neg);
    }

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

// The same for a sig of 17 to 20 digits from one word, for decimal32_t: keep 8 digits
template <typename T>
constexpr auto fx_round(std::uint64_t sig, const int e, const bool neg) noexcept -> T
{
    static_assert(precision_v<T> == 7, "one word is for decimal32_t");
    if (e + 10 >= etiny_v<T>)
    {
        // Keep 8 of the 20, 19, 18 or 17 digits
        if (sig >= pow10(static_cast<std::uint64_t>(19)))
        {
            return fx_round_kept<T>(sig / pow10(static_cast<std::uint64_t>(12)), e + 12, neg);
        }
        if (sig >= pow10(static_cast<std::uint64_t>(18)))
        {
            return fx_round_kept<T>(sig / pow10(static_cast<std::uint64_t>(11)), e + 11, neg);
        }
        if (sig >= pow10(static_cast<std::uint64_t>(17)))
        {
            return fx_round_kept<T>(sig / pow10(static_cast<std::uint64_t>(10)), e + 10, neg);
        }
        if (sig >= pow10(static_cast<std::uint64_t>(16)))
        {
            return fx_round_kept<T>(sig / pow10(static_cast<std::uint64_t>(9)), e + 9, neg);
        }
    }

    if (sig % 5U == 0U)
    {
        sig += 1U;
    }
    return T {sig, e, neg};
}

// Whether sig of 37 to 39 digits, with an error below w units, can round to another value than the exact one for
// decimal128_t: the boundaries are the multiples of 500, 5000 or 50000.
constexpr auto fx_near_boundary(const boost::int128::uint128_t& sig, const std::uint64_t w = 8U) noexcept -> bool
{
    // 2^64 = 1616 (mod 50000), and 500 and 5000 divide 50000.
    // Constant divisors, as a division by m takes a divide instruction
    const std::uint64_t r50 {((sig.high % 50000U) * 1616U + sig.low % 50000U) % 50000U};
    const bool d37 {sig < pow10(static_cast<boost::int128::uint128_t>(37))};
    const bool d38 {sig < pow10(static_cast<boost::int128::uint128_t>(38))};
    const std::uint64_t m {d37 ? UINT64_C(500) : d38 ? UINT64_C(5000) : UINT64_C(50000)};
    const std::uint64_t r {d37 ? r50 % 500U : d38 ? r50 % 5000U : r50};
    return r < w || r > m - w;
}

// For decimal64_t, sig * 10^e with an error below w units, rounded once in out where sig has 19 digits, no
// multiple of 500 (a rounding boundary) is next to it, and the result is not subnormal
template <typename T>
constexpr auto fx_try_round(const std::uint64_t sig, const int e, const bool neg, const std::uint64_t w, T& out) noexcept -> bool
{
    const std::uint64_t r {sig % 500U};
    if (r < w || r > 500U - w || e + 3 < etiny_v<T>)
    {
        return false;
    }
    out = fx_round_kept<T>(sig / 100U, e + 2, neg);
    return true;
}

// N + 1 words in two's complement with the scale of fx<N>: the extra word and the two top bits of word N - 1 hold the
// integer part. fx<N + 1> stays below 4, but logs, exp arguments and Stirling sums need an integer part up to 2^65.
template <int N>
struct fx_wide
{
    std::uint64_t w[static_cast<std::size_t>(N + 1)];
};

template <int N>
constexpr auto fx_widen(const fx<N>& a) noexcept -> fx_wide<N>
{
    fx_wide<N> r {};
    for (int i {}; i < N; ++i)
    {
        r.w[i] = a.w[i];
    }
    return r;
}

template <int N>
constexpr auto fx_wide_add(const fx_wide<N>& a, const fx_wide<N>& b) noexcept -> fx_wide<N>
{
    fx_wide<N> r {a};
    fx_add_to(fx_make_span(r.w), fx_make_span(b.w));
    return r;
}

template <int N>
constexpr auto fx_wide_negate(const fx_wide<N>& a) noexcept -> fx_wide<N>
{
    fx_wide<N> r {};
    std::uint64_t carry {1U};
    for (int i {}; i <= N; ++i)
    {
        r.w[i] = ~a.w[i] + carry;
        carry = static_cast<std::uint64_t>(carry == 1U && r.w[i] == 0U);
    }
    return r;
}

// a + b, or a - b for neg
template <int N>
constexpr auto fx_wide_add(const fx_wide<N>& a, const fx_wide<N>& b, const bool neg) noexcept -> fx_wide<N>
{
    return fx_wide_add(a, neg ? fx_wide_negate(b) : b);
}

// a * c, truncated, for a >= 0 and c < 4 in N words
template <int N>
constexpr auto fx_wide_mul(const fx_wide<N>& a, const fx<N>& c) noexcept -> fx_wide<N>
{
    std::uint64_t p[static_cast<std::size_t>(2 * N + 2)] {};
    for (int i {}; i < N; ++i)
    {
        const auto carry {fx_mul_add(fx_make_span<N + 1>(p, i), fx_make_span(a.w), c.w[i])};
        p[i + N + 1] = carry;
    }
    fx_wide<N> r {};
    fx_shr_words(fx_make_span(r.w), fx_make_span<N + 1>(p, N - 1), p[2 * N], 62U);
    return r;
}

template <int N>
constexpr auto fx_wide_int(const std::uint64_t n) noexcept -> fx_wide<N>
{
    fx_wide<N> r {};
    r.w[N - 1] = n << 62U;
    r.w[N] = n >> 2U;
    return r;
}

// |r| as digits * 10^exp10, and neg flipped by the sign of r. |r| > 0.001 has 37 or 38 digits (18 or 19 for one word)
// from the integer part and the fraction times 10 while it is below 0.1
template <int N>
constexpr auto fx_wide_digits(fx_wide<N> r, bool& neg, int& exp10) noexcept -> fx_digits_t<N>
{
    constexpr int scale10 {fx_scale10(N)};

    if ((r.w[N] >> 63U) != 0U)
    {
        r = fx_wide_negate(r);
        neg = !neg;
    }
    const std::uint64_t whole {(r.w[N] << 2U) | (r.w[N - 1] >> 62U)};
    auto f {fx_frac<N>(r.w)};
    BOOST_DECIMAL_ASSERT(whole != 0U || f.w[N - 1] != 0U);

    int shift {};
    while (whole == 0U && f.w[N - 1] < UINT64_C(0x0666666666666666)) // 0.1 * 2^62, rounded down
    {
        f = fx_mul_small(f, 10U);
        ++shift;
    }
    const int s10 {scale10 + 1 - (whole == 0U ? 0 : num_digits(whole))};

    exp10 = -s10 - shift;
    const auto scale {pow10(static_cast<boost::int128::uint128_t>(s10))};
    return static_cast<fx_digits_t<N>>(static_cast<boost::int128::uint128_t>(whole) * scale + fx_scale(scale, f));
}

// r rounded once in the current mode, with the sign flipped by neg
template <typename T, int N>
constexpr auto fx_wide_round(const fx_wide<N>& r, bool neg) noexcept -> T
{
    int exp10 {};
    const auto digits {fx_wide_digits(r, neg, exp10)};
    return fx_round<T>(digits, exp10, neg);
}

// For decimal128_t, r in two words with an error below 2^bits units of 2^-126, which is 10^-exp10 * 2^(bits - 126)
// units of the digits, rounded once in out where it cannot be next to a rounding boundary
template <typename T>
constexpr auto fx_wide_try_round(const fx_wide<2>& r, bool neg, T& out, const unsigned bits = 6U) noexcept -> bool
{
    int exp10 {};
    const auto digits {fx_wide_digits(r, neg, exp10)};
    if (exp10 < -38 || fx_near_boundary(digits, static_cast<std::uint64_t>(pow10(static_cast<boost::int128::uint128_t>(-exp10)) >> (126U - bits)) + 2U))
    {
        return false;
    }
    out = fx_round<T>(digits, exp10, neg);
    return true;
}

// The same for decimal64_t in one word and units of 2^-62, for |r| >= 0.1: the digits are 19 with exp10 >= -19
template <typename T>
constexpr auto fx_wide_try_round(const fx_wide<1>& r, bool neg, T& out, const unsigned bits = 6U) noexcept -> bool
{
    int exp10 {};
    const auto digits {fx_wide_digits(r, neg, exp10)};
    return fx_try_round(digits, exp10, neg, (pow10(static_cast<std::uint64_t>(-exp10)) >> (62U - bits)) + 2U, out);
}

#ifdef _MSC_VER
#  pragma warning(pop)
#endif

} // namespace detail
} // namespace decimal
} // namespace boost

#endif // BOOST_DECIMAL_DETAIL_CMATH_IMPL_FIXED_POINT_HPP
