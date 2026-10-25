// Copyright 2026 Shen-Ta Hsieh
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//
// See: https://github.com/boostorg/decimal/issues/1540

#include <boost/decimal.hpp>
#include <boost/core/lightweight_test.hpp>
#include <cstring>
#include <limits>
#include <string>

using namespace boost::decimal;

template <typename T>
void check(const char* str, const T expected, const std::size_t length)
{
    T value {};
    const auto r {from_chars(str, str + std::strlen(str), value)};
    BOOST_TEST(r);
    BOOST_TEST_EQ(value, expected);
    BOOST_TEST_EQ(signbit(value), signbit(expected));
    BOOST_TEST_EQ(static_cast<std::size_t>(r.ptr - str), length);

    char* end {};
    value = strtod<T>(str, &end);
    BOOST_TEST_EQ(value, expected);
    BOOST_TEST_EQ(signbit(value), signbit(expected));
    BOOST_TEST_EQ(static_cast<std::size_t>(end - str), length);
}

template <typename T>
void test()
{
    const T zero {0};
    const auto inf {std::numeric_limits<T>::infinity()};

    // A value far outside the range is zero or infinity, and all of its exponent digits are read
    check("1e-999999", zero, 9);
    check("0e-999999", zero, 9);
    check("-1.5e-99999999999999999999", -zero, 26);
    check("1e99999999999999999999", inf, 22);
    check("-1e2147483648", -inf, 13);

    // A five digit negative exponent with text after it
    check("2e-82064x", zero, 8);

    // Leading zeros of the exponent do not saturate it
    check("1e-000000000000000000001", T{1, -1}, 24);

    // The digit positions can cancel most of a long exponent
    const std::string zeros(100000U, '0');
    const auto small {"0." + zeros + "1e"};
    const auto large {"1" + zeros + "e-"};
    check((small + "1000000").c_str(), inf, small.size() + 7U);
    check((small + "100005").c_str(), T{1, 4}, small.size() + 6U);
    check((large + "100003").c_str(), T{1, -3}, large.size() + 6U);
    check((large + "1000000").c_str(), zero, large.size() + 7U);
}

int main()
{
    test<decimal32_t>();
    test<decimal64_t>();
    test<decimal128_t>();
    test<decimal_fast32_t>();
    test<decimal_fast64_t>();
    test<decimal_fast128_t>();

    return boost::report_errors();
}
