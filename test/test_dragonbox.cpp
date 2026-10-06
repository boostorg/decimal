// Copyright 2026 Pavel P
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include <boost/decimal.hpp>
#include <boost/core/lightweight_test.hpp>
#include <cinttypes>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <random>

using boost::decimal::detail::dragonbox::floating_point_to_fd32;
using boost::decimal::detail::dragonbox::floating_point_to_fd64;

static auto float_from_bits(const std::uint32_t bits) -> float
{
    float val {};
    std::memcpy(&val, &bits, sizeof(val));
    return val;
}

static auto double_from_bits(const std::uint64_t bits) -> double
{
    double val {};
    std::memcpy(&val, &bits, sizeof(val));
    return val;
}

static auto shortest(const float val) -> decltype(floating_point_to_fd32(val))
{
    return floating_point_to_fd32(val);
}

static auto shortest(const double val) -> decltype(floating_point_to_fd64(val))
{
    return floating_point_to_fd64(val);
}

static auto parse(const char* str, float) -> float
{
    return std::strtof(str, nullptr);
}

static auto parse(const char* str, double) -> double
{
    return std::strtod(str, nullptr);
}

template <typename T>
static auto reads_back(const std::uint64_t mantissa, const int exponent, const bool sign, const T val) -> bool
{
    char buffer[64];
    std::snprintf(buffer, sizeof(buffer), "%s%" PRIu64 "e%d", sign ? "-" : "", mantissa, exponent);
    const auto parsed {parse(buffer, val)};
    return std::memcmp(&parsed, &val, sizeof(val)) == 0;
}

template <typename T>
static void check_value(const T val, const std::uint64_t mantissa, const std::int32_t exponent)
{
    const auto fd {shortest(val)};
    BOOST_TEST_EQ(fd.mantissa, mantissa);
    BOOST_TEST_EQ(fd.exponent, exponent);
    BOOST_TEST_EQ(fd.sign, false);
}

// The digits read back as the value, end in a non-zero digit, and no number one digit shorter reads back as it.
template <typename T>
static void check_shortest(const T val)
{
    const auto fd {shortest(val)};
    BOOST_TEST(reads_back(fd.mantissa, fd.exponent, fd.sign, val));
    BOOST_TEST_NE(fd.mantissa % 10U, 0U);
    const auto shorter {fd.mantissa / 10U};
    BOOST_TEST(shorter == 0U || !reads_back(shorter, fd.exponent + 1, fd.sign, val));
    BOOST_TEST(!reads_back(shorter + 1U, fd.exponent + 1, fd.sign, val));
}

static void test_float()
{
    #ifdef BOOST_DECIMAL_HAS_CONSTEXPR_BITCAST
    static_assert(floating_point_to_fd32(0.1F).mantissa == 1U, "0.1 is 1E-1");
    static_assert(floating_point_to_fd32(0.1F).exponent == -1, "0.1 is 1E-1");
    #endif

    check_value(0.1F, 1U, -1);
    check_value(123.456F, 123456U, -3);
    check_value(16777216.0F, 16777216U, 0);
    check_value(float_from_bits(1U), 1U, -45);
    check_value(float_from_bits(3U), 4U, -45);
    check_value(float_from_bits(UINT32_C(0x007FFFFF)), 11754942U, -45);
    check_value(float_from_bits(UINT32_C(0x00800000)), 11754944U, -45);
    check_value(float_from_bits(UINT32_C(0x7F7FFFFF)), 34028235U, 31);
    check_value(float_from_bits((UINT32_C(127) - 35U) << 23U), 2910383U, -17);
    check_value(float_from_bits(UINT32_C(0x516A7EB4)), 62946755U, 3);

    const auto negative_zero {floating_point_to_fd32(-0.0F)};
    BOOST_TEST_EQ(negative_zero.mantissa, 0U);
    BOOST_TEST_EQ(negative_zero.exponent, 0);
    BOOST_TEST_EQ(negative_zero.sign, true);

    BOOST_TEST_EQ(floating_point_to_fd32(-2.5F).sign, true);
    BOOST_TEST(boost::decimal::decimal32_t {0.1F} == boost::decimal::decimal32_t(1U, -1));
    BOOST_TEST(boost::decimal::decimal64_t {123.456F} == boost::decimal::decimal64_t(123456U, -3));

    for (std::uint32_t exponent {1}; exponent < 255U; ++exponent)
    {
        check_shortest(float_from_bits(exponent << 23U));
    }

    std::mt19937 rng(42);
    std::uniform_int_distribution<std::uint32_t> dist;
    for (int i {}; i < 1000000; ++i)
    {
        const auto bits {dist(rng)};
        if (((bits >> 23U) & 0xFFU) != 0xFFU)
        {
            check_shortest(float_from_bits(bits));
        }
    }
}

static void test_double()
{
    #ifdef BOOST_DECIMAL_HAS_CONSTEXPR_BITCAST
    static_assert(floating_point_to_fd64(0.1).mantissa == 1U, "0.1 is 1E-1");
    static_assert(floating_point_to_fd64(0.1).exponent == -1, "0.1 is 1E-1");
    #endif

    check_value(0.1, 1U, -1);
    check_value(123456.78, 12345678U, -2);
    check_value(1e23, 1U, 23);
    check_value(double_from_bits(1U), 5U, -324);
    check_value(double_from_bits(3U), 15U, -324);
    check_value(double_from_bits(UINT64_C(0x000FFFFFFFFFFFFF)), UINT64_C(2225073858507201), -323);
    check_value(double_from_bits(UINT64_C(0x0010000000000000)), UINT64_C(22250738585072014), -324);
    check_value(double_from_bits(UINT64_C(0x7FEFFFFFFFFFFFFF)), UINT64_C(17976931348623157), 292);
    check_value(double_from_bits((UINT64_C(1023) - 77U) << 52U), UINT64_C(6617444900424222), -39);
    check_value(double_from_bits((UINT64_C(1023) + 63U) << 52U), UINT64_C(9223372036854776), 3);
    check_value(double_from_bits(UINT64_C(0x43F89DA997A854D9)), UINT64_C(28380164753330115), 3);

    const auto negative_zero {floating_point_to_fd64(-0.0)};
    BOOST_TEST_EQ(negative_zero.mantissa, 0U);
    BOOST_TEST_EQ(negative_zero.exponent, 0);
    BOOST_TEST_EQ(negative_zero.sign, true);

    BOOST_TEST_EQ(floating_point_to_fd64(-2.5).sign, true);
    BOOST_TEST(boost::decimal::decimal64_t {0.1} == boost::decimal::decimal64_t(1U, -1));
    BOOST_TEST(boost::decimal::decimal32_t {123456.78} == boost::decimal::decimal32_t(1234568U, -1));
    BOOST_TEST(boost::decimal::decimal128_t {0.1L} == boost::decimal::decimal128_t(1U, -1));

    for (std::uint64_t exponent {1}; exponent < 2047U; ++exponent)
    {
        check_shortest(double_from_bits(exponent << 52U));
    }

    std::mt19937_64 rng(42);
    for (int i {}; i < 1000000; ++i)
    {
        const auto bits {rng()};
        if (((bits >> 52U) & 0x7FFU) != 0x7FFU)
        {
            check_shortest(double_from_bits(bits));
        }
    }
}

int main()
{
    test_float();
    test_double();

    return boost::report_errors();
}
