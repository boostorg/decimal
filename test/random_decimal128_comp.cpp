// Copyright 2023 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include "testing_config.hpp"
#include <boost/decimal.hpp>
#include <boost/core/lightweight_test.hpp>
#include <random>
#include <limits>

using namespace boost::decimal;
using namespace boost::decimal::literals;

#if !defined(BOOST_DECIMAL_REDUCE_TEST_DEPTH)
static constexpr auto N = static_cast<std::size_t>(1024); // Number of trials
#else
static constexpr auto N = static_cast<std::size_t>(1024 >> 4U); // Number of trials
#endif

// NOLINTNEXTLINE : Seed with a constant for repeatability
static std::mt19937_64 rng(42); // NOSONAR : Global rng is not const

template <typename T>
void random_LT(T lower, T upper)
{
    std::uniform_int_distribution<T> dist(lower, upper);

    for (std::size_t i {}; i < N; ++i)
    {
        const T val1 {dist(rng)};
        const T val2 {dist(rng)};

        const decimal128_t dec1 {val1};
        const decimal128_t dec2 {val2};

        if (!BOOST_TEST_EQ(dec1 < dec2, val1 < val2))
        {
            // LCOV_EXCL_START
            std::cerr << "Val 1: " << val1
                      << "\nDec 1: " << dec1
                      << "\nVal 2: " << val2
                      << "\nDec 2: " << dec2 << std::endl;
            // LCOV_EXCL_STOP
        }
    }

    // Edge cases
    BOOST_TEST(decimal128_t(dist(rng)) < std::numeric_limits<decimal128_t>::infinity());
    BOOST_TEST(!(decimal128_t(dist(rng)) < -std::numeric_limits<decimal128_t>::infinity()));

    // The payload of an infinity has no meaning
    const auto payload_inf {from_bits(boost::decimal::detail::d128_inf_mask | boost::int128::uint128_t{UINT64_C(0x03FFFFFFFFFFFFFF), UINT64_MAX})};
    BOOST_TEST(!(std::numeric_limits<decimal128_t>::infinity() < payload_inf));
    BOOST_TEST(!(payload_inf < std::numeric_limits<decimal128_t>::infinity()));
    BOOST_TEST(std::numeric_limits<decimal128_t>::infinity() == payload_inf);
    BOOST_TEST(!(decimal128_t(dist(rng)) < std::numeric_limits<decimal128_t>::quiet_NaN()));
    BOOST_TEST(!(std::numeric_limits<decimal128_t>::quiet_NaN() < std::numeric_limits<decimal128_t>::quiet_NaN()));
}

template <typename T>
void random_mixed_LT(T lower, T upper)
{
    std::uniform_int_distribution<T> dist(lower, upper);

    for (std::size_t i {}; i < N; ++i)
    {
        const T val1 {dist(rng)};
        const T val2 {dist(rng)};

        const decimal128_t dec1 {val1};
        const T dec2 {static_cast<T>(decimal128_t(val2))};

        if (!BOOST_TEST_EQ(dec1 < dec2, val1 < val2))
        {
            // LCOV_EXCL_START
            std::cerr << "Val 1: " << val1
                      << "\nDec 1: " << dec1
                      << "\nVal 2: " << val2
                      << "\nDec 2: " << dec2 << std::endl;
            // LCOV_EXCL_STOP
        }
    }

    // Reverse order of the operands
    for (std::size_t i {}; i < N; ++i)
    {
        const T val1 {dist(rng)};
        const T val2 {dist(rng)};

        const T dec1 {static_cast<T>(decimal128_t(val1))};
        const decimal128_t dec2 {val2};

        if (!BOOST_TEST_EQ(dec1 < dec2, val1 < val2))
        {
            // LCOV_EXCL_START
            std::cerr << "Val 1: " << val1
                      << "\nDec 1: " << dec1
                      << "\nVal 2: " << val2
                      << "\nDec 2: " << dec2 << std::endl;
            // LCOV_EXCL_STOP
        }
    }

    // Edge Cases
    BOOST_TEST_EQ(decimal128_t(1) < T(1), false);
    BOOST_TEST_EQ(decimal128_t(10) < T(10), false);
    BOOST_TEST_EQ(T(1) < decimal128_t(1), false);
    BOOST_TEST_EQ(T(10) < decimal128_t(10), false);
    BOOST_TEST_EQ(std::numeric_limits<decimal128_t>::infinity() < T(1), false);
    BOOST_TEST_EQ(-std::numeric_limits<decimal128_t>::infinity() < T(1), true);
    BOOST_TEST_EQ(std::numeric_limits<decimal128_t>::quiet_NaN() < T(1), false);
}

