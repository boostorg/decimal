// Copyright 2023 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include <boost/decimal.hpp>
#include <boost/decimal/detail/fast_float/compute_float80_128.hpp>
#include <iostream>
#include <iomanip>
#include <random>
#include <cmath>
#include <climits>

#if defined(__clang__)
#  pragma clang diagnostic push
#  pragma clang diagnostic ignored "-Wold-style-cast"
#  pragma clang diagnostic ignored "-Wundef"
#  pragma clang diagnostic ignored "-Wconversion"
#  pragma clang diagnostic ignored "-Wsign-conversion"
#  pragma clang diagnostic ignored "-Wfloat-equal"
#elif defined(__GNUC__)
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wold-style-cast"
#  pragma GCC diagnostic ignored "-Wundef"
#  pragma GCC diagnostic ignored "-Wconversion"
#  pragma GCC diagnostic ignored "-Wsign-conversion"
#  pragma GCC diagnostic ignored "-Wfloat-equal"
#endif

#include <boost/core/lightweight_test.hpp>

void test_compute_float32()
{
    using boost::decimal::detail::fast_float::compute_float32;

    std::mt19937_64 gen(42);
    std::uniform_int_distribution<std::int64_t> dist (1, 1);

    bool success {};

    // Trivial verifcation
    BOOST_TEST_EQ(compute_float32(1 * dist(gen), 1, false, success), 1e1F);
    BOOST_TEST_EQ(compute_float32(0 * dist(gen), 1, true, success), -1e0F);
    BOOST_TEST_EQ(compute_float32(38 * dist(gen), 1, false, success), 1e38F);

    // out of range
    BOOST_TEST_EQ(compute_float32(310 * dist(gen), 5, false, success), HUGE_VALF);
    BOOST_TEST_EQ(compute_float32(310 * dist(gen), 5, true, success), -HUGE_VALF);
    BOOST_TEST_EQ(compute_float32(1000 * dist(gen), 5, false, success), HUGE_VALF);
    BOOST_TEST_EQ(compute_float32(1000 * dist(gen), 5, true, success), -HUGE_VALF);
    BOOST_TEST_EQ(compute_float32(-325 * dist(gen), 5, false, success), 0.0F);

    // Composite
    BOOST_TEST_EQ(compute_float32(10 * dist(gen), 123456789, false, success), 123456789e10F);
    BOOST_TEST_EQ(compute_float32(20 * dist(gen), UINT64_C(444444444), false, success), 444444444e20F);
}

void test_compute_float64()
{
    using boost::decimal::detail::fast_float::compute_float64;

    std::mt19937_64 gen(42);
    std::uniform_int_distribution<std::uint64_t> dist (1, 1);

    bool success {};

    // Trivial verification
    BOOST_TEST_EQ(compute_float64(1, dist(gen), false, success), 1e1);
    BOOST_TEST_EQ(compute_float64(0, dist(gen), true, success), -1e0);
    BOOST_TEST_EQ(compute_float64(308, dist(gen), false, success), 1e308);

    // out of range
    BOOST_TEST_EQ(compute_float64(310, 5U * dist(gen), false, success), HUGE_VAL);
    BOOST_TEST_EQ(compute_float64(310, 5U * dist(gen), true, success), -HUGE_VAL);
    BOOST_TEST_EQ(compute_float64(1000, 5U * dist(gen), false, success), HUGE_VAL);
    BOOST_TEST_EQ(compute_float64(1000, 5U * dist(gen), true, success), -HUGE_VAL);
    BOOST_TEST_EQ(compute_float64(-325, 5U * dist(gen), false, success), 0.0);
    BOOST_TEST_EQ(compute_float64(static_cast<std::int64_t>(dist(gen)) * 50U, 0, false, success), 0.0);
    BOOST_TEST_EQ(compute_float64(static_cast<std::int64_t>(dist(gen)) * 50U, 0, true, success), 0.0);
    BOOST_TEST_EQ(compute_float64(300, UINT64_MAX, false, success), 0.0 * static_cast<double>(dist(gen)));

    // Composite
    BOOST_TEST_EQ(compute_float64(10 * static_cast<std::int64_t>(dist(gen)), 123456789, false, success), 123456789e10);
    BOOST_TEST_EQ(compute_float64(100 * static_cast<std::int64_t>(dist(gen)), UINT64_C(4444444444444444444), false, success), 4444444444444444444e100);
    BOOST_TEST_EQ(compute_float64(100 * static_cast<std::int64_t>(dist(gen)), std::numeric_limits<std::uint64_t>::max(), false, success), 18446744073709551615e100);
    BOOST_TEST_EQ(compute_float64(100 * static_cast<std::int64_t>(dist(gen)), UINT64_C(10000000000000000000), false, success), 10000000000000000000e100);
}

