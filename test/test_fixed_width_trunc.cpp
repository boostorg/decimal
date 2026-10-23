// Copyright 2024 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include <boost/decimal.hpp>
#include <boost/core/lightweight_test.hpp>
#include <limits>

using namespace boost::decimal;

// precision is the number of digits kept after the decimal point
template <typename T>
void test()
{
    // 123.4567 is exactly representable in every decimal type
    constexpr T test_val {UINT32_C(1234567), -4};
    constexpr T validation_val_0 {UINT32_C(123), 0};
    constexpr T validation_val_1 {UINT32_C(1235), -1};
    constexpr T validation_val_2 {UINT32_C(12346), -2};
    constexpr T validation_val_3 {UINT32_C(123457), -3};

    BOOST_TEST_EQ(rescale(test_val, 0), rescale(test_val));
    BOOST_TEST_EQ(rescale(test_val, 0), validation_val_0);
    BOOST_TEST_EQ(rescale(test_val, 1), validation_val_1);
    BOOST_TEST_EQ(rescale(test_val, 2), validation_val_2);
    BOOST_TEST_EQ(rescale(test_val, 3), validation_val_3);
    BOOST_TEST_EQ(rescale(test_val, 4), test_val);

    // Asking for more fractional digits than val holds leaves it alone
    BOOST_TEST_EQ(rescale(test_val, 5), test_val);
    BOOST_TEST_EQ(rescale(test_val, 100), test_val);

    // Non-finite values
    for (int i = 0; i < 10; ++i)
    {
        BOOST_TEST(isinf(rescale(std::numeric_limits<T>::infinity(), i)));
        BOOST_TEST(isnan(rescale(std::numeric_limits<T>::quiet_NaN(), i)));
        BOOST_TEST(isnan(rescale(std::numeric_limits<T>::signaling_NaN(), i)));
        BOOST_TEST_EQ(rescale(T{0}, i), T{0});
    }

    // A value with no fractional part is unchanged for any non-negative precision
    constexpr T big_val {1, 20};
    for (int i = 0; i < 10; ++i)
    {
        BOOST_TEST_EQ(rescale(big_val, i), big_val);
    }
}

// rescale rounds, it does not truncate, and the default mode is ties to even
template <typename T>
void test_rounding()
{
    constexpr T half {UINT32_C(5), -1};
    constexpr T one_and_a_half {UINT32_C(15), -1};
    constexpr T two_and_a_half {UINT32_C(25), -1};
    constexpr T three_and_a_half {UINT32_C(35), -1};
    constexpr T two {UINT32_C(2), 0};
    constexpr T four {UINT32_C(4), 0};

    BOOST_TEST_EQ(rescale(half, 0), T{0});
    BOOST_TEST_EQ(rescale(one_and_a_half, 0), two);
    BOOST_TEST_EQ(rescale(two_and_a_half, 0), two);
    BOOST_TEST_EQ(rescale(three_and_a_half, 0), four);

    // Truncation would give 123 here
    constexpr T just_under_124 {UINT32_C(1239), -1};
    constexpr T val_124 {UINT32_C(124), 0};
    BOOST_TEST_EQ(rescale(just_under_124, 0), val_124);

    // Rounding that carries into an additional digit
    constexpr T nines_small {UINT32_C(999), -3};
    constexpr T one {UINT32_C(1), 0};
    constexpr T nines_large {UINT32_C(99999), -2};
    constexpr T one_thousand {UINT32_C(1000), 0};
    BOOST_TEST_EQ(rescale(nines_small, 2), one);
    BOOST_TEST_EQ(rescale(nines_large, 1), one_thousand);
}

// A negative precision rounds to the matching power of ten.
// Previously the unbounded digit count indexed past the end of the pow10 table.
template <typename T>
void test_negative_precision()
{
    constexpr T pos_val {UINT32_C(1234567), -4};
    constexpr T neg_val {-1234567, -4};
    constexpr T pos_tens {UINT32_C(12), 1};
    constexpr T pos_hundreds {UINT32_C(1), 2};
    constexpr T neg_tens {-12, 1};
    constexpr T neg_hundreds {-1, 2};

    BOOST_TEST_EQ(rescale(pos_val, -1), pos_tens);
    BOOST_TEST_EQ(rescale(pos_val, -2), pos_hundreds);
    BOOST_TEST_EQ(rescale(pos_val, -3), T{0});

    BOOST_TEST_EQ(rescale(neg_val, -1), neg_tens);
    BOOST_TEST_EQ(rescale(neg_val, -2), neg_hundreds);
    BOOST_TEST_EQ(rescale(neg_val, -3), T{0});

    // Everything rounds away, and the sign of the zero is kept
    for (int precision : {-6, -20, -100, -1000000, (std::numeric_limits<int>::min)()})
    {
        BOOST_TEST_EQ(rescale(pos_val, precision), T{0});
        BOOST_TEST_EQ(rescale(neg_val, precision), T{0});
        BOOST_TEST(!signbit(rescale(pos_val, precision)));
        BOOST_TEST(signbit(rescale(neg_val, precision)));
    }

    // Nothing to remove, so val comes back untouched
    for (int precision : {50, 1000, 1000000, (std::numeric_limits<int>::max)()})
    {
        BOOST_TEST_EQ(rescale(pos_val, precision), pos_val);
        BOOST_TEST_EQ(rescale(neg_val, precision), neg_val);
    }

    // Non-finite values are returned unmodified regardless of precision
    for (int precision : {-1, -20, -1000000})
    {
        BOOST_TEST(isinf(rescale(std::numeric_limits<T>::infinity(), precision)));
        BOOST_TEST(isnan(rescale(std::numeric_limits<T>::quiet_NaN(), precision)));
    }
}

// rescale must be symmetric about zero
template <typename T>
void test_sign_symmetry()
{
    constexpr T pos_val {UINT32_C(1234567), -4};
    constexpr T neg_val {-1234567, -4};
    constexpr T big_pos_val {UINT32_C(1234567), 40};
    constexpr T big_neg_val {-1234567, 40};

    for (int precision = -6; precision < 10; ++precision)
    {
        BOOST_TEST_EQ(rescale(neg_val, precision), -rescale(pos_val, precision));
        BOOST_TEST_EQ(rescale(big_neg_val, precision), -rescale(big_pos_val, precision));
    }

    // A value far above the fractional range is untouched by a non-negative precision
    for (int precision = 0; precision < 10; ++precision)
    {
        BOOST_TEST_EQ(rescale(big_pos_val, precision), big_pos_val);
        BOOST_TEST_EQ(rescale(big_neg_val, precision), big_neg_val);
    }
}

int main()
{
    test<decimal32_t>();
    test<decimal64_t>();
    test<decimal128_t>();

    test<decimal_fast32_t>();

    test_rounding<decimal32_t>();
    test_rounding<decimal64_t>();
    test_rounding<decimal128_t>();
    test_rounding<decimal_fast32_t>();

    test_negative_precision<decimal32_t>();
    test_negative_precision<decimal64_t>();
    test_negative_precision<decimal128_t>();
    test_negative_precision<decimal_fast32_t>();

    test_sign_symmetry<decimal32_t>();
    test_sign_symmetry<decimal64_t>();
    test_sign_symmetry<decimal128_t>();
    test_sign_symmetry<decimal_fast32_t>();

    return boost::report_errors();
}