template <typename T>
void random_LE(T lower, T upper)
{
    std::uniform_int_distribution<T> dist(lower, upper);

    for (std::size_t i {}; i < N; ++i)
    {
        const T val1 {dist(rng)};
        const T val2 {dist(rng)};

        const decimal128_t dec1 {val1};
        const decimal128_t dec2 {val2};

        if (!BOOST_TEST_EQ(dec1 <= dec2, val1 <= val2))
        {
            // /LCOV_EXCL_START
            std::cerr << "Val 1: " << val1
                      << "\nDec 1: " << dec1
                      << "\nVal 2: " << val2
                      << "\nDec 2: " << dec2 << std::endl;
            // LCOV_EXCL_STOP
        }
    }

    BOOST_TEST(decimal128_t(dist(rng)) <= std::numeric_limits<decimal128_t>::infinity());
    BOOST_TEST(!(decimal128_t(dist(rng)) <= -std::numeric_limits<decimal128_t>::infinity()));
    BOOST_TEST(!(decimal128_t(dist(rng)) <= std::numeric_limits<decimal128_t>::quiet_NaN()));
    BOOST_TEST(!(std::numeric_limits<decimal128_t>::quiet_NaN() <= std::numeric_limits<decimal128_t>::quiet_NaN()));
    BOOST_TEST(std::numeric_limits<decimal128_t>::infinity() <= std::numeric_limits<decimal128_t>::infinity());
    BOOST_TEST(!(std::numeric_limits<decimal128_t>::infinity() <= -std::numeric_limits<decimal128_t>::infinity()));
    BOOST_TEST(-std::numeric_limits<decimal128_t>::infinity() <= -std::numeric_limits<decimal128_t>::infinity());
}

template <typename T>
void random_mixed_LE(T lower, T upper)
{
    std::uniform_int_distribution<T> dist(lower, upper);

    for (std::size_t i {}; i < N; ++i)
    {
        const T val1 {dist(rng)};
        const T val2 {dist(rng)};

        const decimal128_t dec1 {val1};
        const T dec2 {static_cast<T>(decimal128_t(val2))};

        if (!BOOST_TEST_EQ(dec1 <= dec2, val1 <= val2))
        {
            // LCOV_EXCL_START
            std::cerr << "Val 1: " << val1
                      << "\nDec 1: " << dec1
                      << "\nVal 2: " << val2
                      << "\nDec 2: " << dec2 << std::endl;
            // LCOV_EXCL_STOP
        }
    }

    BOOST_TEST(dist(rng) <= std::numeric_limits<decimal128_t>::infinity());
    BOOST_TEST(!(dist(rng) <= -std::numeric_limits<decimal128_t>::infinity()));
    BOOST_TEST(!(dist(rng) <= std::numeric_limits<decimal128_t>::quiet_NaN()));
}