void test_compute_float80_128()
{
    using boost::decimal::detail::fast_float::compute_float80_128;

    std::mt19937_64 gen(42);
    std::uniform_int_distribution<std::uint64_t> dist (1, 1);

    bool success {};
    
    BOOST_TEST_EQ(compute_float80_128(1, dist(gen), false, success), 1e1L);
    BOOST_TEST_EQ(compute_float80_128(0, dist(gen), true, success), -1e0L);
    BOOST_TEST_EQ(compute_float80_128(-1, dist(gen), false, success), 1e-1L);
}

#ifdef __GNUC__
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif

template <typename T>
void test_generic_binary_to_decimal()
{
    std::mt19937_64 gen(42);
    std::uniform_real_distribution<T> dist (1, 2);

    using namespace boost::decimal::detail::ryu;

    std::uint64_t result[5] {};
    generic_computePow5(56, result);
    BOOST_TEST_EQ(result[0], 0);
    BOOST_TEST_EQ(result[1], 5206161169240293376);
    BOOST_TEST_EQ(result[2], 4575641699882439235);

    BOOST_TEST(floating_point_to_fd128(T(0) * dist(gen)).mantissa == 0U);
    BOOST_TEST(floating_point_to_fd128(std::numeric_limits<T>::infinity() * dist(gen)).exponent == fd128_exceptional_exponent);
    BOOST_TEST(floating_point_to_fd128(std::numeric_limits<T>::quiet_NaN() * dist(gen)).exponent == fd128_exceptional_exponent);
    BOOST_TEST(floating_point_to_fd128(-std::numeric_limits<T>::infinity() * dist(gen)).exponent == fd128_exceptional_exponent);
    BOOST_TEST(floating_point_to_fd128(-std::numeric_limits<T>::quiet_NaN() * dist(gen)).exponent == fd128_exceptional_exponent);

    // https://github.com/boostorg/decimal/issues/242
    BOOST_DECIMAL_IF_CONSTEXPR (std::is_same<T, double>::value)
    {
        // Consider the following integral patterns potentially stored
        // in a given decimal64_t.

        // 0x6A3A25E507BB83D9
        // 0x6A3800D0288A63E9
        // 0x6A380BF1150D7F35
        // 0x6A3B71802C99CB39

        // https://bugs.llvm.org/show_bug.cgi?id=21629
        std::array<std::uint64_t, 4> test_values = {{UINT64_C(0x6A3A25E507BB83D9),
                                                     UINT64_C(0x6A3800D0288A63E9),
                                                     UINT64_C(0x6A380BF1150D7F35),
                                                     UINT64_C(0x6A3B71802C99CB39)}};

        for (const auto bit_values : test_values)
        {
            const std::uint64_t ui = bit_values;

            // And we convert it via direct-memory access to a decimal64_t.

            // In this particular example we assume that this number could
            // be the result of some calculation, initialization or any other
            // valid operations.

            const boost::decimal::decimal64_t dec = *reinterpret_cast<const boost::decimal::decimal64_t*>(&ui);

            // Now let's convert this decimal64_t value to built-in double.
            // ISSUE: This cast seems to result in 0.
            const auto dbl = static_cast<double>(dec);
            const boost::decimal::decimal64_t return_dec(dbl);

            if (!BOOST_TEST(abs(dec - return_dec) <= 1 ))
            {
                // LCOV_EXCL_START
                std::cerr << std::scientific << std::setprecision(std::numeric_limits<boost::decimal::decimal64_t>::digits10)
                          << "       Dec: " << dec
                          << "\n       Dbl: " << dbl
                          << "\nReturn Dec: " << return_dec
                          << "\nDist: " << abs(dec - return_dec) << std::endl;
                // LCOV_EXCL_STOP
            }
        }
    }
}

