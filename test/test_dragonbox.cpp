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

using boost::decimal::detail::dragonbox::floating_point_to_fd64;

static auto from_bits(const std::uint64_t bits) -> double
{
    double val {};
    std::memcpy(&val, &bits, sizeof(val));
    return val;
}

static auto to_bits(const double val) -> std::uint64_t
{
    std::uint64_t bits {};
    std::memcpy(&bits, &val, sizeof(bits));
    return bits;
}

static auto reads_back(const std::uint64_t mantissa, const int exponent, const bool sign, const double val) -> bool
{
    char buffer[64];
    std::snprintf(buffer, sizeof(buffer), "%s%" PRIu64 "e%d", sign ? "-" : "", mantissa, exponent);
    return to_bits(std::strtod(buffer, nullptr)) == to_bits(val);
}

static void check_value(const double val, const std::uint64_t mantissa, const std::int32_t exponent)
{
    const auto fd {floating_point_to_fd64(val)};
    BOOST_TEST_EQ(fd.mantissa, mantissa);
    BOOST_TEST_EQ(fd.exponent, exponent);
    BOOST_TEST_EQ(fd.sign, false);
}

// The digits read back as the double, end in a non-zero digit, and no number one digit shorter reads back as it.
static void check_shortest(const double val)
{
    const auto fd {floating_point_to_fd64(val)};
    BOOST_TEST(reads_back(fd.mantissa, fd.exponent, fd.sign, val));
    BOOST_TEST_NE(fd.mantissa % 10U, 0U);
    const auto shorter {fd.mantissa / 10U};
    BOOST_TEST(shorter == 0U || !reads_back(shorter, fd.exponent + 1, fd.sign, val));
    BOOST_TEST(!reads_back(shorter + 1U, fd.exponent + 1, fd.sign, val));
}

int main()
{
    #ifdef BOOST_DECIMAL_HAS_CONSTEXPR_BITCAST
    static_assert(floating_point_to_fd64(0.1).mantissa == 1U, "0.1 is 1E-1");
    static_assert(floating_point_to_fd64(0.1).exponent == -1, "0.1 is 1E-1");
    #endif

    check_value(0.1, 1U, -1);
    check_value(123456.78, 12345678U, -2);
    check_value(1e23, 1U, 23);
    check_value(from_bits(1), 5U, -324);
    check_value(from_bits(3), 15U, -324);
    check_value(from_bits(UINT64_C(0x000FFFFFFFFFFFFF)), UINT64_C(2225073858507201), -323);
    check_value(from_bits(UINT64_C(0x0010000000000000)), UINT64_C(22250738585072014), -324);
    check_value(from_bits(UINT64_C(0x7FEFFFFFFFFFFFFF)), UINT64_C(17976931348623157), 292);
    check_value(from_bits((UINT64_C(1023) - 77U) << 52U), UINT64_C(6617444900424222), -39);
    check_value(from_bits((UINT64_C(1023) + 63U) << 52U), UINT64_C(9223372036854776), 3);
    check_value(from_bits(UINT64_C(0x43F89DA997A854D9)), UINT64_C(28380164753330115), 3);

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
        check_shortest(from_bits(exponent << 52));
    }

    std::mt19937_64 rng(42);
    for (int i {}; i < 1000000; ++i)
    {
        const auto bits {rng()};
        if (((bits >> 52) & 0x7FFU) != 0x7FFU)
        {
            check_shortest(from_bits(bits));
        }
    }

    return boost::report_errors();
}
