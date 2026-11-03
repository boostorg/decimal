// Copyright 2026 Shen-Ta Hsieh
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include <boost/decimal.hpp>
#include <boost/core/lightweight_test.hpp>
#include <algorithm>
#include <cstring>
#include <initializer_list>
#include <iostream>
#include <limits>
#include <string>
#include <utility>

#ifdef BOOST_DECIMAL_HAS_FORMAT_SUPPORT
#include <format>
#endif

using namespace boost::decimal;
using boost::int128::uint128_t;

constexpr chars_format formats[] {chars_format::general, chars_format::fixed, chars_format::scientific, chars_format::hex};
// -1 is no precision. The others are near the digits of each type (7, 16, 34) and of 2^64 (19, 20)
constexpr int precisions[] {-1, 0, 1, 2, 5, 6, 7, 15, 16, 17, 19, 20, 21, 33, 34, 40};

// With a precision, the result fits a range of its own length, and a range one byte shorter fails.
// Neither one writes past its end
template <typename T>
std::string print(const T value, const chars_format fmt, const int precision)
{
    char full[256];
    if (precision < 0)
    {
        const auto r {to_chars(full, full + sizeof(full), value, fmt)};
        BOOST_TEST(r);
        return std::string(full, r.ptr);
    }
    const auto r {to_chars(full, full + sizeof(full), value, fmt, precision)};
    BOOST_TEST(r);
    const auto length {r.ptr - full};

    for (const auto size : {length, length - 1})
    {
        char buffer[256];
        std::memset(buffer, '#', sizeof(buffer));
        const auto s {to_chars(buffer, buffer + size, value, fmt, precision)};
        BOOST_TEST_EQ(buffer[size], '#');
        BOOST_TEST_EQ(static_cast<bool>(s), size == length);
        if (s)
        {
            BOOST_TEST_EQ(s.ptr - buffer, length);
            BOOST_TEST_EQ(std::string(buffer, s.ptr), std::string(full, r.ptr));
        }
    }

    return std::string(full, r.ptr);
}

// coefficient * 10^exponent with each coefficient length that the type holds, of each sign. They are equal
// values with the same text in each format, also in the fast type, which keeps one coefficient length
template <typename T, typename Fast, typename Coefficient>
void test_cohorts(const Coefficient coefficient, const int exponent)
{
    for (const bool negative : {false, true})
    {
        const Fast fast {coefficient, exponent, negative};
        Coefficient c {coefficient};
        int e {exponent};
        T values[std::numeric_limits<T>::digits] {};
        int count {};
        while (detail::num_digits(c) <= std::numeric_limits<T>::digits)
        {
            values[count++] = T{c, e, negative};
            c *= 10U;
            --e;
        }

        for (const auto fmt : formats)
        {
            for (const int precision : precisions)
            {
                const auto expected {print(fast, fmt, precision)};
                for (int i {}; i < count; ++i)
                {
                    if (!BOOST_TEST_EQ(print(values[i], fmt, precision), expected))
                    {
                        std::cerr << (negative ? "-" : "") << "coefficient " << coefficient << "e" << exponent << " zeros "
                                  << i << " format " << static_cast<int>(fmt) << " precision " << precision << '\n';
                    }
                }
            }
        }
    }
}

template <typename T, typename Fast, typename Coefficient>
void test_type(const std::initializer_list<Coefficient> coefficients)
{
    for (const auto coefficient : coefficients)
    {
        for (const int exponent : {-45, -30, -21, -6, -1, 0, 2, 25})
        {
            test_cohorts<T, Fast>(coefficient, exponent);
        }
    }
}

