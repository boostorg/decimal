// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//
// See: https://github.com/boostorg/decimal/issues/1383

#include <boost/decimal.hpp>
#include <boost/core/lightweight_test.hpp>

using namespace boost::decimal;

template <typename T>
void test_spot(const T lhs, const T rhs, const T result)
{
    BOOST_TEST_EQ(quantize(lhs, rhs), result);
}

template <typename T>
void test_driver()
{
    test_spot(T{0}, T{1}, T{0});
    test_spot(T{1}, T{1}, T{1});
    test_spot(T{1, -1}, T{1, 2}, T{0});
    test_spot(T{1, -1}, T{1, 1}, T{0});
    test_spot(T{1, -1}, T{1, -1}, T{1, -1});
}

int main()
{
    test_driver<decimal32_t>();
    test_driver<decimal64_t>();
    test_driver<decimal128_t>();

    return boost::report_errors();
}