template <typename T>
void random_GT(T lower, T upper)
{
    std::uniform_int_distribution<T> dist(lower, upper);

    for (std::size_t i {}; i < N; ++i)
    {
        const T val1 {dist(rng)};
        const T val2 {dist(rng)};

        const decimal128_t dec1 {val1};
        const decimal128_t dec2 {val2};

        if (!BOOST_TEST_EQ(dec1 > dec2, val1 > val2))
        {
            // LCOV_EXCL_START
            std::cerr << "Val 1: " << val1
                      << "\nDec 1: " << dec1
                      << "\nVal 2: " << val2
                      << "\nDec 2: " << dec2 << std::endl;
            // LCOV_EXCL_STOP
        }
    }

    BOOST_TEST(!(decimal128_t(dist(rng)) > std::numeric_limits<decimal128_t>::infinity()));
    BOOST_TEST((decimal128_t(dist(rng)) > -std::numeric_limits<decimal128_t>::infinity()));
    BOOST_TEST(!(decimal128_t(dist(rng)) > std::numeric_limits<decimal128_t>::quiet_NaN()));
    BOOST_TEST(!(std::numeric_limits<decimal128_t>::quiet_NaN() > std::numeric_limits<decimal128_t>::quiet_NaN()));
    BOOST_TEST(!(std::numeric_limits<decimal128_t>::infinity() > std::numeric_limits<decimal128_t>::infinity()));
    BOOST_TEST(std::numeric_limits<decimal128_t>::infinity() > -std::numeric_limits<decimal128_t>::infinity());
    BOOST_TEST(!(-std::numeric_limits<decimal128_t>::infinity() > -std::numeric_limits<decimal128_t>::infinity()));
}

template <typename T>
void random_mixed_GT(T lower, T upper)
{
    std::uniform_int_distribution<T> dist(lower, upper);

    for (std::size_t i {}; i < N; ++i)
    {
        const T val1 {dist(rng)};
        const T val2 {dist(rng)};

        const decimal128_t dec1 {val1};
        const T dec2 {static_cast<T>(decimal128_t(val2))};

        if (!BOOST_TEST_EQ(dec1 > dec2, val1 > val2))
        {
            // LCOV_EXCL_START
            std::cerr << "Val 1: " << val1
                      << "\nDec 1: " << dec1
                      << "\nVal 2: " << val2
                      << "\nDec 2: " << dec2 << std::endl;
            // LCOV_EXCL_STOP
        }
    }

    BOOST_TEST(!(dist(rng) > std::numeric_limits<decimal128_t>::infinity()));
    BOOST_TEST((dist(rng) > -std::numeric_limits<decimal128_t>::infinity()));
    BOOST_TEST(!(dist(rng) > std::numeric_limits<decimal128_t>::quiet_NaN()));
}

template <typename T>
void random_GE(T lower, T upper)
{
    std::uniform_int_distribution<T> dist(lower, upper);

    for (std::size_t i {}; i < N; ++i)
    {
        const T val1 {dist(rng)};
        const T val2 {dist(rng)};

        const decimal128_t dec1 {val1};
        const decimal128_t dec2 {val2};

        if (!BOOST_TEST_EQ(dec1 >= dec2, val1 >= val2))
        {
            // LCOV_EXCL_START
            std::cerr << "Val 1: " << val1
                      << "\nDec 1: " << dec1
                      << "\nVal 2: " << val2
                      << "\nDec 2: " << dec2 << std::endl;
            // LCOV_EXCL_STOP
        }
    }

    BOOST_TEST(!(decimal128_t(dist(rng)) >= std::numeric_limits<decimal128_t>::infinity()));
    BOOST_TEST((decimal128_t(dist(rng)) >= -std::numeric_limits<decimal128_t>::infinity()));
    BOOST_TEST(!(decimal128_t(dist(rng)) >= std::numeric_limits<decimal128_t>::quiet_NaN()));
    BOOST_TEST(!(std::numeric_limits<decimal128_t>::quiet_NaN() >= std::numeric_limits<decimal128_t>::quiet_NaN()));
}

