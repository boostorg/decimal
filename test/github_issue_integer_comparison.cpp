// Copyright 2026 Shen-Ta Hsieh
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//
// See: https://github.com/boostorg/decimal/issues/1564

#include <boost/decimal.hpp>
#include <boost/core/lightweight_test.hpp>
#include <cstdint>
#include <limits>
#include <type_traits>

using namespace boost::decimal;

// x equals the integer n
template <typename T, typename Integer>
void test_equal(const T x, const Integer n)
{
    BOOST_TEST(x == n);
    BOOST_TEST(n == x);
    BOOST_TEST(!(x != n));
    BOOST_TEST(!(n != x));
    BOOST_TEST(!(x < n));
    BOOST_TEST(!(n < x));
    BOOST_TEST(x <= n);
    BOOST_TEST(n <= x);
    BOOST_TEST(!(x > n));
    BOOST_TEST(!(n > x));
    BOOST_TEST(x >= n);
    BOOST_TEST(n >= x);
}

// x is less than the integer n
template <typename T, typename Integer>
void test_less(const T x, const Integer n)
{
    BOOST_TEST(!(x == n));
    BOOST_TEST(!(n == x));
    BOOST_TEST(x != n);
    BOOST_TEST(n != x);
    BOOST_TEST(x < n);
    BOOST_TEST(!(n < x));
    BOOST_TEST(x <= n);
    BOOST_TEST(!(n <= x));
    BOOST_TEST(!(x > n));
    BOOST_TEST(n > x);
    BOOST_TEST(!(x >= n));
    BOOST_TEST(n >= x);
}

// x is greater than the integer n
template <typename T, typename Integer>
void test_greater(const T x, const Integer n)
{
    BOOST_TEST(!(x == n));
    BOOST_TEST(!(n == x));
    BOOST_TEST(x != n);
    BOOST_TEST(n != x);
    BOOST_TEST(!(x < n));
    BOOST_TEST(n < x);
    BOOST_TEST(!(x <= n));
    BOOST_TEST(n <= x);
    BOOST_TEST(x > n);
    BOOST_TEST(!(n > x));
    BOOST_TEST(x >= n);
    BOOST_TEST(!(n >= x));
}

// The comparison is exact: the integer is not rounded to the precision of T
template <typename T>
void test_exact()
{
    using sig_type = typename T::significand_type;
    constexpr int digits {std::numeric_limits<T>::digits10};
    sig_type max_sig {0U};
    for (int i {0}; i < digits; ++i)
    {
        max_sig = static_cast<sig_type>(max_sig * 10U + 9U);
    }

    // The exponent differs from the integer exponent by more than the precision
    test_equal(T{5U, 8}, INT32_C(500000000));
    test_equal(T{5U, 8}, UINT64_C(500000000));
    test_equal(-T{1U, 18}, INT64_C(-1000000000000000000));

    // The integer has more digits than T
    // A coefficient of all digits needs a 128-bit integer for decimal128_t
    using big_type = std::conditional_t<(digits > 16), boost::int128::uint128_t, std::uint64_t>;
    using signed_big_type = std::conditional_t<(digits > 16), boost::int128::int128_t, std::int64_t>;
    const big_type big {big_type{max_sig} * 1000U};
    test_less(T{max_sig, 3}, big + 1U);
    test_greater(T{max_sig, 3}, big - 1U);
    const auto negative {-static_cast<signed_big_type>(max_sig) * 100};
    test_less(-T{max_sig, 2}, negative + 1);
    test_greater(-T{max_sig, 2}, negative - 1);
    test_less(T{1U, 19}, UINT64_C(10000000000000000001));
    test_equal(T{1U, 19}, UINT64_C(10000000000000000000));

    // A fraction is never equal to an integer
    test_greater(T{max_sig, -1}, max_sig / 10U);
    test_less(T{max_sig, -1}, max_sig / 10U + 1U);
    test_less(T{1U, -digits - 5}, 1);
    test_greater(T{1U, -digits - 5}, 0);

    // The integer scaled to the exponent of T passes 64 bits
    test_less(T{1U, -1}, UINT64_MAX);
    test_greater(-T{1U, -1}, INT64_MIN);

    // The coefficient scaled to the exponent of the integer passes 64 bits
    test_greater(T{2U, 19}, UINT64_MAX);

    // A 128-bit integer
    const auto big128 {boost::int128::uint128_t {UINT64_C(1000000000000000000)} * UINT64_C(1000000000000000000)};
    test_less(T{1U, 36}, big128 + 1U);
    test_greater(T{1U, 36}, big128 - 1U);
}

// The result does not change with the rounding mode
template <typename T>
void test_rounding_modes()
{
    #ifndef BOOST_DECIMAL_NO_CONSTEVAL_DETECTION
    for (const auto mode : {rounding_mode::fe_dec_downward, rounding_mode::fe_dec_upward,
                            rounding_mode::fe_dec_toward_zero, rounding_mode::fe_dec_to_nearest_from_zero})
    {
        fesetround(mode);
        test_exact<T>();
    }
    fesetround(rounding_mode::fe_dec_to_nearest);
    #endif
}

template <typename T>
void test_zero()
{
    test_equal(T{0}, 0);
    test_equal(-T{0}, 0);
    test_equal(-T{0}, 0U);
    test_less(-T{0}, 1);
    test_greater(-T{0}, -1);
    test_less(T{0U, 50}, 1);
    test_equal(T{0U, -50}, 0);
}

template <typename T>
void test_all()
{
    test_exact<T>();
    test_rounding_modes<T>();
    test_zero<T>();
}

int main()
{
    test_all<decimal32_t>();
    test_all<decimal64_t>();
    test_all<decimal128_t>();
    test_all<decimal_fast32_t>();
    test_all<decimal_fast64_t>();
    test_all<decimal_fast128_t>();

    // decimal128_t holds 34 digits, so a 38-digit integer is still exact
    using namespace boost::decimal::literals;
    const boost::int128::uint128_t big38 {UINT64_C(5421010862427522170), UINT64_C(687399551400673280)};
    test_equal(1e38_DL, big38);
    test_greater(1e38_DL, big38 - 1U);
    test_less(1e38_DLF, big38 + 1U);

    return boost::report_errors();
}