void test_all_types()
{
    test_type<decimal32_t, decimal_fast32_t, std::uint32_t>({1U, 5U, 15U, 25U, 95U, 999U, 9999995U, 1234567U});
    test_type<decimal64_t, decimal_fast64_t, std::uint64_t>({1U, 5U, 15U, 25U, 95U, 999999999999995U, 999999999999999U,
                                                            1000000000000000U, 9999999999999999U, 1234567890123456U});

    // Coefficients below, at and above 2^64, ties above 2^64, and 64-bit coefficients that become wide
    // with the zeros
    constexpr auto max64 {(std::numeric_limits<std::uint64_t>::max)()};
    test_type<decimal128_t, decimal_fast128_t, uint128_t>({uint128_t{1U}, uint128_t{25U}, uint128_t{95U},
                                                         uint128_t{UINT64_C(9999999999999999999)}, uint128_t{UINT64_C(10000000000000000000)},
                                                         uint128_t{max64 - 1U}, uint128_t{max64}, uint128_t{max64} + 1U, uint128_t{max64} + 2U,
                                                         uint128_t{UINT64_C(2500000000000000000)} * 10U + 5U, uint128_t{UINT64_C(5000000000000000000)} * 10U + 5U,
                                                         uint128_t{UINT64_C(1234567890123456789)} * UINT64_C(1000000000000000) + UINT64_C(123456789012345)});
}

// Equal values give equal text in each rounding mode
void test_rounding_modes()
{
    #if !defined(BOOST_DECIMAL_NO_CONSTEVAL_DETECTION) && !defined(BOOST_DECIMAL_REDUCE_TEST_DEPTH) && !defined(BOOST_DECIMAL_QEMU_TEST)
    for (const auto mode : {rounding_mode::fe_dec_downward, rounding_mode::fe_dec_to_nearest_from_zero,
                            rounding_mode::fe_dec_toward_zero, rounding_mode::fe_dec_upward})
    {
        fesetround(mode);
        test_all_types();
    }
    fesetround(rounding_mode::fe_dec_default);
    #endif
}

// coefficient * 10^exponent with a precision, and its text in each rounding mode, calculated by hand:
// to nearest, to nearest from zero, toward zero, upward and downward
struct rounded_text
{
    std::uint32_t coefficient;
    int exponent;
    chars_format fmt;
    int precision;
    const char* text[5];
};

constexpr rounded_text rounded_texts[] {
    // Halfway with an even digit kept, just above halfway, and a carry into a new digit
    {9985U, -3, chars_format::general, 3, {"9.98", "9.99", "9.98", "9.99", "9.98"}},
    {998501U, -5, chars_format::general, 3, {"9.99", "9.99", "9.98", "9.99", "9.98"}},
    {99995U, -4, chars_format::general, 3, {"10", "10", "9.99", "10", "9.99"}},
    // A carry to 10^-4 and to 10^P changes the general notation
    {99995U, -9, chars_format::general, 4, {"0.0001", "0.0001", "9.999e-05", "0.0001", "9.999e-05"}},
    {9999995U, -1, chars_format::general, 6, {"1e+06", "1e+06", "999999", "1e+06", "999999"}},
    {25U, -1, chars_format::fixed, 0, {"2", "3", "2", "3", "2"}},
    {996U, -2, chars_format::fixed, 1, {"10.0", "10.0", "9.9", "10.0", "9.9"}},
    // Rounding to zero
    {4U, -3, chars_format::fixed, 2, {"0.00", "0.00", "0.00", "0.01", "0.00"}},
    {5U, -3, chars_format::fixed, 2, {"0.00", "0.01", "0.00", "0.01", "0.00"}},
    {125U, 8, chars_format::scientific, 1, {"1.2e+10", "1.3e+10", "1.2e+10", "1.3e+10", "1.2e+10"}},
    {999996U, -1, chars_format::scientific, 4, {"1.0000e+05", "1.0000e+05", "9.9999e+04", "1.0000e+05", "9.9999e+04"}},
};