#ifdef __GNUC__
#  pragma GCC diagnostic pop
#endif

void test_parser()
{
    using namespace boost::decimal::detail;

    std::mt19937_64 gen(42);
    std::uniform_int_distribution<std::uint64_t> dist (1, 2);

    const char* pos_str = "+12345";
    std::uint64_t sig {dist(gen)};
    std::int32_t exp {};
    bool sign {};

    auto res = parser(pos_str, pos_str, sign, sig, exp);
    BOOST_TEST(res.ec == std::errc::invalid_argument);
    res = parser(pos_str, pos_str + std::strlen(pos_str), sign, sig, exp);
    BOOST_TEST(res.ec == std::errc::invalid_argument);

    const char* nan_str = "nan";
    res = parser(nan_str, nan_str + std::strlen(nan_str), sign, sig, exp);
    BOOST_TEST(res.ec == std::errc::not_supported);

    const char* no_trailing = "12345";
    res = parser(no_trailing, no_trailing + std::strlen(no_trailing), sign, sig, exp);
    BOOST_TEST(res.ec == std::errc());

    const char* all_zeros = "0.00000001";
    res = parser(all_zeros, all_zeros + std::strlen(all_zeros), sign, sig, exp);
    BOOST_TEST(res.ec == std::errc());

    const char* big_sig = "123456789012345678901234567890";
    res = parser(big_sig, big_sig + std::strlen(big_sig), sign, sig, exp);
    BOOST_TEST(res.ec == std::errc());

    const char* big_sig_with_frac = "123456789012345678901234567890.123";
    res = parser(big_sig_with_frac, big_sig_with_frac + std::strlen(big_sig_with_frac), sign, sig, exp);
    BOOST_TEST(res.ec == std::errc());

    const char* big_exp = "12345.6789e+1000000";
    res = parser(big_exp, big_exp + std::strlen(big_exp), sign, sig, exp);
    BOOST_TEST(res.ec == std::errc());
    BOOST_TEST_EQ(res.ptr, big_exp + std::strlen(big_exp));
    BOOST_TEST_EQ(exp, 999996); // 1000000 less the 4 fraction digits

    const char* zeros = "0.00000000";
    res = parser(zeros, zeros + std::strlen(zeros), sign, sig, exp);
    BOOST_TEST(res.ec == std::errc());

    const char* fives = "5.555555555555555555555555555555e+05";
    res = parser(fives, fives + std::strlen(fives), sign, sig, exp);
    BOOST_TEST(res.ec == std::errc());

    const char* sixes = "6.6666666666666666666666666666666e+06";
    res = parser(sixes, sixes + std::strlen(sixes), sign, sig, exp);
    BOOST_TEST(res.ec == std::errc());

    const char* sevens = "7.777777777777777777777777777777e+07";
    res = parser(sevens, sevens + std::strlen(sevens), sign, sig, exp);
    BOOST_TEST(res.ec == std::errc());

    const char* eights = "8.888888888888888888888888888888e+08";
    res = parser(eights, eights + std::strlen(eights), sign, sig, exp);
    BOOST_TEST(res.ec == std::errc());

    const char* nines = "9.99999999999999999999999999999999e+09";
    res = parser(nines, nines + std::strlen(nines), sign, sig, exp);
    BOOST_TEST(res.ec == std::errc());

    // A type wider than 64 bits takes the digits in groups of 19 decimal or 15 hex digits
    using boost::int128::uint128_t;
    uint128_t wide_sig {};
    const char* group_and_four = "12345678901234567890123";
    res = parser(group_and_four, group_and_four + std::strlen(group_and_four), sign, wide_sig, exp);
    BOOST_TEST(res.ec == std::errc());
    BOOST_TEST(wide_sig == uint128_t{UINT64_C(1234567890123456789)} * 10000U + 123U);

    const uint128_t thirty_eight_digits {uint128_t{UINT64_C(1234567890123456789)} * UINT64_C(10000000000000000000) + UINT64_C(123456789012345678)};
    const char* two_groups = "12345678901234567890123456789012345678";
    res = parser(two_groups, two_groups + std::strlen(two_groups), sign, wide_sig, exp);
    BOOST_TEST(res.ec == std::errc());
    BOOST_TEST(wide_sig == thirty_eight_digits);

    const char* hex_group = "fffffffffffffffff";
    res = parser(hex_group, hex_group + std::strlen(hex_group), sign, wide_sig, exp, boost::decimal::chars_format::hex);
    BOOST_TEST(res.ec == std::errc());
    BOOST_TEST(wide_sig == (uint128_t{1U} << 68U) - 1U);

    // Only hex digits can be too many for the type
    const char* hex_33 = "100000000000000000000000000000000";
    res = parser(hex_33, hex_33 + std::strlen(hex_33), sign, wide_sig, exp, boost::decimal::chars_format::hex);
    BOOST_TEST(res.ec == std::errc::result_out_of_range);
    res = parser(hex_33, hex_33 + 17, sign, sig, exp, boost::decimal::chars_format::hex);
    BOOST_TEST(res.ec == std::errc::result_out_of_range);
    res = parser(hex_33, hex_33 + 16, sign, sig, exp, boost::decimal::chars_format::hex);
    BOOST_TEST(res.ec == std::errc());
    BOOST_TEST_EQ(sig, UINT64_C(1) << 60U);

    // The error comes at the end of the number, after its exponent
    const char* hex_exp = "1.0000000000000000p12x";
    res = parser(hex_exp, hex_exp + std::strlen(hex_exp), sign, sig, exp, boost::decimal::chars_format::hex);
    BOOST_TEST(res.ec == std::errc::result_out_of_range);
    BOOST_TEST_EQ(res.ptr, hex_exp + 21);

    // Eight digits at a time stop at a character just below '0', just above '9' or outside ASCII
    for (const char stop : {'/', ':', 'e', static_cast<char>(0xB9)})
    {
        for (std::size_t k {1}; k < 16U; ++k)
        {
            char digits[] = "1234567890123456";
            digits[k] = stop;
            std::uint64_t expected {};
            for (std::size_t j {}; j < k; ++j)
            {
                expected = expected * 10U + static_cast<std::uint64_t>(digits[j] - '0');
            }

            res = parser(digits, digits + 16, sign, sig, exp, boost::decimal::chars_format::fixed);
            BOOST_TEST(res.ec == std::errc());
            BOOST_TEST_EQ(res.ptr, digits + k);
            BOOST_TEST_EQ(sig, expected);

            res = parser(digits, digits + 16, sign, wide_sig, exp, boost::decimal::chars_format::fixed);
            BOOST_TEST(res.ec == std::errc());
            BOOST_TEST_EQ(res.ptr, digits + k);
            BOOST_TEST(wide_sig == uint128_t{expected});
        }
    }

    const char* fraction_groups = "1.2345678901234567890123456789012345678";
    res = parser(fraction_groups, fraction_groups + std::strlen(fraction_groups), sign, wide_sig, exp);
    BOOST_TEST(res.ec == std::errc());
    BOOST_TEST(wide_sig == thirty_eight_digits);
    BOOST_TEST_EQ(exp, -37);
}

