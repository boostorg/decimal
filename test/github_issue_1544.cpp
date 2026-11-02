// Copyright 2026 Shen-Ta Hsieh
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//
// See: https://github.com/boostorg/decimal/issues/1544

#include <boost/decimal.hpp>
#include <boost/core/lightweight_test.hpp>
#include <algorithm>
#include <string>

using namespace boost::decimal;
using namespace boost::decimal::literals;

template <typename T>
void check_long(const std::string& literal)
{
    std::string plain {literal};
    plain.erase(std::remove(plain.begin(), plain.end(), '\''), plain.end());
    T expected {};
    BOOST_TEST(from_chars(plain.data(), plain.data() + plain.size(), expected));
    BOOST_TEST_EQ(detail::from_literal<T>(literal.c_str()), expected);
}

template <typename T>
void test_long()
{
    std::string ones {"1"};
    ones.reserve(1U + 1000U * 4U);
    for (int k {}; k < 1000; ++k)
    {
        ones += "'111";
    }

    std::string zeros {"0"};
    zeros.reserve(1U + 1000000U * 2U);
    for (int k {}; k < 1000000; ++k)
    {
        zeros += "'0";
    }

    // Separators after or inside eight digits, after the last digit the type keeps, and in a long exponent
    check_long<T>("12345678'12345678'12345678'12345678'12345678.5");
    check_long<T>("1234'5678'9012'3456'7890'1234'5678'9012'34");
    check_long<T>(ones);
    check_long<T>(ones + "." + ones + "e-3'0'0'0");
    check_long<T>("0.0'" + std::string(2000U, '0') + "'1");
    check_long<T>("0'0'0.0'0'1e1'0'0'0'0'5");
    check_long<T>("1e-1'0'0'0'0'0'0");
    check_long<T>("1'" + zeros + "e-1'0'0'0'0'0'3");

    // The same values without a second parse: 1 and 1000001 zeros, times 10^-1000003, is 10^-2
    BOOST_TEST_EQ(detail::from_literal<T>(("1'" + zeros + "e-1'0'0'0'0'0'3").c_str()), T(1, -2));

    // A directed rounding gives the smallest or the largest finite value in place of zero or infinity
    #if !defined(BOOST_DECIMAL_FE_DEC_DOWNWARD) && !defined(BOOST_DECIMAL_FE_DEC_TOWARD_ZERO) && !defined(BOOST_DECIMAL_FE_DEC_UPWARD)
    BOOST_TEST_EQ(detail::from_literal<T>("1e-1'0'0'0'0'0'0"), T{0});
    #ifndef BOOST_DECIMAL_FAST_MATH
    BOOST_TEST(isinf(detail::from_literal<T>("0'0'0.0'0'1e1'0'0'0'0'5")));
    #endif
    #endif
}

int main()
{
    // The digit separators are not a part of the value
    static_assert(1'000'000.25_DD == decimal64_t{100000025, -2}, "");
    BOOST_TEST_EQ(1'000_DF, decimal32_t{1000});
    BOOST_TEST_EQ(1'000_df, decimal32_t{1000});
    BOOST_TEST_EQ(12'345.678'9_DD, decimal64_t(123456789, -4));
    BOOST_TEST_EQ(1'2_dd, decimal64_t{12});
    BOOST_TEST_EQ(1.5e1'0_DL, decimal128_t(15, 9));
    BOOST_TEST_EQ(0.000'1_dl, decimal128_t(1, -4));
    BOOST_TEST_EQ(1'000_DFF, decimal_fast32_t{1000});
    BOOST_TEST_EQ(1'000_dff, decimal_fast32_t{1000});
    BOOST_TEST_EQ(1'000_DDF, decimal_fast64_t{1000});
    BOOST_TEST_EQ(1'000_ddf, decimal_fast64_t{1000});
    BOOST_TEST_EQ(1'000_DLF, decimal_fast128_t{1000});
    BOOST_TEST_EQ(1'000_dlf, decimal_fast128_t{1000});

    // A long literal gives the same value with separators as without them
    test_long<decimal32_t>();
    test_long<decimal64_t>();
    test_long<decimal128_t>();
    test_long<decimal_fast32_t>();
    test_long<decimal_fast64_t>();
    test_long<decimal_fast128_t>();

    return boost::report_errors();
}