template <typename T>
void random_mixed_GE(T lower, T upper)
{
    std::uniform_int_distribution<T> dist(lower, upper);

    for (std::size_t i {}; i < N; ++i)
    {
        const T val1 {dist(rng)};
        const T val2 {dist(rng)};

        const decimal128_t dec1 {val1};
        const T dec2 {static_cast<T>(decimal128_t(val2))};

        if (!BOOST_TEST_EQ(dec1 >= dec2, val1 >= val2))
        {
            // LCOV_EXCL_START
            std::cerr << "Val 1: " << val1
                      << "\nDec 1: " << dec1
                      << "\nVal 2: " << val2
                      << "\nDec 2: " << dec2 << std::endl;
            // LCOV_EXCL_STOP
        }
    }

    BOOST_TEST(!(dist(rng) >= std::numeric_limits<decimal128_t>::infinity()));
    BOOST_TEST((dist(rng) >= -std::numeric_limits<decimal128_t>::infinity()));
    BOOST_TEST(!(dist(rng) >= std::numeric_limits<decimal128_t>::quiet_NaN()));
}

template <typename T>
void random_EQ(T lower, T upper)
{
    std::uniform_int_distribution<T> dist(lower, upper);

    for (std::size_t i {}; i < N; ++i)
    {
        const T val1 {dist(rng)};
        const T val2 {dist(rng)};

        const decimal128_t dec1 {val1};
        const decimal128_t dec2 {val2};

        if (!BOOST_TEST_EQ(dec1 == dec2, val1 == val2))
        {
            // LCOV_EXCL_START
            std::cerr << "Val 1: " << val1
                      << "\nDec 1: " << dec1
                      << "\nVal 2: " << val2
                      << "\nDec 2: " << dec2 << std::endl;
            // LCOV_EXCL_STOP
        }
    }

    BOOST_TEST(!(std::numeric_limits<decimal128_t>::quiet_NaN() == std::numeric_limits<decimal128_t>::quiet_NaN()));
    BOOST_TEST(decimal128_t{0} == -decimal128_t{0});
}

template <typename T>
void random_mixed_EQ(T lower, T upper)
{
    std::uniform_int_distribution<T> dist(lower, upper);

    for (std::size_t i {}; i < N; ++i)
    {
        const T val1 {dist(rng)};
        const T val2 {dist(rng)};

        const decimal128_t dec1 {val1};
        const T dec2 {static_cast<T>(decimal128_t(val2))};

        if (!BOOST_TEST_EQ(dec1 == dec2, val1 == val2))
        {
            // LCOV_EXCL_START
            std::cerr << "Val 1: " << val1
                      << "\nDec 1: " << dec1
                      << "\nVal 2: " << val2
                      << "\nDec 2: " << dec2 << std::endl;
            // LCOV_EXCL_STOP
        }
    }

    for (std::size_t i {}; i < N; ++i)
    {
        const T val1 {dist(rng)};
        const T val2 {dist(rng)};

        const T dec1 {static_cast<T>(decimal128_t(val1))};
        const decimal128_t dec2 {val2};

        if (!BOOST_TEST_EQ(dec1 == dec2, val1 == val2))
        {
            // LCOV_EXCL_START
            std::cerr << "Val 1: " << val1
                      << "\nDec 1: " << dec1
                      << "\nVal 2: " << val2
                      << "\nDec 2: " << dec2 << std::endl;
            // LCOV_EXCL_STOP
        }
    }

    // Edge Cases
    BOOST_TEST_EQ(decimal128_t(1), T(1));
    BOOST_TEST_EQ(decimal128_t(10), T(10));
    BOOST_TEST_EQ(decimal128_t(100), T(100));
    BOOST_TEST_EQ(decimal128_t(1000), T(1000));
    BOOST_TEST_EQ(decimal128_t(10000), T(10000));
    BOOST_TEST_EQ(decimal128_t(100000), T(100000));
    BOOST_TEST_EQ(std::numeric_limits<decimal128_t>::quiet_NaN() == T(1), false);
    BOOST_TEST_EQ(std::numeric_limits<decimal128_t>::infinity() == T(1), false);
}