// Each byte value at each place of eight digits gives the same result as one digit at a time.
// The accumulator starts empty, or with digits just below or at the end of a 64-bit group
template <typename Unsigned_Integer>
void test_push_eight(const std::size_t start)
{
    using namespace boost::decimal::detail;

    for (std::size_t k {}; k < 8U; ++k)
    {
        for (int b {}; b < 256; ++b)
        {
            char digits[] = "12345678";
            digits[k] = static_cast<char>(b);

            significand_accumulator<Unsigned_Integer> eight {};
            significand_accumulator<Unsigned_Integer> one {};
            for (std::size_t j {}; j < start; ++j)
            {
                eight.push('9', 10);
                one.push('9', 10);
            }

            const bool accepted {eight.push_eight(digits)};
            BOOST_TEST_EQ(accepted, is_integer_char(digits[k]));
            for (std::size_t j {}; j < 8U && accepted; ++j)
            {
                one.push(digits[j], 10);
            }

            Unsigned_Integer eight_value {};
            Unsigned_Integer one_value {};
            BOOST_TEST(eight.get(eight_value, start + 8U, 10) == std::errc());
            BOOST_TEST(one.get(one_value, start + 8U, 10) == std::errc());
            BOOST_TEST(eight_value == one_value);
        }
    }
}

