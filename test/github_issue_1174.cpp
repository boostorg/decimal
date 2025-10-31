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
    const decimal64_t a {dist(rng) * 7, detail::etiny_v<decimal64_t>};

    BOOST_TEST_GT(a, zero);

    const decimal64_t b {dist(rng) * 7, detail::etiny_v<decimal64_t> - 1};

    BOOST_TEST_EQ(b, sub_min); // Should be rounded up

    const decimal64_t c {dist(rng) * 7, detail::etiny_v<decimal64_t> - 2};

    BOOST_TEST_EQ(c, zero); // Should be flushed to 0
}

template <>
void test<decimal32_t>()
{
    constexpr decimal32_t zero {0};
    constexpr auto sub_min {std::numeric_limits<decimal32_t>::denorm_min()};
    const decimal32_t a {dist(rng) * 7, detail::etiny_v<decimal32_t>};

    BOOST_TEST_GT(a, zero);

    const decimal32_t b {dist(rng) * 7, detail::etiny_v<decimal32_t> - 1};

    BOOST_TEST_EQ(b, sub_min);

    const decimal32_t c {dist(rng) * 7, detail::etiny_v<decimal32_t> - 2};

    BOOST_TEST_EQ(c, zero);
}

template <>
void test<decimal128_t>()
{
    constexpr decimal128_t zero {0};
    constexpr auto sub_min {std::numeric_limits<decimal128_t>::denorm_min()};
    const decimal128_t a {dist(rng) * 7, detail::etiny_v<decimal128_t>};

    BOOST_TEST_GT(a, zero);

    const decimal128_t b {dist(rng) * 7, detail::etiny_v<decimal128_t> - 1};

    BOOST_TEST_EQ(b, sub_min);

    const decimal128_t c {dist(rng) * 7, detail::etiny_v<decimal128_t> - 2};

    BOOST_TEST_EQ(c, zero);
}

int main()
{
    test<decimal64_t>();
    test<decimal32_t>();
    test<decimal128_t>();

    return boost::report_errors();
}
