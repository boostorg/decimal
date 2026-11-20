// Copyright 2026 Shen-Ta Hsieh
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//
// Checks the shared fixed-point operations against schoolbook arithmetic on 32-bit digits

#include <boost/decimal.hpp>
#include <boost/core/lightweight_test.hpp>
#include <cstdint>

using namespace boost::decimal;
using boost::int128::uint128_t;
using detail::fx;

namespace {

// A little-endian integer, long enough for the full product of five words
struct big
{
    std::uint32_t d[22];
};

std::uint64_t g_state {UINT64_C(0x9E3779B97F4A7C15)};

auto next_word() -> std::uint64_t
{
    g_state ^= g_state << 13U;
    g_state ^= g_state >> 7U;
    g_state ^= g_state << 17U;
    return g_state;
}

// A word of random bits, or one with long runs of ones and zeros for carries and borrows
auto next_pattern() -> std::uint64_t
{
    const auto kind {next_word() % 4U};
    return kind == 0U ? UINT64_MAX : kind == 1U ? 0U : kind == 2U ? UINT64_MAX << (next_word() % 64U) : next_word();
}

template <int N>
auto to_big(const fx<N>& a) -> big
{
    big r {};
    for (int i {}; i < N; ++i)
    {
        r.d[2 * i] = static_cast<std::uint32_t>(a.w[i]);
        r.d[2 * i + 1] = static_cast<std::uint32_t>(a.w[i] >> 32U);
    }
    return r;
}

auto big_product(const big& a, const big& b) -> big
{
    big r {};
    for (int i {}; i < 11; ++i)
    {
        std::uint64_t c {};
        for (int j {}; j < 11; ++j)
        {
            c += static_cast<std::uint64_t>(a.d[i]) * b.d[j] + r.d[i + j];
            r.d[i + j] = static_cast<std::uint32_t>(c);
            c >>= 32U;
        }
    }
    return r;
}

auto big_shr(const big& a, const int bits) -> big
{
    big r {};
    const int words {bits / 32};
    const auto s {static_cast<unsigned>(bits % 32)};
    for (int i {}; i + words < 22; ++i)
    {
        const std::uint64_t low {a.d[i + words]};
        const std::uint64_t high {i + words + 1 < 22 ? a.d[i + words + 1] : 0U};
        r.d[i] = static_cast<std::uint32_t>(((high << 32U) | low) >> s);
    }
    return r;
}

auto big_bit(const int bit) -> big
{
    big r {};
    r.d[bit / 32] = UINT32_C(1) << static_cast<unsigned>(bit % 32);
    return r;
}

auto big_less(const big& a, const big& b) -> bool
{
    for (int i {21}; i >= 0; --i)
    {
        if (a.d[i] != b.d[i])
        {
            return a.d[i] < b.d[i];
        }
    }
    return false;
}

// a - b for a >= b
auto big_minus(const big& a, const big& b) -> big
{
    big r {};
    std::uint64_t borrow {};
    for (int i {}; i < 22; ++i)
    {
        const std::uint64_t t {static_cast<std::uint64_t>(a.d[i]) - b.d[i] - borrow};
        r.d[i] = static_cast<std::uint32_t>(t);
        borrow = (t >> 32U) & 1U;
    }
    return r;
}

// A value below 2, in which the two integer bits of the top word leave a product below 4
template <int N>
auto next_fx() -> fx<N>
{
    fx<N> r {};
    for (int i {}; i < N; ++i)
    {
        r.w[i] = next_pattern();
    }
    r.w[N - 1] >>= 2U;
    return r;
}

// fx_mul leaves out less than 4(N - 1) units and then truncates, so its floor is at most 4(N - 1) units below that of
// the full product, and in one word equal
template <int N>
void test_mul()
{
    for (int i {}; i < 4000; ++i)
    {
        const auto a {next_fx<N>()};
        const auto b {next_fx<N>()};
        const auto r {detail::fx_mul(a, b)};
        const auto floor_product {big_shr(big_product(to_big(a), to_big(b)), 64 * N - 2)};
        BOOST_TEST(!big_less(floor_product, to_big(r)));
        const auto diff {big_minus(floor_product, to_big(r))};
        big limit {};
        limit.d[0] = 4U * static_cast<std::uint32_t>(N - 1) + 1U;
        BOOST_TEST(big_less(diff, limit));
    }
}

// Words past the top word of a sum or a difference are dropped, so these compare modulo 2^(64N)
template <int N>
void test_add_sub()
{
    for (int i {}; i < 4000; ++i)
    {
        const auto a {next_fx<N>()};
        const auto b {next_fx<N>()};
        const auto sum {detail::fx_add(a, b)};
        std::uint64_t carry {};
        for (int k {}; k < N; ++k)
        {
            const std::uint64_t low {(a.w[k] & UINT32_MAX) + (b.w[k] & UINT32_MAX) + carry};
            const std::uint64_t high {(a.w[k] >> 32U) + (b.w[k] >> 32U) + (low >> 32U)};
            BOOST_TEST_EQ(sum.w[k], (high << 32U) | (low & UINT32_MAX));
            carry = high >> 32U;
        }
        const auto back {detail::fx_sub(sum, b)};
        for (int k {}; k < N; ++k)
        {
            BOOST_TEST_EQ(back.w[k], a.w[k]);
        }
    }
}

// |d * fx_recip(d) - 1| is below 2^(1 - 60N) for 1/2 <= d < 2
template <int N>
void test_recip()
{
    for (int i {}; i < 2000; ++i)
    {
        auto d {next_fx<N>()};
        d.w[N - 1] |= UINT64_C(1) << 61U;
        const auto r {detail::fx_recip(d)};
        const auto product {big_product(to_big(d), to_big(r))};
        const int one_bit {2 * (64 * N - 2)};
        const auto one {big_bit(one_bit)};
        const auto err {big_less(product, one) ? big_minus(one, product) : big_minus(product, one)};
        BOOST_TEST(big_less(err, big_bit(one_bit + 1 - 60 * N)));
    }
}

// Each boundary of fx_near_boundary, at each digit count of sig
void test_near_boundary()
{
    const uint128_t p36 {detail::pow10(static_cast<uint128_t>(36))};
    const std::uint64_t ms[3] {UINT64_C(500), UINT64_C(5000), UINT64_C(50000)};
    for (int len {}; len < 3; ++len)
    {
        const auto m {ms[len]};
        const uint128_t base {p36 * (len == 0 ? UINT64_C(1) : len == 1 ? UINT64_C(10) : UINT64_C(100))};
        const uint128_t k {base / m + 12345U};
        const std::uint64_t w {8U};
        const std::uint64_t offsets[8] {0U, 1U, w - 1U, w, m - w - 1U, m - w, m - w + 1U, m - 1U};
        for (const auto offset : offsets)
        {
            const bool expected {offset < w || offset > m - w};
            BOOST_TEST_EQ(detail::fx_near_boundary(k * m + offset, w), expected);
        }
    }
}

// fx_round takes a sig that truncates an inexact value, so the rest of a digit string is never exactly zero or a half
template <typename T>
void test_round(const rounding_mode mode, const uint128_t& q, const int digits, const unsigned rest_case, const bool neg)
{
    const int k {digits - detail::precision_v<T>};
    const uint128_t scale {detail::pow10(static_cast<uint128_t>(k))};
    const uint128_t half {scale / 2U};
    const uint128_t rest {rest_case == 0U ? uint128_t {0U} : rest_case == 1U ? half - 1U : rest_case == 2U ? half : scale - 1U};
    const uint128_t sig {q * scale + rest};
    const bool up {mode == rounding_mode::fe_dec_upward ? !neg
                   : mode == rounding_mode::fe_dec_downward ? neg
                   : mode == rounding_mode::fe_dec_toward_zero ? false
                   : rest >= half};
    BOOST_TEST_EQ(detail::fx_round<T>(sig, -5, neg), (T {q + (up ? 1U : 0U), k - 5, neg}));
}

template <typename T>
void test_round_all()
{
    const int p {detail::precision_v<T>};
    const uint128_t top {detail::pow10(static_cast<uint128_t>(p)) - 1U};
    const uint128_t mid {detail::pow10(static_cast<uint128_t>(p - 1)) + 12345U};
    #ifndef BOOST_DECIMAL_NO_CONSTEVAL_DETECTION
    const rounding_mode modes[5] {rounding_mode::fe_dec_to_nearest, rounding_mode::fe_dec_upward, rounding_mode::fe_dec_downward,
                                  rounding_mode::fe_dec_to_nearest_from_zero, rounding_mode::fe_dec_toward_zero};
    #else
    const rounding_mode modes[1] {rounding_mode::fe_dec_to_nearest};
    #endif
    for (const auto mode : modes)
    {
        fesetround(mode);
        for (int digits {37}; digits <= 38; ++digits)
        {
            for (unsigned rest_case {}; rest_case < 4U; ++rest_case)
            {
                for (const bool neg : {false, true})
                {
                    test_round<T>(mode, mid, digits, rest_case, neg);
                    test_round<T>(mode, top, digits, rest_case, neg);
                }
            }
        }
    }
    fesetround(rounding_mode::fe_dec_to_nearest);
}

template <int N>
void test_words()
{
    test_mul<N>();
    test_add_sub<N>();
    test_recip<N>();
}

} // namespace

int main()
{
    test_words<1>();
    test_words<2>();
    test_words<3>();
    test_words<4>();
    test_words<5>();
    test_near_boundary();
    test_round_all<decimal32_t>();
    test_round_all<decimal64_t>();
    test_round_all<decimal128_t>();
    test_round_all<decimal_fast32_t>();
    test_round_all<decimal_fast64_t>();
    test_round_all<decimal_fast128_t>();
    return boost::report_errors();
}
