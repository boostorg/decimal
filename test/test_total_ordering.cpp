// Copyright 2025 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include <boost/decimal.hpp>
#include <boost/core/lightweight_test.hpp>
#include <random>
#include <limits>
#include <string>
#include <climits>

using namespace boost::decimal;

static std::mt19937_64 rng(42);
static std::uniform_int_distribution<int> dist(INT_MIN, INT_MAX);
static std::uniform_int_distribution<unsigned> payload_dist(0U, 10U);
static constexpr std::size_t N {1024};

template <typename T>
void test_unequal()
{
    for (std::size_t i {}; i < N; ++i)
    {
        const auto lhs_int {dist(rng)};
        const auto rhs_int {dist(rng)};

        const T lhs {lhs_int};
        const T rhs {rhs_int};

        if (lhs_int < rhs_int)
        {
            BOOST_TEST(comparetotal(lhs, rhs));
        }
        else if (lhs_int > rhs_int)
        {
            BOOST_TEST(!comparetotal(lhs, rhs));
        }
    }
}

template <typename T>
void test_part_d12()
{
    for (std::size_t i {}; i < N / 2U; ++i)
    {
        const auto rhs_int {dist(rng)};

        const auto lhs {-std::numeric_limits<T>::quiet_NaN()};
        const T rhs {rhs_int};

        BOOST_TEST(comparetotal(lhs, rhs));
        BOOST_TEST(!comparetotal(rhs, lhs));
        BOOST_TEST(!comparetotal(-lhs, rhs));
        BOOST_TEST(comparetotal(rhs, -lhs));
    }

    for (std::size_t i {}; i < N / 2U; ++i)
    {
        const auto lhs_int {dist(rng)};

        const T lhs {lhs_int};
        const auto rhs {std::numeric_limits<T>::signaling_NaN()};

        BOOST_TEST(comparetotal(lhs, rhs));
        BOOST_TEST(!comparetotal(rhs, lhs));
        BOOST_TEST(!comparetotal(lhs, -rhs));
        BOOST_TEST(comparetotal(-rhs, lhs));
    }
}

template <typename T>
void test_part_d3()
{
    // d.3.i
    for (std::size_t i {}; i < N / 3; ++i)
    {
        const auto lhs_int {dist(rng)};
        const auto rhs_int {dist(rng)};

        const T lhs {lhs_int * -std::numeric_limits<T>::quiet_NaN()};
        const T rhs {rhs_int * std::numeric_limits<T>::quiet_NaN()};

        BOOST_TEST(comparetotal(lhs, rhs));
        BOOST_TEST(!comparetotal(rhs, lhs));
        BOOST_TEST(comparetotal(rhs, rhs));
        BOOST_TEST(!comparetotal(std::numeric_limits<T>::signaling_NaN(), lhs));
        BOOST_TEST(!comparetotal(rhs, -std::numeric_limits<T>::signaling_NaN()));
    }

    // d.3.ii
    for (std::size_t i {}; i < N / 3; ++i)
    {
        const auto rhs_int {dist(rng)};

        const T lhs {std::numeric_limits<T>::signaling_NaN()};
        const T rhs {rhs_int * std::numeric_limits<T>::quiet_NaN()};

        BOOST_TEST(comparetotal(lhs, rhs));
        BOOST_TEST(!comparetotal(rhs, lhs));
        BOOST_TEST(comparetotal(rhs, rhs));

        const T neg_lhs {-std::numeric_limits<T>::signaling_NaN()};
        const T neg_rhs {rhs_int * -std::numeric_limits<T>::quiet_NaN()};

        BOOST_TEST(!comparetotal(neg_lhs, neg_rhs));
        BOOST_TEST(comparetotal(neg_rhs, neg_lhs));
    }

    // d.3.iii
    for (std::size_t i {}; i < N / 3; ++i)
    {
        const auto lhs_int {payload_dist(rng)};
        const auto rhs_int {payload_dist(rng)};

        const auto lhs_int_string {std::to_string(lhs_int)};
        const auto rhs_int_string {std::to_string(rhs_int)};

        const auto lhs {nan<T>(lhs_int_string.c_str())};
        const auto rhs {nan<T>(rhs_int_string.c_str())};

        if (lhs_int < rhs_int)
        {
            BOOST_TEST(comparetotal(lhs, rhs));
            BOOST_TEST(!comparetotal(rhs, lhs));
        }
        else if (lhs_int > rhs_int)
        {
            BOOST_TEST(!comparetotal(lhs, rhs));
            BOOST_TEST(comparetotal(rhs, lhs));
        }
        else
        {
            BOOST_TEST(comparetotal(lhs, rhs));
            BOOST_TEST(comparetotal(rhs, lhs));
        }
    }
}