void test_hex_integer()
{
    std::uint64_t significand {};
    std::int64_t  exponent {};
    bool sign {};

    const char* val1 = "2a";
    auto r1 = boost::decimal::detail::parser(val1, val1 + std::strlen(val1), sign, significand, exponent, boost::decimal::chars_format::hex);
    BOOST_TEST(r1.ec == std::errc());
    BOOST_TEST_EQ(sign, false);
    BOOST_TEST_EQ(significand, UINT64_C(42));
    BOOST_TEST_EQ(exponent, 0);

    significand = 0;
    exponent = 0;
    sign = false;

    const char* val2 = "-1a3b5c7d9";
    auto r2 = boost::decimal::detail::parser(val2, val2 + std::strlen(val2), sign, significand, exponent, boost::decimal::chars_format::hex);
    BOOST_TEST(r2.ec == std::errc());
    BOOST_TEST_EQ(sign, true);
    BOOST_TEST_EQ(exponent, 0);
    BOOST_TEST_EQ(significand, UINT64_C(7041566681));

    auto r3 = boost::decimal::detail::parser(val2, val2 + std::strlen(val2), sign, significand, exponent, boost::decimal::chars_format::scientific);
    BOOST_TEST(r3.ec == std::errc::invalid_argument);
}

void test_hex_scientific()
{
    std::uint64_t significand {};
    std::int64_t  exponent {};
    bool sign {};

    const char* val1 = "2ap+5";
    auto r1 = boost::decimal::detail::parser(val1, val1 + std::strlen(val1), sign, significand, exponent, boost::decimal::chars_format::hex);
    BOOST_TEST(r1.ec == std::errc());
    BOOST_TEST_EQ(sign, false);
    BOOST_TEST_EQ(significand, UINT64_C(42));
    BOOST_TEST_EQ(exponent, 5);

    significand = 0;
    exponent = 0;
    sign = false;

    const char* val2 = "-1.3a2bp-10";
    auto r2 = boost::decimal::detail::parser(val2, val2 + std::strlen(val2), sign, significand, exponent, boost::decimal::chars_format::hex);
    BOOST_TEST(r2.ec == std::errc());
    BOOST_TEST_EQ(sign, true);
    BOOST_TEST_EQ(exponent, -14);
    BOOST_TEST_EQ(significand, UINT64_C(80427));

    auto r3 = boost::decimal::detail::parser(val2, val2 + std::strlen(val2), sign, significand, exponent, boost::decimal::chars_format::scientific);
    BOOST_TEST(r3.ec == std::errc::invalid_argument);

    const char* val4 = "-1.3A2BP-10";
    auto r4 = boost::decimal::detail::parser(val4, val4 + std::strlen(val4), sign, significand, exponent, boost::decimal::chars_format::hex);
    BOOST_TEST(r4.ec == std::errc());
    BOOST_TEST_EQ(sign, true);
    BOOST_TEST_EQ(exponent, -14);
    BOOST_TEST_EQ(significand, UINT64_C(80427));
}

