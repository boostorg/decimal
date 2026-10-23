// Copyright 2023 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//
// Checks that std::hash compiles and that it honours its contract: values which
// compare equal have to hash equally

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4324) // MSVC 14.2 complains that std::pair is padded
#endif

#include <boost/decimal/decimal32_t.hpp>
#include <boost/decimal/decimal64_t.hpp>
#include <boost/decimal/decimal128_t.hpp>
#include <boost/decimal/decimal_fast32_t.hpp>
#include <boost/decimal/decimal_fast64_t.hpp>
#include <boost/decimal/decimal_fast128_t.hpp>
#include <boost/decimal/bid_conversion.hpp>
#include <boost/decimal/dpd_conversion.hpp>
#include <boost/decimal/hash.hpp>
#include <boost/decimal/iostream.hpp>
#include <boost/core/lightweight_test.hpp>
#include <iostream>
#include <functional>
#include <unordered_map>
#include <unordered_set>
#include <limits>
#include <vector>
#include <array>

template <typename T>
void test_hash()
{
    std::hash<T> hasher;
    for (int i = 0; i < 100; ++i)
    {
        T dec_val(i);
        BOOST_TEST_EQ(hasher(dec_val), hasher(dec_val));
    }
}

// See: https://github.com/boostorg/decimal/issues/1120
template <typename T>
void test_hash_cohorts()
{
    const std::array<T, 7> values {
        T {3, 7},
        T {30, 6},
        T {300, 5},
        T {3000, 4},
        T {30000, 3},
        T {300000, 2},
        T {3000000, 1}
    };

    std::hash<T> hasher;

    for (const auto val1 : values)
    {
        for (const auto val2 : values)
        {
            BOOST_TEST_EQ(hasher(val1), hasher(val2));
        }
    }
}

// Builds every flavour of zero the library can produce, plus a spread of non-zero
// cohorts and the non-finite values
template <typename T>
std::vector<T> hash_test_values()
{
    using namespace boost::decimal;

    const T scaled {123, -2};

    std::vector<T> values {
        T {0, 0},
        -T {0, 0},
        T {0, 5},
        -T {0, 5},
        T {0, -5},
        -T {0, -5},
        T {0, 60},
        T {0, -60},
        // Zeros that arrive from an interchange encoding rather than a constructor
        from_bid<T>(to_bid(T {0, -60})),
        from_bid<T>(to_bid(-T {0, 60})),
        from_dpd<T>(to_dpd(T {0, 60})),
        from_dpd<T>(to_dpd(-T {0, -60})),
        // Zeros that arrive from arithmetic, which keep the quantum exponent of the operands
        scaled - scaled,
        scaled * T {0, -4},
        // Non-zero cohorts, whose hashes must stay unaffected
        T {1, 0},
        T {10, -1},
        T {100, -2},
        T {-1, 0},
        T {-10, -1},
        T {-100, -2},
        T {123, -2},
        T {12300, -4},
        T {-123, -2},
        T {-12300, -4}
    };

    #ifndef BOOST_DECIMAL_FAST_MATH
    values.push_back(std::numeric_limits<T>::infinity());
    values.push_back(-std::numeric_limits<T>::infinity());
    #endif

    return values;
}

// The std::hash contract: a == b implies hash(a) == hash(b)
template <typename T>
void test_hash_contract()
{
    const auto values {hash_test_values<T>()};
    std::hash<T> hasher;

    for (const auto lhs : values)
    {
        for (const auto rhs : values)
        {
            if (lhs == rhs && !BOOST_TEST_EQ(hasher(lhs), hasher(rhs)))
            {
                std::cerr << "Equal values hashed differently: " << lhs << " and " << rhs << std::endl;
            }
        }
    }
}

