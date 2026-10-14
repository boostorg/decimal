// Copyright 2024 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#define BOOST_DECIMAL_FAST_MATH

#include "testing_config.hpp"
#include <boost/decimal.hpp>
#include <boost/core/lightweight_test.hpp>
#include <random>
#include <limits>
#include <functional>
#include <cstdint>

using namespace boost::decimal;

#if !defined(BOOST_DECIMAL_REDUCE_TEST_DEPTH)
static constexpr auto N = static_cast<std::size_t>(1024); // Number of trials
#else
static constexpr auto N = static_cast<std::size_t>(1024 >> 4U); // Number of trials
#endif

// NOLINTNEXTLINE : Seed with a constant for repeatability
static std::mt19937_64 rng(42); // NOSONAR : Global rng is not const

template <typename T>
void random_addition()
{
    std::uniform_int_distribution<std::int64_t> dist(1, 1000);

    for (std::size_t i {}; i < N; ++i)
    {
        const T val1 {dist(rng)};
        const T val2 {dist(rng)};

        const T dec1 {val1};
        const T dec2 {val2};

        const T res = dec1 + dec2;
        const auto res_int = static_cast<T>(res);

        if (!BOOST_TEST_EQ(res_int, val1 + val2))
        {
            // LCOV_EXCL_START
            std::cerr << "Val 1: " << val1
                      << "\nDec 1: " << dec1
                      << "\nVal 2: " << val2
                      << "\nDec 2: " << dec2
                      << "\nDec res: " << res
                      << "\nInt res: " << val1 + val2 << std::endl;
            // LCOV_EXCL_STOP
        }
    }
}

template <typename T>
void random_subtraction()
{
    std::uniform_int_distribution<std::int64_t> dist(1, 1000);

    for (std::size_t i {}; i < N; ++i)
    {
        const T val1 {dist(rng)};
        const T val2 {dist(rng)};

        const T dec1 {val1};
        const T dec2 {val2};

        const T res = dec1 - dec2;
        const auto res_int = static_cast<T>(res);

        if (!BOOST_TEST_EQ(res_int, val1 - val2))
        {
            // LCOV_EXCL_START
            std::cerr << "Val 1: " << val1
                      << "\nDec 1: " << dec1
                      << "\nVal 2: " << val2
                      << "\nDec 2: " << dec2
                      << "\nDec res: " << res
                      << "\nInt res: " << val1 + val2 << std::endl;
            // LCOV_EXCL_STOP
        }
    }
}

template <typename T>
void random_multiplication()
{
    std::uniform_int_distribution<std::int64_t> dist(1, 1000);

    for (std::size_t i {}; i < N; ++i)
    {
        const T val1 {dist(rng)};
        const T val2 {dist(rng)};

        const T dec1 {val1};
        const T dec2 {val2};

        const T res = dec1 * dec2;
        const auto res_int = static_cast<T>(res);

        if (!BOOST_TEST_EQ(res_int, val1 * val2))
        {
            // LCOV_EXCL_START
            std::cerr << "Val 1: " << val1
                      << "\nDec 1: " << dec1
                      << "\nVal 2: " << val2
                      << "\nDec 2: " << dec2
                      << "\nDec res: " << res
                      << "\nInt res: " << val1 + val2 << std::endl;
            // LCOV_EXCL_STOP
        }
    }
}

template <typename T>
void random_division()
{
    std::uniform_int_distribution<std::int64_t> dist(1, 1000);

    for (std::size_t i {}; i < N; ++i)
    {
        const T val1 {dist(rng)};
        const T val2 {dist(rng)};

        const T dec1 {val1};
        const T dec2 {val2};

        const T res = dec1 / dec2;
        const auto res_int = static_cast<T>(res);

        if (!BOOST_TEST_EQ(res_int, val1 / val2))
        {
            // LCOV_EXCL_START
            std::cerr << "Val 1: " << val1
                      << "\nDec 1: " << dec1
                      << "\nVal 2: " << val2
                      << "\nDec 2: " << dec2
                      << "\nDec res: " << res
                      << "\nInt res: " << val1 + val2 << std::endl;
            // LCOV_EXCL_STOP
        }
    }
}

template <typename T>
void test_comparisions()
{
    std::uniform_int_distribution<std::int64_t> dist(-1000, 1000);

    for (std::size_t i {}; i < N; ++i)
    {
        const T val1 {dist(rng)};
        const T val2 {dist(rng)};

        const T dec1 {val1};
        const T dec2 {val2};

        BOOST_TEST_EQ(val1 == val2, dec1 == dec2);
        BOOST_TEST_EQ(val1 != val2, dec1 != dec2);
        BOOST_TEST_EQ(val1 < val2, dec1 < dec2);
        BOOST_TEST_EQ(val1 <= val2, dec1 <= dec2);
        BOOST_TEST_EQ(val1 > val2, dec1 > dec2);
        BOOST_TEST_EQ(val1 >= val2, dec1 >= dec2);
    }
}

