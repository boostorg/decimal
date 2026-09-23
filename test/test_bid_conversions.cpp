// Copyright 2024 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include <boost/decimal.hpp>
#include <boost/core/lightweight_test.hpp>
#include <iostream>
#include <limits>
#include <random>
#include <cstdint>
#include <type_traits>

using namespace boost::decimal;

template <typename T>
void test()
{
    std::mt19937_64 rng(42);
    std::uniform_int_distribution<std::int64_t> dist(std::numeric_limits<std::int64_t>::min(),
                                                     std::numeric_limits<std::int64_t>::max());

    for (std::size_t i {}; i < 1024; ++i)
    {
        const T val {dist(rng)};
        const auto bits {to_bid<T>(val)};
        const T return_val {from_bid<T>(bits)};
        BOOST_TEST_EQ(val, return_val);
    }
}

static auto make_u128(const std::uint64_t high, const std::uint64_t low) noexcept -> boost::int128::uint128_t
{
    boost::int128::uint128_t value {};
    value.high = high;
    value.low = low;
    return value;
}

// The largest coefficient the format can hold, which is the last canonical encoding
// before the flush to zero starts
template <typename T, typename BitType>
void check_canonical(const char* context, const BitType bits, const T expected)
{
    const auto decoded {from_bid<T>(bits)};
    if (!BOOST_TEST_EQ(decoded, expected))
    {
        std::cerr << "Canonical encoding decoded incorrectly: " << context << std::endl;
    }
}

// Only the compliant types carry a cohort, so only they can report a quantum exponent
template <typename T>
auto check_quantum(const char*, const T, const int) noexcept
    -> std::enable_if_t<detail::is_fast_type_v<T>, void>
{
}

template <typename T>
auto check_quantum(const char* context, const T decoded, const int expected_exp)
    -> std::enable_if_t<!detail::is_fast_type_v<T>, void>
{
    if (!BOOST_TEST_EQ(quantexp(decoded), expected_exp))
    {
        std::cerr << "Non canonical encoding lost its quantum exponent: " << context << std::endl;
    }
}

// IEEE 754-2008 3.5.2: a coefficient larger than the format can hold makes the encoding
// non canonical, and it represents zero with the sign and quantum exponent it does encode
template <typename T, typename BitType>
void check_non_canonical(const char* context, const BitType bits, const int expected_exp, const bool expected_sign)
{
    const auto decoded {from_bid<T>(bits)};

    const bool ok {decoded == T{0, 0} && signbit(decoded) == expected_sign && isfinite(decoded)};
    if (!BOOST_TEST(ok))
    {
        std::cerr << "Non canonical encoding did not flush to zero: " << context
                  << " -> " << decoded << " signbit " << signbit(decoded) << std::endl;
    }

    check_quantum<T>(context, decoded, expected_exp);
}

template <typename T>
void test_non_canonical_d32(const char* name)
{
    // Steering 11, biased exponent 101, coefficient 2^23 + 1611391 = 9'999'999
    check_canonical<T>(name, UINT32_C(0x6CB8967F), T{9999999, 0});

    // One past the maximum, then the largest encodable coefficient
    check_non_canonical<T>(name, UINT32_C(0x6CB89680), 0, false);
    check_non_canonical<T>(name, UINT32_C(0x6FFFFFFF), 26, false);
    check_non_canonical<T>(name, UINT32_C(0xECB89680), 0, true);

    // Across the whole biased exponent range, with the coefficient pinned at its maximum
    for (std::uint32_t biased {}; biased <= UINT32_C(191); ++biased)
    {
        const std::uint32_t bits {UINT32_C(0x60000000) | (biased << 21U) | UINT32_C(0x1FFFFF)};
        const auto exp {static_cast<int>(biased) - 101};
        check_non_canonical<T>(name, bits, exp, false);
        check_non_canonical<T>(name, bits | UINT32_C(0x80000000), exp, true);
    }

    // The wider steering form holds 23 bits, and 2^23 - 1 is below the maximum, so every
    // one of its encodings stays canonical
    check_canonical<T>(name, UINT32_C(0x32FFFFFF), T{8388607, 0});
}

template <typename T>
void test_non_canonical_d64(const char* name)
{
    // Steering 11, biased exponent 398, coefficient 9'999'999'999'999'999
    check_canonical<T>(name, UINT64_C(0x6C7386F26FC0FFFF), T{UINT64_C(9999999999999999), 0});

    check_non_canonical<T>(name, UINT64_C(0x6C7386F26FC10000), 0, false);
    check_non_canonical<T>(name, UINT64_C(0x6FFFFFFFFFFFFFFF), 113, false);
    check_non_canonical<T>(name, UINT64_C(0xEC7386F26FC10000), 0, true);

    for (std::uint64_t biased {}; biased <= UINT64_C(767); ++biased)
    {
        const std::uint64_t bits {UINT64_C(0x6000000000000000) | (biased << 51U) | UINT64_C(0x7FFFFFFFFFFFF)};
        const auto exp {static_cast<int>(biased) - 398};
        check_non_canonical<T>(name, bits, exp, false);
        check_non_canonical<T>(name, bits | UINT64_C(0x8000000000000000), exp, true);
    }

    // 2^53 - 1 in the wider steering form is still canonical
    check_canonical<T>(name, UINT64_C(0x31DFFFFFFFFFFFFF), T{UINT64_C(9007199254740991), 0});
}

