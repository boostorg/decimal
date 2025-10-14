// Copyright 2025 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include <boost/decimal.hpp>
#include <boost/core/lightweight_test.hpp>

using namespace boost::decimal;

template <typename T>
void test_trivial()
{
    const auto str = "42";
    const T str_val {str};
    const T int_val {42};
    BOOST_TEST_EQ(str_val, int_val);

    // We allow plus signs here by popular demand
    const auto str2 = "+1.2e+3";
    const T str2_val {str2};
    const T int2_val {12, 2};
    BOOST_TEST_EQ(str2_val, int2_val);
}

#ifndef BOOST_DECIMAL_DISABLE_EXCEPTIONS

template <typename T>
void test_invalid()
{
    BOOST_TEST_THROWS(T("orange"), std::runtime_error);
    BOOST_TEST_THROWS(T(nullptr), std::runtime_error);
}

#else

template <typename T>
void test_invalid()
{
    BOOST_TEST(isnan(T("orange")));
}

#endif

template <typename T>
void test_nonfinite()
{
    const auto nan_str = "nan";
    const T nan_val {nan_str};
    BOOST_TEST(isnan(nan_val));
    BOOST_TEST(!signbit(nan_val));

    const auto inf_str = "inf";
    const T inf_val {inf_str};
    BOOST_TEST(isinf(inf_val));
    BOOST_TEST(!signbit(inf_val));

    const auto neg_inf_str = "-inf";
    const T neg_inf_val {neg_inf_str};
    BOOST_TEST(isinf(neg_inf_val));
    BOOST_TEST(signbit(neg_inf_val));
}

int main()
{
    test_trivial<decimal32_t>();
    test_invalid<decimal32_t>();
    test_nonfinite<decimal32_t>();

    return boost::report_errors();
}