// A negative value rounds upward to the negative of its magnitude rounded downward, and the reverse
template <typename T>
void test_rounded_texts()
{
    #ifndef BOOST_DECIMAL_NO_CONSTEVAL_DETECTION
    constexpr rounding_mode modes[] {rounding_mode::fe_dec_to_nearest, rounding_mode::fe_dec_to_nearest_from_zero,
                                     rounding_mode::fe_dec_toward_zero, rounding_mode::fe_dec_upward, rounding_mode::fe_dec_downward};
    #else
    constexpr rounding_mode modes[] {rounding_mode::fe_dec_to_nearest};
    #endif
    for (const auto& row : rounded_texts)
    {
        for (std::size_t m {}; m < sizeof(modes) / sizeof(modes[0]); ++m)
        {
            fesetround(modes[m]);
            const T value {row.coefficient, row.exponent};
            BOOST_TEST_EQ(print(value, row.fmt, row.precision), row.text[m]);
            const auto mirror {m == 3U ? 4U : m == 4U ? 3U : m};
            BOOST_TEST_EQ(print(-value, row.fmt, row.precision), std::string("-") + row.text[mirror]);
        }
    }
    fesetround(rounding_mode::fe_dec_default);
}

// The cohort preserving formats fit a range of their own length, and fail in a shorter range without
// writing past its end
template <typename T>
void test_cohort_lengths(const T value)
{
    for (const auto fmt : {chars_format::cohort_preserving_scientific, chars_format::cohort_preserving_fixed})
    {
        char full[64];
        const auto f {to_chars(full, full + sizeof(full), value, fmt)};
        BOOST_TEST(f);
        const auto length {f.ptr - full};
        for (std::ptrdiff_t size {1}; size <= length; ++size)
        {
            char buffer[64];
            std::memset(buffer, '#', sizeof(buffer));
            const auto r {to_chars(buffer, buffer + size, value, fmt)};
            BOOST_TEST_EQ(buffer[size], '#');
            BOOST_TEST_EQ(static_cast<bool>(r), size == length);
        }
    }
}

void test_cohort_formats()
{
    test_cohort_lengths(decimal32_t{1234567, -3});
    test_cohort_lengths(decimal32_t{5, -2});
    test_cohort_lengths(decimal32_t{-5, -20});
    test_cohort_lengths(decimal32_t{0, -3});
    test_cohort_lengths(decimal64_t{12, -30});
    test_cohort_lengths(decimal128_t{-1, -20});
}

// Some results with a coefficient near 2^64
void test_values()
{
    const decimal128_t max64 {uint128_t{(std::numeric_limits<std::uint64_t>::max)()}, -19};
    BOOST_TEST_EQ(print(max64, chars_format::general, 20), "1.8446744073709551615");
    BOOST_TEST_EQ(print(max64, chars_format::scientific, 18), "1.844674407370955162e+00");
    BOOST_TEST_EQ(print(-max64, chars_format::fixed, 3), "-1.845");
    const decimal128_t above {uint128_t{(std::numeric_limits<std::uint64_t>::max)()} + 1U, 0};
    BOOST_TEST_EQ(print(above, chars_format::general, 19), "1.844674407370955162e+19");
}

// A non-canonical coefficient with more digits than the type gives its stored digits, also with a precision
void test_non_canonical()
{
    const auto d32 {from_bits(std::uint32_t{UINT32_C(0x60000000) | (UINT32_C(101) << 21) | UINT32_C(0x1FFFFF)})};
    BOOST_TEST_EQ(print(d32, chars_format::general, 8), "10485759");
    BOOST_TEST_EQ(print(d32, chars_format::fixed, 0), "10485759");
    BOOST_TEST_EQ(print(d32, chars_format::scientific, 7), "1.0485759e+07");
    BOOST_TEST_EQ(print(d32, chars_format::hex, 5), "9.fffffp+05");

    const auto d64 {from_bits(std::uint64_t{(UINT64_C(3) << 61) | (UINT64_C(398) << 51) | ((UINT64_C(1) << 51) - 1U)})};
    BOOST_TEST_EQ(print(d64, chars_format::general, 17), "11258999068426239");
    BOOST_TEST_EQ(print(d64, chars_format::fixed, 0), "11258999068426239");
    BOOST_TEST_EQ(print(d64, chars_format::scientific, 16), "1.1258999068426239e+16");
    BOOST_TEST_EQ(print(d64, chars_format::hex, 13), "2.7ffffffffffffp+13");

    const auto d128 {from_bits((uint128_t{6176U} << 113) | ((uint128_t{1U} << 113) - 1U))};
    BOOST_TEST_EQ(print(d128, chars_format::general, 35), "10384593717069655257060992658440191");
    BOOST_TEST_EQ(print(d128, chars_format::fixed, 0), "10384593717069655257060992658440191");
    BOOST_TEST_EQ(print(d128, chars_format::scientific, 34), "1.0384593717069655257060992658440191e+34");
    BOOST_TEST_EQ(print(d128, chars_format::hex, 28), "1.ffffffffffffffffffffffffffffp+28");
}