// Fast math removes all checks for non-finite values, so every value is finite and none
// is an infinity or a NaN. Which overload answers must not change the answer: the per type
// isfinite and the generic template used to disagree, and that quietly turned to_dpd into
// to_bid, since the DPD encoders bail out to BID for non-finite input.
template <typename T>
void test_classification()
{
    const T values[] {
        T {0, 0},
        -T {0, 0},
        T {1, 0},
        T {-1, 0},
        T {123, -2},
        std::numeric_limits<T>::min(),
        std::numeric_limits<T>::max(),
        std::numeric_limits<T>::denorm_min()
    };

    for (const auto val : values)
    {
        BOOST_TEST(isfinite(val));
        BOOST_TEST(!isinf(val));
        BOOST_TEST(!isnan(val));
        BOOST_TEST(!issignaling(val));
        // Qualified, because the explicit template argument form would otherwise also
        // match the global isfinite template that the MSVC UCRT declares
        BOOST_TEST_EQ(isfinite(val), boost::decimal::isfinite<T>(val));

        const auto classification {fpclassify(val)};
        BOOST_TEST(classification != FP_INFINITE);
        BOOST_TEST(classification != FP_NAN);
    }
}

// Both interchange encodings have to round trip, and they have to stay distinct
template <typename T>
void test_encodings()
{
    const T values[] {
        T {0, 0},
        -T {0, 0},
        T {0, 5},
        T {1, 0},
        T {-1, 0},
        T {123, -2},
        T {-123, -2},
        T {1234567, -3},
        std::numeric_limits<T>::min(),
        std::numeric_limits<T>::denorm_min()
    };

    for (const auto val : values)
    {
        BOOST_TEST_EQ(from_dpd<T>(to_dpd(val)), val);
        BOOST_TEST_EQ(from_bid<T>(to_bid(val)), val);
    }

    // DPD and BID are different encodings, so a non-trivial value must not land on the
    // same bit pattern in both
    const T probe {1234567, -3};
    BOOST_TEST(to_dpd(probe) != to_bid(probe));
}

// Signed and cohort zeros compare and hash the same way they do without fast math
template <typename T>
void test_zeros()
{
    const T pos_zero {0, 0};
    const T neg_zero {-pos_zero};
    const T zero_cohort {0, 20};
    const T one {1, 0};
    const T neg_one {-one};

    BOOST_TEST(pos_zero == neg_zero);
    BOOST_TEST(!(pos_zero < neg_zero));
    BOOST_TEST(!(neg_zero < pos_zero));
    BOOST_TEST(pos_zero == zero_cohort);
    BOOST_TEST(!(pos_zero < zero_cohort));
    BOOST_TEST(!(zero_cohort < pos_zero));
    BOOST_TEST(neg_zero == 0);
    BOOST_TEST(!(neg_zero < 0));
    BOOST_TEST(neg_one < pos_zero);
    BOOST_TEST(neg_one < neg_zero);
    BOOST_TEST(pos_zero < one);
    BOOST_TEST(neg_zero < one);

    std::hash<T> hasher;
    BOOST_TEST_EQ(hasher(pos_zero), hasher(neg_zero));
    BOOST_TEST_EQ(hasher(pos_zero), hasher(zero_cohort));
}

// sqrt of a zero is zero: the root search needs a nonzero significand
template <typename T>
void test_sqrt_zero()
{
    const T pos_zero {0, 0};
    const T neg_zero {-pos_zero};
    const T zero_cohort {0, 20};

    BOOST_TEST_EQ(sqrt(pos_zero), pos_zero);
    BOOST_TEST_EQ(sqrt(neg_zero), pos_zero);
    BOOST_TEST_EQ(sqrt(zero_cohort), pos_zero);
}

template <typename T>
void test_non_arithmetic()
{
    test_classification<T>();
    test_encodings<T>();
    test_zeros<T>();
}

int main()
{
    random_addition<decimal32_t>();
    random_subtraction<decimal32_t>();
    random_multiplication<decimal32_t>();
    random_division<decimal32_t>();
    test_comparisions<decimal32_t>();

    random_addition<decimal_fast32_t>();
    random_subtraction<decimal_fast32_t>();
    random_multiplication<decimal_fast32_t>();
    random_division<decimal_fast32_t>();
    test_comparisions<decimal_fast32_t>();

    random_addition<decimal64_t>();
    random_subtraction<decimal64_t>();
    random_multiplication<decimal64_t>();
    random_division<decimal64_t>();
    test_comparisions<decimal64_t>();

    random_addition<decimal_fast64_t>();
    random_subtraction<decimal_fast64_t>();
    random_multiplication<decimal_fast64_t>();
    random_division<decimal_fast64_t>();
    test_comparisions<decimal_fast64_t>();

    #if !defined(BOOST_DECIMAL_REDUCE_TEST_DEPTH)
    random_addition<decimal128_t>();
    random_subtraction<decimal128_t>();
    random_multiplication<decimal128_t>();
    random_division<decimal128_t>();
    test_comparisions<decimal128_t>();
    #endif

    test_non_arithmetic<decimal32_t>();
    test_non_arithmetic<decimal64_t>();
    test_non_arithmetic<decimal128_t>();
    test_non_arithmetic<decimal_fast32_t>();
    test_non_arithmetic<decimal_fast64_t>();
    test_non_arithmetic<decimal_fast128_t>();

    // A zero that reaches the root search fails constant evaluation with an out-of-range shift
    static_assert(sqrt(decimal32_t {0, 0}) == decimal32_t {0, 0}, "");
    static_assert(sqrt(decimal64_t {0, 0}) == decimal64_t {0, 0}, "");
    static_assert(sqrt(decimal128_t {0, 0}) == decimal128_t {0, 0}, "");
    test_sqrt_zero<decimal32_t>();
    test_sqrt_zero<decimal64_t>();
    test_sqrt_zero<decimal128_t>();
    test_sqrt_zero<decimal_fast32_t>();
    test_sqrt_zero<decimal_fast64_t>();
    test_sqrt_zero<decimal_fast128_t>();

    return boost::report_errors();
}