template <typename T>
void random_NE(T lower, T upper)
{
    std::uniform_int_distribution<T> dist(lower, upper);

    for (std::size_t i {}; i < N; ++i)
    {
        const T val1 {dist(rng)};
        const T val2 {dist(rng)};

        const decimal128_t dec1 {val1};
        const decimal128_t dec2 {val2};

        if (!BOOST_TEST_EQ(dec1 != dec2, val1 != val2))
        {
            // LCOV_EXCL_START
            std::cerr << "Val 1: " << val1
                      << "\nDec 1: " << dec1
                      << "\nVal 2: " << val2
                      << "\nDec 2: " << dec2 << std::endl;
            // LCOV_EXCL_STOP
        }
    }

    BOOST_TEST((std::numeric_limits<decimal128_t>::quiet_NaN() != std::numeric_limits<decimal128_t>::quiet_NaN()));
    BOOST_TEST(!(decimal128_t{0} != -decimal128_t{0}));
}

template <typename T>
void random_mixed_NE(T lower, T upper)
{
    std::uniform_int_distribution<T> dist(lower, upper);

    for (std::size_t i {}; i < N; ++i)
    {
        const T val1 {dist(rng)};
        const T val2 {dist(rng)};

        const decimal128_t dec1 {val1};
        const T dec2 {static_cast<T>(decimal128_t(val2))};

        if (!BOOST_TEST_EQ(dec1 != dec2, val1 != val2))
        {
            // LCOV_EXCL_START
            std::cerr << "Val 1: " << val1
                      << "\nDec 1: " << dec1
                      << "\nVal 2: " << val2
                      << "\nDec 2: " << dec2 << std::endl;
            // LCOV_EXCL_STOP
        }
    }
}

#ifdef BOOST_DECIMAL_HAS_SPACESHIP_OPERATOR
template <typename T>
void random_SPACESHIP(T lower, T upper)
{
    std::uniform_int_distribution<T> dist(lower, upper);

    for (std::size_t i {}; i < N; ++i)
    {
        const T val1 {dist(rng)};
        const T val2 {dist(rng)};

        const decimal128_t dec1 {val1};
        const decimal128_t dec2 {val2};

        if (!BOOST_TEST((dec1 <=> dec2) == (val1 <=> val2)))
        {
            // LCOV_EXCL_START
            std::cerr << "Val 1: " << val1
                      << "\nDec 1: " << dec1
                      << "\nVal 2: " << val2
                      << "\nDec 2: " << dec2 << std::endl;
            // LCOV_EXCL_STOP
        }
    }
    BOOST_TEST((decimal128_t(dist(rng)) <=> std::numeric_limits<decimal128_t>::quiet_NaN()) == std::partial_ordering::unordered);
    BOOST_TEST((std::numeric_limits<decimal128_t>::quiet_NaN() <=> std::numeric_limits<decimal128_t>::quiet_NaN()) == std::partial_ordering::unordered);
}

template <typename T>
void random_mixed_SPACESHIP(T lower, T upper)
{
    std::uniform_int_distribution<T> dist(lower, upper);

    for (std::size_t i {}; i < N; ++i)
    {
        const T val1 {dist(rng)};
        const T val2 {dist(rng)};

        const decimal128_t dec1 {val1};
        const T dec2 {static_cast<T>(decimal128_t(val2))};

        if (!BOOST_TEST((dec1 <=> dec2) == (val1 <=> val2)))
        {
            // LCOV_EXCL_START
            std::cerr << "Val 1: " << val1
                      << "\nDec 1: " << dec1
                      << "\nVal 2: " << val2
                      << "\nDec 2: " << dec2 << std::endl;
            // LCOV_EXCL_STOP
        }
    }
    BOOST_TEST((dist(rng) <=> std::numeric_limits<decimal128_t>::quiet_NaN()) == std::partial_ordering::unordered);
    BOOST_TEST((std::numeric_limits<decimal128_t>::quiet_NaN() <=> std::numeric_limits<decimal128_t>::quiet_NaN()) == std::partial_ordering::unordered);
}

