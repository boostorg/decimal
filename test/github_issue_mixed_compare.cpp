// Copyright 2026 Shen-Ta Hsieh
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//
// https://github.com/boostorg/decimal/issues/1563
//
// A comparison of two decimal types converted both values to the type with the larger size.
// A fast type can have the same size as an IEEE type with more digits, or with subnormal values.

#include <boost/decimal.hpp>
#include <boost/core/lightweight_test.hpp>

using namespace boost::decimal;
using namespace boost::decimal::literals;

// The values of small and large are not equal, and small is less than large
template <typename T1, typename T2>
void test_less(const T1 small, const T2 large)
{
    BOOST_TEST(!(small == large));
    BOOST_TEST(small != large);
    BOOST_TEST(small < large);
    BOOST_TEST(small <= large);
    BOOST_TEST(!(small > large));
    BOOST_TEST(!(small >= large));

    BOOST_TEST(!(large == small));
    BOOST_TEST(large != small);
    BOOST_TEST(!(large < small));
    BOOST_TEST(!(large <= small));
    BOOST_TEST(large > small);
    BOOST_TEST(large >= small);
}

int main()
{
    // The IEEE type has more digits than the fast type of the same size
    test_less(18496.97_DFF, 18496.9723_DD);
    test_less(1_DFF, 1.0000001_DD);
    test_less(-1.0000001_DD, -1_DFF);
    test_less(1_DDF, 1.0000000000000001_DL);
    test_less(8.2598786884044776e-22_DL, 8.259878688404478e-22_DDF);
    test_less(-1.0000000000000001_DL, -1_DDF);

    // The fast type has no subnormal values
    test_less(0_DFF, 1e-100_DF);
    test_less(-1e-100_DF, 0_DFF);
    test_less(0_DDF, 1e-390_DD);
    test_less(0_DLF, 1e-6170_DL);

    return boost::report_errors();
}
