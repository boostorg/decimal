// Copyright 2025 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//
// See: https://github.com/cppalliance/decimal/issues/1260

#include <boost/decimal.hpp>
#include <boost/core/lightweight_test.hpp>

template <typename T>
void test()
{
    const T lhs {"1E34"};
    const T rhs {"-0.51"};
    const T res {"9999999999999999999999999999999999"};

    const T add_val {lhs + rhs};
    BOOST_TEST_EQ(add_val, res);

    const T sub_val {lhs - boost::decimal::abs(rhs)};
    BOOST_TEST_EQ(sub_val, res);
}

template <typename T>
void test2()
{
    const T lhs {"1E34"};
    const T rhs {"-0.501"};
    const T res {"9999999999999999999999999999999999"};

    const T add_val {lhs + rhs};
    BOOST_TEST_EQ(add_val, res);

    const T sub_val {lhs - boost::decimal::abs(rhs)};
    BOOST_TEST_EQ(sub_val, res);
}

int main()
{
    test<boost::decimal::decimal128_t>();
    test<boost::decimal::decimal_fast128_t>();

    test2<boost::decimal::decimal128_t>();
    test2<boost::decimal::decimal_fast128_t>();
    return boost::report_errors();
}
