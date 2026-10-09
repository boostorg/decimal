// Copyright 2026 Pavel P
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include <boost/decimal.hpp>

#if defined(__GNUC__) && !defined(__clang__)
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wuseless-cast"
#endif
#include <boost/charconv.hpp>
#if defined(__GNUC__) && !defined(__clang__)
#  pragma GCC diagnostic pop
#endif

#include <boost/core/lightweight_test.hpp>
#include <cfloat>
#include <cstdint>
#include <cstring>
#include <limits>
#include <random>

using namespace boost::decimal;

template <typename DecimalType>
static auto same_bits(const DecimalType lhs, const DecimalType rhs) -> bool
{
    return to_bid(lhs) == to_bid(rhs);
}

static auto double_from_bits(const std::uint64_t bits) -> double
{
    double val {};
    std::memcpy(&val, &bits, sizeof(val));
    return val;
}

static auto float_from_bits(const std::uint32_t bits) -> float
{
    float val {};
    std::memcpy(&val, &bits, sizeof(val));
    return val;
}

// Charconv prints the exact binary value correctly rounded to the requested digits
template <typename DecimalType, typename T>
static void check_against_charconv(const T val)
{
    char buffer[64];
    const auto digits {std::numeric_limits<DecimalType>::digits10};
    const auto printed {boost::charconv::to_chars(buffer, buffer + sizeof(buffer), val,
                                                  boost::charconv::chars_format::scientific, digits - 1)};
    DecimalType expected {};
    from_chars(buffer, printed.ptr, expected);
    BOOST_TEST_EQ(from_binary<DecimalType>(val), expected);
}

// The cohorts are the ones Intel's binary64_to_bid32/64/128 give
static void test_known_values()
{
    #ifdef BOOST_DECIMAL_HAS_CONSTEXPR_BITCAST
    static_assert(from_binary<decimal64_t>(2.5) == decimal64_t(25U, -1), "2.5 is exact");
    #endif

    BOOST_TEST(same_bits(from_binary<decimal64_t>(1.0), decimal64_t {1U, 0}));
    BOOST_TEST(same_bits(from_binary<decimal64_t>(0.5), decimal64_t {5U, -1}));
    BOOST_TEST(same_bits(from_binary<decimal64_t>(-3.75), decimal64_t {375U, -2, true}));
    BOOST_TEST(same_bits(from_binary<decimal64_t>(100.0), decimal64_t {100U, 0}));
    BOOST_TEST(same_bits(from_binary<decimal64_t>(1e20), decimal64_t {UINT64_C(1000000000000000), 5}));
    BOOST_TEST(same_bits(from_binary<decimal64_t>(1e23), decimal64_t {UINT64_C(9999999999999999), 7}));
    BOOST_TEST(same_bits(from_binary<decimal64_t>(0.1), decimal64_t {UINT64_C(1000000000000000), -16}));
    BOOST_TEST(same_bits(from_binary<decimal64_t>(99.99), decimal64_t {UINT64_C(9998999999999999), -14}));
    BOOST_TEST(same_bits(from_binary<decimal64_t>(0.07), decimal64_t {UINT64_C(7000000000000001), -17}));
    const auto min_subnormal {double_from_bits(1U)};
    BOOST_TEST(same_bits(from_binary<decimal64_t>(min_subnormal), decimal64_t {UINT64_C(4940656458412465), -339}));
    BOOST_TEST(same_bits(from_binary<decimal64_t>(DBL_MAX), decimal64_t {UINT64_C(1797693134862316), 293}));

    BOOST_TEST(same_bits(from_binary<decimal32_t>(0.1), decimal32_t {1000000U, -7}));
    BOOST_TEST(same_bits(from_binary<decimal32_t>(99.99), decimal32_t {9999000U, -5}));
    BOOST_TEST(same_bits(from_binary<decimal32_t>(1e20), decimal32_t {1000000U, 14}));

    const boost::int128::uint128_t point_one {UINT64_C(0x314DC6448D93), UINT64_C(0x3986922312364CE3)};
    const boost::int128::uint128_t max_double {UINT64_C(0x58A213CC7A4F), UINT64_C(0xFAE03C4825156FB4)};
    const boost::int128::uint128_t ten_to_20 {UINT64_C(5), UINT64_C(0x6BC75E2D63100000)};
    BOOST_TEST(same_bits(from_binary<decimal128_t>(0.1), decimal128_t {point_one, -34}));
    BOOST_TEST(same_bits(from_binary<decimal128_t>(DBL_MAX), decimal128_t {max_double, 275}));
    BOOST_TEST(same_bits(from_binary<decimal128_t>(1e20), decimal128_t {ten_to_20, 0}));

    BOOST_TEST(same_bits(from_binary<decimal64_t>(0.1F), decimal64_t {UINT64_C(1000000014901161), -16}));
    BOOST_TEST(same_bits(from_binary<decimal32_t>(0.1F), decimal32_t {1000000U, -7}));
    BOOST_TEST(same_bits(from_binary<decimal64_t>(16777216.0F), decimal64_t {16777216U, 0}));

    // The constructors keep the shortest decimal instead
    BOOST_TEST(from_binary<decimal64_t>(0.07) != decimal64_t {0.07});
    BOOST_TEST(from_binary<decimal_fast64_t>(0.07) == decimal_fast64_t(UINT64_C(7000000000000001), -17));
}

