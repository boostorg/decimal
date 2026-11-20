// Copyright 2026 Shen-Ta Hsieh
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//
// Checks the error bounds of the first tries of exp, exp2, sinh, cosh, tanh, erf and erfc against the same code in
// more words.
// A bound that is too small gives a wrong result only next to a rounding boundary, so the result alone rarely shows it.

#include <boost/decimal.hpp>
#include <boost/core/lightweight_test.hpp>
#include <cstdint>

using namespace boost::decimal;
using namespace boost::decimal::detail;
using boost::int128::uint128_t;

namespace {

constexpr int trials {20000};

std::uint64_t g_state {UINT64_C(0x9E3779B97F4A7C15)};

auto next_word() -> std::uint64_t
{
    g_state ^= g_state << 13U;
    g_state ^= g_state >> 7U;
    g_state ^= g_state << 17U;
    return g_state;
}

// A significand of p digits
auto next_sig(const int p) -> uint128_t
{
    uint128_t sig {next_word() % 9U + 1U};
    for (int i {1}; i < p; ++i)
    {
        sig = sig * 10U + next_word() % 10U;
    }
    return sig;
}

auto distance(const uint128_t& a, const uint128_t& b) -> uint128_t
{
    return a > b ? a - b : b - a;
}

// The first try in the given words has a bound of d * 2^-bits + base units of d, and three words add 8 units of theirs
template <int words, bool Precise, bool Base2>
void test_exp(const int p, const unsigned bits, const unsigned base)
{
    const auto& c {Base2 ? fx_table::log10_2 : fx_table::log10_e};
    constexpr int reference {3};
    const auto scale {pow10(static_cast<uint128_t>(fx_scale10(reference) - fx_scale10(words)))};

    for (int k_shift {-2}; k_shift < 22; ++k_shift)
    {
        for (int i {0}; i < trials / 24; ++i)
        {
            const auto sig {next_sig(p)};
            const int k {p + k_shift};
            const auto whole {static_cast<std::uint64_t>(exp_detail::exp_whole(sig, k))};
            if (whole >= 880U)
            {
                continue;
            }
            const bool neg {(next_word() & 1U) != 0U};
            const auto y1 {exp_detail::exp_log10_signed<words>(sig, k, whole, c, neg)};
            const auto y3 {exp_detail::exp_log10_signed<reference>(sig, k, whole, c, neg)};
            if (y1.exp10 != y3.exp10)
            {
                continue;
            }
            const auto d1 {exp_detail::exp10_digits(exp_detail::exp10_frac<words, Precise>(y1.f), fx_scale10(words))};
            const auto d3 {exp_detail::exp10_digits(exp_detail::exp10_frac(y3.f), fx_scale10(reference))};
            BOOST_TEST(distance(d1 * scale, d3) <= ((d1 >> bits) + base) * scale + 8U);
        }
    }
}

// The first try in one word has a bound of 2 units of its 19 digits, or 2^25 units with Narrow for 7 digits
template <bool Base2, bool Narrow>
void test_exp_word()
{
    constexpr int p {Narrow ? 7 : 16};
    const uint128_t w {Narrow ? UINT64_C(1) << 25U : 2U};
    const auto& c {Base2 ? fx_table::log10_2 : fx_table::log10_e};
    constexpr int reference {3};
    const auto scale {pow10(static_cast<uint128_t>(fx_scale10(reference) - fx_scale10(1)))};

    for (int k {12}; k <= 32; ++k)
    {
        for (int i {0}; i < trials / 21; ++i)
        {
            const auto sig {next_sig(p) * pow10(static_cast<uint128_t>(16 - p))};
            const auto whole {static_cast<std::uint64_t>(exp_detail::exp_whole(sig, k))};
            if (whole >= 880U)
            {
                continue;
            }
            const bool neg {(next_word() & 1U) != 0U};
            int e1 {};
            const uint128_t d1 {exp_detail::exp_word_digits<Base2, Narrow>(sig.low, k, neg, e1)};
            const auto y3 {exp_detail::exp_log10_signed<reference>(sig, k, whole, c, neg)};
            if (e1 != y3.exp10)
            {
                continue;
            }
            const auto d3 {exp_detail::exp10_digits(exp_detail::exp10_frac(y3.f), fx_scale10(reference))};
            BOOST_TEST(distance(d1 * scale, d3) <= w * scale + 8U);
        }
    }
}

// The first try of sinh and cosh in one word has a bound of 3 units of its 19 digits, or 2^25 units with Narrow and
// sinh from 0.1. With near, exp(|x|) is next above 2 * 10^n, where sinh(|x|) is next below 10^n: the largest error.
template <bool Narrow>
void test_sinh_cosh_word(const bool is_cosh, const bool near)
{
    constexpr int p {Narrow ? 7 : 16};
    const uint128_t w {Narrow ? UINT64_C(1) << 25U : 3U};
    for (int k {12}; k <= 17; ++k)
    {
        for (int i {0}; i < trials / 6; ++i)
        {
            auto sig {next_sig(p) * pow10(static_cast<uint128_t>(16 - p))};
            if (near)
            {
                // ln(20) and ln(200) with 16 digits, and up to 10^-4 more
                sig = ((i & 1) != 0 ? UINT64_C(2995732273553991) : UINT64_C(5298317366548036)) + next_word() % UINT64_C(100000000000);
            }
            const auto whole {static_cast<std::uint64_t>(exp_detail::exp_whole(sig, k))};
            if (whole >= 880U || (near ? k != 15 : is_cosh ? k == 17 && sig < UINT64_C(6250000000000000) : k > (Narrow ? 16 : 15)))
            {
                continue;
            }
            int e1 {};
            const uint128_t d1 {exp_detail::sinh_cosh_word_digits<Narrow>(sig.low, k, is_cosh, e1)};
            const auto r {exp_detail::sinh_cosh_parts<3>(whole, exp_detail::exp_frac<3>(sig, k, whole), is_cosh)};
            const auto d3 {fx_scale(pow10(static_cast<uint128_t>(37)) * 4U, r.f)};
            const auto scale {pow10(static_cast<uint128_t>(e1 - (r.exp10 - 37)))};
            BOOST_TEST(distance(d1 * scale, d3) <= w * scale + 8U);
        }
    }
}

// The first try of tanh in one word has a bound of 3 units of its 19 digits, or 2^25 units with Narrow
template <bool Narrow>
void test_tanh_word()
{
    constexpr int p {Narrow ? 7 : 16};
    const uint128_t w {Narrow ? UINT64_C(1) << 25U : 3U};
    const auto scale {pow10(static_cast<uint128_t>(19))};

    for (int k {12}; k <= 16; ++k)
    {
        for (int i {0}; i < trials / 5; ++i)
        {
            const auto sig {next_sig(p) * pow10(static_cast<uint128_t>(16 - p))};
            if (k == 16 && sig < UINT64_C(1100000000000000))
            {
                continue;
            }
            const uint128_t d1 {exp_detail::tanh_word_digits<Narrow>(sig.low, k)};
            const auto whole {static_cast<std::uint64_t>(exp_detail::exp_whole(sig, k))};
            fx<3> q {};
            if (d1 == 0U || !exp_detail::tanh_quotient<3>(whole, exp_detail::exp_frac<3>(sig, k, whole), q))
            {
                continue;
            }
            BOOST_TEST(distance(d1 * scale, fx_scale(scale * scale, q)) <= w * scale + 8U);
        }
    }
}

} // namespace

int main()
{
    test_exp_word<false, true>();
    test_exp_word<true, true>();
    test_exp_word<false, false>();
    test_exp_word<true, false>();
    for (const bool is_cosh : {false, true})
    {
        test_sinh_cosh_word<true>(is_cosh, false);
        test_sinh_cosh_word<false>(is_cosh, false);
        test_sinh_cosh_word<false>(is_cosh, true);
    }
    test_tanh_word<true>();
    test_tanh_word<false>();
    test_exp<2, true, false>(34, 119U, 2U);
    test_exp<2, true, true>(34, 119U, 2U);

    return boost::report_errors();
}
