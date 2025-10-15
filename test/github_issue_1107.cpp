// Copyright 2025 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//
// See: https://github.com/cppalliance/decimal/issues/1107

#include <boost/decimal.hpp>
#include <boost/core/lightweight_test.hpp>
#include <limits>
#include <random>

using namespace boost::decimal;

static std::mt19937_64 rng(42);
static std::uniform_int_distribution<int> dist(1, 10);

// 7.2.d
template <typename T>
void test_add_sub()
{
    const T a {dist(rng) * std::numeric_limits<T>::infinity()};
    const T b {-dist(rng) * std::numeric_limits<T>::infinity()};

    BOOST_TEST(isnan(b + a)); // -inf + inf
    BOOST_TEST(isnan(b - b)); // -inf + inf
}

int main()
{
    test_add_sub<decimal32_t>();
    test_add_sub<decimal64_t>();
    test_add_sub<decimal128_t>();
    test_add_sub<decimal_fast32_t>();
    test_add_sub<decimal_fast64_t>();
    test_add_sub<decimal_fast128_t>();

    return boost::report_errors();
}