template <typename T>
void test_non_canonical_d128(const char* name)
{
    // The wider steering form holds 113 bits here, and 2^113 - 1 is above 10^34 - 1, so
    // unlike the narrower formats decimal128 can reach a non canonical coefficient from
    // either form. The 11 form starts at 2^113 and so is never canonical.
    check_canonical<T>(name, make_u128(UINT64_C(0x3041ED09BEAD87C0), UINT64_C(0x378D8E63FFFFFFFF)),
                       T{make_u128(UINT64_C(0x1ED09BEAD87C0), UINT64_C(0x378D8E63FFFFFFFF)), 0});

    check_non_canonical<T>(name, make_u128(UINT64_C(0x3041ED09BEAD87C0), UINT64_C(0x378D8E6400000000)), 0, false);
    check_non_canonical<T>(name, make_u128(UINT64_C(0x3041FFFFFFFFFFFF), UINT64_MAX), 0, false);
    check_non_canonical<T>(name, make_u128(UINT64_C(0xB041ED09BEAD87C0), UINT64_C(0x378D8E6400000000)), 0, true);

    // Every 11 form encoding is non canonical, whatever its exponent
    for (std::uint64_t biased {}; biased <= UINT64_C(12287); biased += UINT64_C(97))
    {
        const auto bits {make_u128(UINT64_C(0x6000000000000000) | (biased << 47U), UINT64_C(0))};
        const auto exp {static_cast<int>(biased) - 6176};
        check_non_canonical<T>(name, bits, exp, false);

        auto neg_bits {bits};
        neg_bits.high |= UINT64_C(0x8000000000000000);
        check_non_canonical<T>(name, neg_bits, exp, true);
    }

    // The wider form at the same exponents, with an out of range coefficient
    for (std::uint64_t biased {}; biased <= UINT64_C(12287); biased += UINT64_C(97))
    {
        const auto bits {make_u128((biased << 49U) | UINT64_C(0x1FFFFFFFFFFFF), UINT64_MAX)};
        check_non_canonical<T>(name, bits, static_cast<int>(biased) - 6176, false);
    }
}

// A non-finite encoding is not a coefficient, so it has to pass through untouched
template <typename T>
void test_non_finite_pass_through()
{
    #ifndef BOOST_DECIMAL_FAST_MATH
    BOOST_TEST(isinf(from_bid<T>(UINT32_C(0x78000000))));
    BOOST_TEST(isinf(from_bid<T>(UINT32_C(0xF8000000))));
    BOOST_TEST(signbit(from_bid<T>(UINT32_C(0xF8000000))));
    BOOST_TEST(isnan(from_bid<T>(UINT32_C(0x7C000000))));
    BOOST_TEST(isnan(from_bid<T>(UINT32_C(0x7E000000))));
    BOOST_TEST(issignaling(from_bid<T>(UINT32_C(0x7E000000))));
    #endif
}

// from_bid selects the representation by overload, so only a decimal type may match one
template <typename...>
using void_t = void;

template <typename T, typename BitType, typename = void>
struct has_from_bid : std::false_type {};

template <typename T, typename BitType>
struct has_from_bid<T, BitType, void_t<decltype(from_bid<T>(BitType {}))>> : std::true_type {};

struct not_a_decimal {};

static_assert(has_from_bid<decimal32_t, std::uint32_t>::value, "decimal32_t has no from_bid");
static_assert(has_from_bid<decimal_fast32_t, std::uint32_t>::value, "decimal_fast32_t has no from_bid");
static_assert(has_from_bid<decimal64_t, std::uint64_t>::value, "decimal64_t has no from_bid");
static_assert(has_from_bid<decimal_fast64_t, std::uint64_t>::value, "decimal_fast64_t has no from_bid");
static_assert(has_from_bid<decimal128_t, boost::int128::uint128_t>::value, "decimal128_t has no from_bid");
static_assert(has_from_bid<decimal_fast128_t, boost::int128::uint128_t>::value, "decimal_fast128_t has no from_bid");

static_assert(!has_from_bid<int, std::uint32_t>::value, "from_bid takes an integer");
static_assert(!has_from_bid<double, std::uint32_t>::value, "from_bid takes a binary float");
static_assert(!has_from_bid<void*, std::uint64_t>::value, "from_bid takes a pointer");
static_assert(!has_from_bid<not_a_decimal, boost::int128::uint128_t>::value, "from_bid takes an unrelated class");

int main()
{
    test<decimal_fast32_t>();
    test<decimal_fast64_t>();

    test<decimal32_t>();
    test<decimal64_t>();

    test<decimal128_t>();
    test<decimal_fast128_t>();

    // See: https://github.com/boostorg/decimal/issues/1424
    test_non_canonical_d32<decimal32_t>("decimal32_t");
    test_non_canonical_d32<decimal_fast32_t>("decimal_fast32_t");
    test_non_canonical_d64<decimal64_t>("decimal64_t");
    test_non_canonical_d64<decimal_fast64_t>("decimal_fast64_t");
    test_non_canonical_d128<decimal128_t>("decimal128_t");
    test_non_canonical_d128<decimal_fast128_t>("decimal_fast128_t");

    test_non_finite_pass_through<decimal32_t>();
    test_non_finite_pass_through<decimal_fast32_t>();

    return boost::report_errors();
}