// The reason the contract matters: an unordered container keyed by a decimal
template <typename T>
void test_unordered_containers()
{
    const auto values {hash_test_values<T>()};

    const T reference_zero {0, 0};

    std::unordered_set<T> zeros {};
    for (const auto val : values)
    {
        if (val == reference_zero)
        {
            zeros.insert(val);
        }
    }

    // Every zero is the same key, whatever its sign, cohort or provenance
    BOOST_TEST_EQ(zeros.size(), std::size_t {1});

    std::unordered_map<T, int> counts {};
    for (const auto val : values)
    {
        ++counts[val];
    }

    for (const auto val : values)
    {
        if (!BOOST_TEST(counts.find(val) != counts.end()))
        {
            std::cerr << "Lookup failed for: " << val << std::endl;
        }
    }

    // Non-finite keys stay distinct from zero and from each other
    #ifndef BOOST_DECIMAL_FAST_MATH
    std::hash<T> hasher;
    const T pos_inf {std::numeric_limits<T>::infinity()};
    const T neg_inf {-pos_inf};

    BOOST_TEST_NE(hasher(pos_inf), hasher(reference_zero));
    BOOST_TEST_NE(hasher(neg_inf), hasher(reference_zero));
    BOOST_TEST_NE(hasher(pos_inf), hasher(neg_inf));
    BOOST_TEST_EQ(hasher(pos_inf), hasher(std::numeric_limits<T>::infinity()));
    BOOST_TEST_EQ(hasher(std::numeric_limits<T>::quiet_NaN()), hasher(std::numeric_limits<T>::quiet_NaN()));
    #endif

    // A zero stored under one sign is found under the other
    const T pos_zero {0, 0};
    const T neg_zero {-pos_zero};
    const T zero_cohort {0, 5};
    const T tiny_zero_cohort {0, -60};

    std::unordered_map<T, int> balance {};
    balance[pos_zero] = 42;
    BOOST_TEST_EQ(balance[neg_zero], 42);
    BOOST_TEST_EQ(balance[zero_cohort], 42);
    BOOST_TEST_EQ(balance[tiny_zero_cohort], 42);
    BOOST_TEST_EQ(balance.size(), std::size_t {1});
}

template <typename T>
void test_all()
{
    test_hash<T>();
    test_hash_cohorts<T>();
    test_hash_contract<T>();
    test_unordered_containers<T>();
}

// detail::hash_encoding has to keep encodings apart at whatever width std::size_t is.
// Every zero encoding, every infinity and every NaN has a zero low word, so where size_t
// is 32 bits and std::hash<std::uint64_t> is a truncating cast they would otherwise all
// share a bucket.
void test_encoding_fold()
{
    const std::uint64_t encodings[] {
        UINT64_C(0x31C0000000000000),   // decimal64_t zero
        UINT64_C(0x7800000000000000),   // positive infinity
        UINT64_C(0xF800000000000000),   // negative infinity
        UINT64_C(0x7C00000000000000),   // quiet NaN
        UINT64_C(0x7E00000000000000),   // signaling NaN
        UINT64_C(0x3040000000000000)    // decimal128_t zero, high word
    };

    constexpr std::size_t count {sizeof(encodings) / sizeof(encodings[0])};

    for (std::size_t i {}; i < count; ++i)
    {
        for (std::size_t j {i + 1}; j < count; ++j)
        {
            const auto lhs {boost::decimal::detail::hash_encoding(encodings[i])};
            const auto rhs {boost::decimal::detail::hash_encoding(encodings[j])};

            if (!BOOST_TEST_NE(lhs, rhs))
            {
                std::cerr << "Encodings collide once narrowed to std::size_t: index "
                          << i << " and " << j << std::endl;
            }
        }
    }
}

int main()
{
    test_all<boost::decimal::decimal32_t>();
    test_all<boost::decimal::decimal64_t>();
    test_all<boost::decimal::decimal128_t>();
    test_all<boost::decimal::decimal_fast32_t>();
    test_all<boost::decimal::decimal_fast64_t>();
    test_all<boost::decimal::decimal_fast128_t>();

    test_encoding_fold();

    return boost::report_errors();
}
