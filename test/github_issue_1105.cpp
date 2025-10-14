// Copyright 2025 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//
// See: https://github.com/cppalliance/decimal/issues/1105

#include <boost/decimal.hpp>
#include <boost/core/lightweight_test.hpp>

using namespace boost::decimal;

template <typename T>
void test()
{
    const T one {1, 0};
    const T zero {0, 0};

    const T val {1, -5};
    int val_exp {};
    const auto val_sig {frexp10(val, &val_exp)};

    const auto next {nextafter(val, one)};
    int next_exp {};
    const auto next_sig {frexp10(next, &next_exp)};
    BOOST_TEST_EQ(next_exp, val_exp);
    BOOST_TEST_EQ(next_sig, val_sig + 1u);

    const auto prev {nextafter(val, zero)};
    int prev_exp {};
    const auto prev_sig {frexp10(prev, &prev_exp)};
    BOOST_TEST_EQ(prev_exp, val_exp - 1);
    BOOST_TEST_EQ(prev_sig, val_sig - 1u);
}

int main()
{
    test<decimal32_t>();

    return boost::report_errors();
}