// Parts c.1 and c.2: -0 orders before +0. A value also orders before or the same as itself.
template <typename T>
void test_equal_values()
{
    const T zero {0};
    const T neg_zero {-zero};
    BOOST_TEST(comparetotal(neg_zero, zero));
    BOOST_TEST(!comparetotal(zero, neg_zero));

    const auto inf {std::numeric_limits<T>::infinity()};
    BOOST_TEST(comparetotal(-inf, inf));
    BOOST_TEST(!comparetotal(inf, -inf));

    for (const T value : {zero, neg_zero, inf, -inf, T{1}, T{-1}, T{15, -1}, T{-15, -1},
                          (std::numeric_limits<T>::max)(), (std::numeric_limits<T>::min)()})
    {
        BOOST_TEST(comparetotal(value, value));
    }
}

// Part c.3: the exponent orders two representations of one value, and a negative sign reverses it
template <typename T>
void test_cohorts()
{
    // Each row has one value with a smaller and with a larger exponent
    const T rows[][2] {{T{10, -1}, T{1, 0}},
                       {T{1000, 0}, T{1, 3}},
                       {T{50, -3}, T{5, -2}},
                       {T{0, -1}, T{0, 0}},
                       {T{0, 0}, T{0, 5}}};

    for (const auto& row : rows)
    {
        BOOST_TEST(row[0] == row[1]);
        BOOST_TEST(comparetotal(row[0], row[1]));
        BOOST_TEST(!comparetotal(row[1], row[0]));
        BOOST_TEST(comparetotal(-row[1], -row[0]));
        BOOST_TEST(!comparetotal(-row[0], -row[1]));
    }
}

// The fast types have no cohorts, so equal values of one sign order both ways, zeros with
// different exponents too
template <typename T>
void test_fast_equal_values()
{
    const T rows[][2] {{T{10, -1}, T{1, 0}},
                       {T{0, -1}, T{0, 0}},
                       {T{0, 0}, T{0, 5}}};

    for (const auto& row : rows)
    {
        BOOST_TEST(comparetotal(row[0], row[1]));
        BOOST_TEST(comparetotal(row[1], row[0]));
        BOOST_TEST(comparetotal(-row[0], -row[1]));
        BOOST_TEST(comparetotal(-row[1], -row[0]));
        BOOST_TEST(comparetotal(-row[0], row[1]));
        BOOST_TEST(!comparetotal(row[0], -row[1]));
    }
}

int main()
{
    test_unequal<decimal32_t>();
    test_unequal<decimal64_t>();
    test_unequal<decimal128_t>();

    test_unequal<decimal_fast32_t>();
    test_unequal<decimal_fast64_t>();
    test_unequal<decimal_fast128_t>();

    test_part_d12<decimal32_t>();
    test_part_d12<decimal64_t>();
    test_part_d12<decimal128_t>();

    test_part_d12<decimal_fast32_t>();
    test_part_d12<decimal_fast64_t>();
    test_part_d12<decimal_fast128_t>();

    test_part_d3<decimal32_t>();
    test_part_d3<decimal64_t>();
    test_part_d3<decimal128_t>();

    test_part_d3<decimal_fast32_t>();
    test_part_d3<decimal_fast64_t>();
    test_part_d3<decimal_fast128_t>();

    test_equal_values<decimal32_t>();
    test_equal_values<decimal64_t>();
    test_equal_values<decimal128_t>();

    test_equal_values<decimal_fast32_t>();
    test_equal_values<decimal_fast64_t>();
    test_equal_values<decimal_fast128_t>();

    test_cohorts<decimal32_t>();
    test_cohorts<decimal64_t>();
    test_cohorts<decimal128_t>();

    test_fast_equal_values<decimal_fast32_t>();
    test_fast_equal_values<decimal_fast64_t>();
    test_fast_equal_values<decimal_fast128_t>();

    return boost::report_errors();
}