static void test_special_values()
{
    BOOST_TEST(isnan(from_binary<decimal64_t>(std::numeric_limits<double>::quiet_NaN())));
    BOOST_TEST(isinf(from_binary<decimal32_t>(-std::numeric_limits<double>::infinity())));
    BOOST_TEST(signbit(from_binary<decimal32_t>(-std::numeric_limits<double>::infinity())));
    BOOST_TEST(same_bits(from_binary<decimal64_t>(-0.0), decimal64_t {0U, 0, true}));
    BOOST_TEST(isinf(from_binary<decimal32_t>(1e300)));
    BOOST_TEST(from_binary<decimal32_t>(1e-300) == decimal32_t(0U, 0));
}

static void test_rounding_modes()
{
    // fesetround changes the mode only when the library can find a constant evaluation
    #ifndef BOOST_DECIMAL_NO_CONSTEVAL_DETECTION
    fesetround(rounding_mode::fe_dec_downward);
    BOOST_TEST(same_bits(from_binary<decimal64_t>(0.1), decimal64_t {UINT64_C(1000000000000000), -16}));
    BOOST_TEST(same_bits(from_binary<decimal64_t>(-0.1), decimal64_t {UINT64_C(1000000000000001), -16, true}));
    fesetround(rounding_mode::fe_dec_upward);
    BOOST_TEST(same_bits(from_binary<decimal64_t>(0.1), decimal64_t {UINT64_C(1000000000000001), -16}));
    fesetround(rounding_mode::fe_dec_toward_zero);
    BOOST_TEST(same_bits(from_binary<decimal64_t>(0.07), decimal64_t {UINT64_C(7000000000000000), -17}));
    fesetround(rounding_mode::fe_dec_to_nearest);
    #endif
}

static void test_random_values()
{
    std::mt19937_64 rng(42);
    for (int i {}; i < 100000; ++i)
    {
        const auto bits {rng()};
        const auto exponent_bits {(bits >> 52U) & 0x7FFU};
        const auto val {double_from_bits(bits)};
        // Charconv (1.91 and develop) prints some subnormals ten times too large (6.654853158546871e-310 as ...e-309)
        if (exponent_bits != 0U && exponent_bits != 0x7FFU)
        {
            check_against_charconv<decimal64_t>(val);
            check_against_charconv<decimal128_t>(val);
            if (val > -1e90 && val < 1e90 && (val > 1e-90 || val < -1e-90))
            {
                check_against_charconv<decimal32_t>(val);
            }
        }

        const auto fval {float_from_bits(static_cast<std::uint32_t>(bits))};
        if (((static_cast<std::uint32_t>(bits) >> 23U) & 0xFFU) != 0xFFU)
        {
            check_against_charconv<decimal32_t>(fval);
            check_against_charconv<decimal64_t>(fval);
        }
    }
}

int main()
{
    test_known_values();
    test_special_values();
    test_rounding_modes();
    test_random_values();

    return boost::report_errors();
}