// The hex text with a precision is the text without one, with its hex digits rounded half up. A carry
// into a new leading digit adds one to the exponent
std::string round_hex(const std::string& text, const int precision)
{
    const auto p {text.find('p')};
    const auto sign {text[0] == '-' ? std::string("-") : std::string()};
    std::string digits {text.substr(sign.size(), p - sign.size())};
    digits.erase(std::remove(digits.begin(), digits.end(), '.'), digits.end());
    int exp {std::stoi(text.substr(p + 1))};
    const auto keep {static_cast<std::size_t>(precision) + 1U};
    if (digits.size() <= keep)
    {
        digits.append(keep - digits.size(), '0');
    }
    else
    {
        const bool up {digits[keep] >= '8'};
        digits.resize(keep);
        auto i {keep};
        while (up && i > 0U && digits[i - 1U] == 'f')
        {
            digits[--i] = '0';
        }
        if (up && i == 0U)
        {
            digits[0] = '1';
            ++exp;
        }
        else if (up)
        {
            digits[i - 1U] = digits[i - 1U] == '9' ? 'a' : static_cast<char>(digits[i - 1U] + 1);
        }
    }

    std::string result {sign};
    result += digits[0];
    if (precision > 0)
    {
        result += '.';
        result.append(digits, 1U, std::string::npos);
    }
    result += 'p';
    result += exp < 0 ? '-' : '+';
    const auto exp_text {std::to_string(exp < 0 ? -exp : exp)};
    if (exp_text.size() < 2U)
    {
        result += '0';
    }
    return result + exp_text;
}

template <typename T, typename Coefficient>
void test_hex_type(std::initializer_list<Coefficient> coefficients)
{
    for (const auto coefficient : coefficients)
    {
        for (const auto value : {T{coefficient, 0}, T{coefficient, -3, true}})
        {
            const auto text {print(value, chars_format::hex, -1)};
            for (const int precision : precisions)
            {
                if (precision >= 0)
                {
                    BOOST_TEST_EQ(print(value, chars_format::hex, precision), round_hex(text, precision));
                }
            }
        }
    }
}

// Hex digits that round up, also with a carry into a new leading digit
void test_hex_rounding()
{
    test_hex_type<decimal32_t>({1U, 15U, 31U, 255U, 4095U, 1048575U, 9999999U, 1234567U});
    test_hex_type<decimal_fast32_t>({1U, 15U, 31U, 255U, 4095U, 1048575U, 9999999U, 1234567U});
    test_hex_type<decimal64_t>({UINT64_C(31), UINT64_C(4095), UINT64_C(4503599627370495), UINT64_C(9999999999999999), UINT64_C(1234567890123456)});
    test_hex_type<decimal_fast64_t>({UINT64_C(31), UINT64_C(4095), UINT64_C(4503599627370495), UINT64_C(9999999999999999), UINT64_C(1234567890123456)});
    const auto max64 {uint128_t{(std::numeric_limits<std::uint64_t>::max)()}};
    test_hex_type<decimal128_t>({uint128_t{31U}, max64, max64 + 1U, (max64 << 40) + max64});
    test_hex_type<decimal_fast128_t>({uint128_t{31U}, max64, max64 + 1U, (max64 << 40) + max64});

    // A precision longer than the range fails before it is used in any sum
    char buffer[64];
    const auto r {to_chars(buffer, buffer + sizeof(buffer), decimal64_t{123456789, -3}, chars_format::hex, (std::numeric_limits<int>::max)())};
    BOOST_TEST(r.ec == std::errc::value_too_large);
}

