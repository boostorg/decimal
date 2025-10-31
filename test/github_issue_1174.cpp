// Copyright 2025 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//
// See: https://github.com/cppalliance/decimal/issues/1174
//
// The decTest tests only apply to decimal64_t so we need to cover the other two as well

#include <boost/decimal.hpp>
#include <boost/core/lightweight_test.hpp>
#include <random>

static std::mt19937_64 rng(42);
static std::uniform_int_distribution<std::int32_t> dist(1, 1);

using namespace boost::decimal;

template <typename T>
void test();

// These values are all from clamp.decTest
// The tests for decimal32_t and decimal128_t are derived from the values used here
template <>
void test<decimal64_t>()
{
    constexpr decimal64_t zero {0};
    constexpr auto sub_min {std::numeric_limits<decimal64_t>::denorm_min()};
    const decimal64_t a {dist(rng) * 7, -398};

    BOOST_TEST_GT(a, zero);

    const decimal64_t b {dist(rng) * 7, -399};

    BOOST_TEST_EQ(b, sub_min); // Should be rounded up

    const decimal64_t c {dist(rng) * 7, -400};

    BOOST_TEST_EQ(c, zero); // Should be flushed to 0
}

template <>
void test<decimal32_t>()
{
    constexpr decimal32_t zero {0};
    constexpr auto sub_min {std::numeric_limits<decimal32_t>::denorm_min()};
    const decimal32_t a {dist(rng) * 7, -101};

    BOOST_TEST_GT(a, zero);

    const decimal32_t b {dist(rng) * 7, -102};

    BOOST_TEST_EQ(b, sub_min);

    const decimal32_t c {dist(rng) * 7, -103};

    BOOST_TEST_EQ(c, zero);
}

int main()
{
    test<decimal64_t>();
    test<decimal32_t>();

    return boost::report_errors();
}