#endif

// Values whose exponents differ by more than the 4 digits of room in the significand
void test_far_exponents()
{
    const decimal128_t one_e10 {1, 10};
    const decimal128_t ten_digits {UINT64_C(10000000000), 0};
    const decimal128_t ten_digits_plus_one {UINT64_C(10000000001), 0};
    const decimal128_t one_e40 {1, 40};

    BOOST_TEST(one_e10 == ten_digits);
    BOOST_TEST(ten_digits == one_e10);
    BOOST_TEST(one_e10 != ten_digits_plus_one);
    BOOST_TEST(one_e40 != 1_DL);
    BOOST_TEST(1_DL != one_e40);
    BOOST_TEST("1234567890123456789012345678901e8"_DL != 1_DL);
    BOOST_TEST(1_DL != "1234567890123456789012345678901e8"_DL);
    BOOST_TEST(decimal128_t(9, 13) != decimal128_t(UINT64_C(90000000204928), 0));

    BOOST_TEST(one_e10 < ten_digits_plus_one);
    BOOST_TEST(!(ten_digits_plus_one < one_e10));
    BOOST_TEST(-ten_digits_plus_one < -one_e10);
    BOOST_TEST(!(-one_e10 < -ten_digits_plus_one));
    BOOST_TEST(2_DL < one_e10);
    BOOST_TEST(!(one_e10 < 2_DL));
    BOOST_TEST(-one_e10 < -2_DL);
    BOOST_TEST(!(-2_DL < -one_e10));
}

