// Copyright 2025 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include <boost/decimal.hpp>
#include <boost/core/lightweight_test.hpp>
#include <random>
#include <climits>

using namespace boost::decimal;

static std::mt19937_64 rng(42);
static std::uniform_int_distribution<int> dist(INT_MIN, INT_MAX);
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
            BOOST_TEST(total_order(lhs, rhs));
        }
        else if (lhs_int > rhs_int)
        {
            BOOST_TEST(!total_order(lhs, rhs));
        }
    }
}

int main()
{
    test_unequal<decimal32_t>();
    test_unequal<decimal64_t>();
    test_unequal<decimal128_t>();

    return boost::report_errors();
}