// A coefficient with zeros at its end gives the digits in front of them, also when they fit 64 bits
void test_trailing_zeros()
{
    for (const auto digits : {uint128_t{1U}, uint128_t{7U}, uint128_t{UINT64_C(123456789)},
                              uint128_t{(std::numeric_limits<std::uint64_t>::max)()}, uint128_t{UINT64_C(9876543210987654321)} * 1000U + 1U})
    {
        auto value {digits};
        for (std::size_t zeros {}; ; ++zeros)
        {
            const auto r {detail::remove_trailing_zeros(value)};
            BOOST_TEST_EQ(r.trimmed_number, digits);
            BOOST_TEST_EQ(r.number_of_removed_zeros, zeros);
            if (value > (std::numeric_limits<uint128_t>::max)() / 10U)
            {
                break;
            }
            value *= 10U;
        }
    }
}

// The 128-bit integer writer with a sign
void test_signed_integer()
{
    using boost::int128::int128_t;
    const std::pair<int128_t, const char*> values[] {{int128_t{-1}, "-1"}, {int128_t{-123}, "-123"},
        {-int128_t{(std::numeric_limits<std::int64_t>::max)()}, "-9223372036854775807"},
        {-int128_t{uint128_t{(std::numeric_limits<std::uint64_t>::max)()}}, "-18446744073709551615"},
        {-int128_t{uint128_t{(std::numeric_limits<std::uint64_t>::max)()} + 1U}, "-18446744073709551616"},
        {(std::numeric_limits<int128_t>::min)(), "-170141183460469231731687303715884105728"},
        {int128_t{uint128_t{(std::numeric_limits<std::uint64_t>::max)()}}, "18446744073709551615"}};

    for (const auto& value : values)
    {
        const auto length {static_cast<std::ptrdiff_t>(std::strlen(value.second))};
        for (std::ptrdiff_t size {}; size <= length; ++size)
        {
            char buffer[64];
            std::memset(buffer, '#', sizeof(buffer));
            const auto r {detail::to_chars_integer_impl(buffer, buffer + size, value.first)};
            BOOST_TEST_EQ(buffer[size], '#');
            BOOST_TEST_EQ(static_cast<bool>(r), size == length);
            if (r)
            {
                BOOST_TEST_EQ(r.ptr - buffer, length);
                BOOST_TEST_EQ(std::string(buffer, r.ptr), value.second);
            }
        }
    }
}

// std::format prints into a local buffer of 128 bytes first. Results around that length, also with a sign,
// are the same as from to_chars
void test_format_lengths()
{
    #ifdef BOOST_DECIMAL_HAS_FORMAT_SUPPORT
    const decimal128_t value {uint128_t{(std::numeric_limits<std::uint64_t>::max)()}, -10};
    char buffer[512];
    for (int precision {100}; precision < 140; ++precision)
    {
        for (const auto v : {value, -value})
        {
            const auto r {to_chars(buffer, buffer + sizeof(buffer), v, chars_format::fixed, precision)};
            BOOST_TEST(r);
            const std::string text(buffer, r.ptr);
            const auto digits {std::to_string(precision)};
            BOOST_TEST_EQ(std::vformat("{:." + digits + "f}", std::make_format_args(v)), text);
            BOOST_TEST_EQ(std::vformat("{:+." + digits + "f}", std::make_format_args(v)), v < 0 ? text : '+' + text);
        }
    }
    // 10 integer digits, the point and 400 fraction digits
    BOOST_TEST_EQ(std::format("{:.400f}", value).size(), 10U + 1U + 400U);
    #endif
}

int main()
{
    test_all_types();
    test_rounding_modes();
    test_rounded_texts<decimal32_t>();
    test_rounded_texts<decimal64_t>();
    test_rounded_texts<decimal128_t>();
    test_rounded_texts<decimal_fast32_t>();
    test_rounded_texts<decimal_fast64_t>();
    test_rounded_texts<decimal_fast128_t>();
    test_values();
    test_non_canonical();
    test_cohort_formats();
    test_hex_rounding();
    test_trailing_zeros();
    test_signed_integer();
    test_format_lengths();

    return boost::report_errors();
}