int main()
{
    test_far_exponents();

    random_LT(std::numeric_limits<int>::min(), std::numeric_limits<int>::max());
    random_LT(std::numeric_limits<unsigned>::min(), std::numeric_limits<unsigned>::max());
    random_LT(std::numeric_limits<long>::min(), std::numeric_limits<long>::max());
    random_LT(std::numeric_limits<unsigned long>::min(), std::numeric_limits<unsigned long>::max());
    random_LT(std::numeric_limits<long long>::min(), std::numeric_limits<long long>::max());
    random_LT(std::numeric_limits<unsigned long long>::min(), std::numeric_limits<unsigned long long>::max());

    random_LE(std::numeric_limits<int>::min(), std::numeric_limits<int>::max());
    random_LE(std::numeric_limits<unsigned>::min(), std::numeric_limits<unsigned>::max());
    random_LE(std::numeric_limits<long>::min(), std::numeric_limits<long>::max());
    random_LE(std::numeric_limits<unsigned long>::min(), std::numeric_limits<unsigned long>::max());
    random_LE(std::numeric_limits<long long>::min(), std::numeric_limits<long long>::max());
    random_LE(std::numeric_limits<unsigned long long>::min(), std::numeric_limits<unsigned long long>::max());

    random_GT(std::numeric_limits<int>::min(), std::numeric_limits<int>::max());
    random_GT(std::numeric_limits<unsigned>::min(), std::numeric_limits<unsigned>::max());
    random_GT(std::numeric_limits<long>::min(), std::numeric_limits<long>::max());
    random_GT(std::numeric_limits<unsigned long>::min(), std::numeric_limits<unsigned long>::max());
    random_GT(std::numeric_limits<long long>::min(), std::numeric_limits<long long>::max());
    random_GT(std::numeric_limits<unsigned long long>::min(), std::numeric_limits<unsigned long long>::max());

    random_GE(std::numeric_limits<int>::min(), std::numeric_limits<int>::max());
    random_GE(std::numeric_limits<unsigned>::min(), std::numeric_limits<unsigned>::max());
    random_GE(std::numeric_limits<long>::min(), std::numeric_limits<long>::max());
    random_GE(std::numeric_limits<unsigned long>::min(), std::numeric_limits<unsigned long>::max());
    random_GE(std::numeric_limits<long long>::min(), std::numeric_limits<long long>::max());
    random_GE(std::numeric_limits<unsigned long long>::min(), std::numeric_limits<unsigned long long>::max());

    random_EQ(std::numeric_limits<int>::min(), std::numeric_limits<int>::max());
    random_EQ(std::numeric_limits<unsigned>::min(), std::numeric_limits<unsigned>::max());
    random_EQ(std::numeric_limits<long>::min(), std::numeric_limits<long>::max());
    random_EQ(std::numeric_limits<unsigned long>::min(), std::numeric_limits<unsigned long>::max());
    random_EQ(std::numeric_limits<long long>::min(), std::numeric_limits<long long>::max());
    random_EQ(std::numeric_limits<unsigned long long>::min(), std::numeric_limits<unsigned long long>::max());

    random_NE(std::numeric_limits<int>::min(), std::numeric_limits<int>::max());
    random_NE(std::numeric_limits<unsigned>::min(), std::numeric_limits<unsigned>::max());
    random_NE(std::numeric_limits<long>::min(), std::numeric_limits<long>::max());
    random_NE(std::numeric_limits<unsigned long>::min(), std::numeric_limits<unsigned long>::max());
    random_NE(std::numeric_limits<long long>::min(), std::numeric_limits<long long>::max());
    random_NE(std::numeric_limits<unsigned long long>::min(), std::numeric_limits<unsigned long long>::max());

    #if (defined(__clang__) || defined(_MSC_VER) || !defined(__GNUC__) || (defined(__GNUC__) && __GNUC__ > 6))
    random_mixed_LT(std::numeric_limits<int>::min(), std::numeric_limits<int>::max());
    random_mixed_LT(std::numeric_limits<unsigned>::min(), std::numeric_limits<unsigned>::max());
    random_mixed_LT(std::numeric_limits<long>::min(), std::numeric_limits<long>::max());
    random_mixed_LT(std::numeric_limits<unsigned long>::min(), std::numeric_limits<unsigned long>::max());
    random_mixed_LT(std::numeric_limits<long long>::min(), std::numeric_limits<long long>::max());
    random_mixed_LT(std::numeric_limits<unsigned long long>::min(), std::numeric_limits<unsigned long long>::max());

    random_mixed_LE(std::numeric_limits<int>::min(), std::numeric_limits<int>::max());
    random_mixed_LE(std::numeric_limits<unsigned>::min(), std::numeric_limits<unsigned>::max());
    random_mixed_LE(std::numeric_limits<long>::min(), std::numeric_limits<long>::max());
    random_mixed_LE(std::numeric_limits<unsigned long>::min(), std::numeric_limits<unsigned long>::max());
    random_mixed_LE(std::numeric_limits<long long>::min(), std::numeric_limits<long long>::max());
    random_mixed_LE(std::numeric_limits<unsigned long long>::min(), std::numeric_limits<unsigned long long>::max());

    random_mixed_GT(std::numeric_limits<int>::min(), std::numeric_limits<int>::max());
    random_mixed_GT(std::numeric_limits<unsigned>::min(), std::numeric_limits<unsigned>::max());
    random_mixed_GT(std::numeric_limits<long>::min(), std::numeric_limits<long>::max());
    random_mixed_GT(std::numeric_limits<unsigned long>::min(), std::numeric_limits<unsigned long>::max());
    random_mixed_GT(std::numeric_limits<long long>::min(), std::numeric_limits<long long>::max());
    random_mixed_GT(std::numeric_limits<unsigned long long>::min(), std::numeric_limits<unsigned long long>::max());

    random_mixed_GE(std::numeric_limits<int>::min(), std::numeric_limits<int>::max());
    random_mixed_GE(std::numeric_limits<unsigned>::min(), std::numeric_limits<unsigned>::max());
    random_mixed_GE(std::numeric_limits<long>::min(), std::numeric_limits<long>::max());
    random_mixed_GE(std::numeric_limits<unsigned long>::min(), std::numeric_limits<unsigned long>::max());
    random_mixed_GE(std::numeric_limits<long long>::min(), std::numeric_limits<long long>::max());
    random_mixed_GE(std::numeric_limits<unsigned long long>::min(), std::numeric_limits<unsigned long long>::max());

    random_mixed_EQ(std::numeric_limits<int>::min(), std::numeric_limits<int>::max());
    random_mixed_EQ(std::numeric_limits<unsigned>::min(), std::numeric_limits<unsigned>::max());
    random_mixed_EQ(std::numeric_limits<long>::min(), std::numeric_limits<long>::max());
    random_mixed_EQ(std::numeric_limits<unsigned long>::min(), std::numeric_limits<unsigned long>::max());
    random_mixed_EQ(std::numeric_limits<long long>::min(), std::numeric_limits<long long>::max());
    random_mixed_EQ(std::numeric_limits<unsigned long long>::min(), std::numeric_limits<unsigned long long>::max());

    random_mixed_NE(std::numeric_limits<int>::min(), std::numeric_limits<int>::max());
    random_mixed_NE(std::numeric_limits<unsigned>::min(), std::numeric_limits<unsigned>::max());
    random_mixed_NE(std::numeric_limits<long>::min(), std::numeric_limits<long>::max());
    random_mixed_NE(std::numeric_limits<unsigned long>::min(), std::numeric_limits<unsigned long>::max());
    random_mixed_NE(std::numeric_limits<long long>::min(), std::numeric_limits<long long>::max());
    random_mixed_NE(std::numeric_limits<unsigned long long>::min(), std::numeric_limits<unsigned long long>::max());
    #endif

    #ifdef BOOST_DECIMAL_HAS_SPACESHIP_OPERATOR
    random_SPACESHIP(std::numeric_limits<int>::min(), std::numeric_limits<int>::max());
    random_SPACESHIP(std::numeric_limits<unsigned>::min(), std::numeric_limits<unsigned>::max());
    random_SPACESHIP(std::numeric_limits<long>::min(), std::numeric_limits<long>::max());
    random_SPACESHIP(std::numeric_limits<unsigned long>::min(), std::numeric_limits<unsigned long>::max());
    random_SPACESHIP(std::numeric_limits<long long>::min(), std::numeric_limits<long long>::max());
    random_SPACESHIP(std::numeric_limits<unsigned long long>::min(), std::numeric_limits<unsigned long long>::max());

    random_mixed_SPACESHIP(std::numeric_limits<int>::min(), std::numeric_limits<int>::max());
    random_mixed_SPACESHIP(std::numeric_limits<unsigned>::min(), std::numeric_limits<unsigned>::max());
    random_mixed_SPACESHIP(std::numeric_limits<long>::min(), std::numeric_limits<long>::max());
    random_mixed_SPACESHIP(std::numeric_limits<unsigned long>::min(), std::numeric_limits<unsigned long>::max());
    random_mixed_SPACESHIP(std::numeric_limits<long long>::min(), std::numeric_limits<long long>::max());
    random_mixed_SPACESHIP(std::numeric_limits<unsigned long long>::min(), std::numeric_limits<unsigned long long>::max());
    #endif

    constexpr auto pos_zero = boost::decimal::decimal128_t{0U, 0, false};
    constexpr auto neg_zero = boost::decimal::decimal128_t{0U, 0, true};
    BOOST_TEST(pos_zero == neg_zero);

    return boost::report_errors();
}