void test_from_chars()
{
    using namespace boost::decimal::detail;

    std::mt19937_64 gen(42);
    std::uniform_int_distribution<std::uint64_t> dist (1, 2);

    const char* pos_str = "+12345";
    std::uint64_t sig {dist(gen)};

    auto res = from_chars_integer_impl<std::uint64_t, std::uint64_t>(pos_str, pos_str, sig, 10);
    BOOST_TEST(res.ec == std::errc::invalid_argument);
    res = from_chars_integer_impl<std::uint64_t, std::uint64_t>(pos_str, pos_str + std::strlen(pos_str), sig, 10);
    BOOST_TEST(res.ec == std::errc::invalid_argument);

    const char* neg_str = "-12345";
    res = from_chars_integer_impl<std::uint64_t, std::uint64_t>(neg_str, neg_str + std::strlen(neg_str), sig, 10);
    BOOST_TEST(res.ec == std::errc::invalid_argument);

    auto signed_sig {static_cast<std::int64_t>(dist(gen))};
    res = from_chars_integer_impl<std::int64_t, std::uint64_t>(neg_str, neg_str + 1, signed_sig, 10);
    BOOST_TEST(res.ec == std::errc::invalid_argument);

    res = from_chars_integer_impl<std::int64_t, std::uint64_t>(neg_str, neg_str + std::strlen(neg_str), signed_sig, 10);
    BOOST_TEST(res.ec == std::errc());

    const char* big_sig = "-123456789123456789123456789123456789";
    res = from_chars_integer_impl<std::int64_t, std::uint64_t>(big_sig, big_sig + std::strlen(big_sig), signed_sig, 10);
    BOOST_TEST(res.ec == std::errc::result_out_of_range);
}

// A negative zero or infinity stays negative in the binary type
template <typename Dec, typename Float>
void test_signed_zero_and_infinity()
{
    const Dec zero {0};
    const Dec inf {std::numeric_limits<Dec>::infinity()};

    BOOST_TEST(!std::signbit(static_cast<Float>(zero)));
    BOOST_TEST(std::signbit(static_cast<Float>(-zero)));
    BOOST_TEST_EQ(static_cast<Float>(inf), std::numeric_limits<Float>::infinity());
    BOOST_TEST_EQ(static_cast<Float>(-inf), -std::numeric_limits<Float>::infinity());
}

template <typename Float>
void test_signed_zero_and_infinity_to()
{
    using namespace boost::decimal;

    test_signed_zero_and_infinity<decimal32_t, Float>();
    test_signed_zero_and_infinity<decimal64_t, Float>();
    test_signed_zero_and_infinity<decimal128_t, Float>();
    test_signed_zero_and_infinity<decimal_fast32_t, Float>();
    test_signed_zero_and_infinity<decimal_fast64_t, Float>();
    test_signed_zero_and_infinity<decimal_fast128_t, Float>();
}

int main()
{
    test_compute_float32();
    test_compute_float64();
    test_compute_float80_128();
    test_generic_binary_to_decimal<float>();
    test_generic_binary_to_decimal<double>();

    #ifndef BOOST_DECIMAL_UNSUPPORTED_LONG_DOUBLE
    test_generic_binary_to_decimal<long double>();
    #endif

    test_parser();
    test_push_eight<std::uint64_t>(0U);
    test_push_eight<std::uint64_t>(11U);
    test_push_eight<boost::int128::uint128_t>(0U);
    test_push_eight<boost::int128::uint128_t>(11U);
    test_push_eight<boost::int128::uint128_t>(12U);
    test_hex_integer();
    test_hex_scientific();

    test_from_chars();

    test_signed_zero_and_infinity_to<float>();
    test_signed_zero_and_infinity_to<double>();
    #ifndef BOOST_DECIMAL_UNSUPPORTED_LONG_DOUBLE
    test_signed_zero_and_infinity_to<long double>();
    #endif

    return boost::report_errors();
}
