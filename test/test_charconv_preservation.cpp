// Copyright 2025 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include <boost/decimal.hpp>
#include <boost/core/lightweight_test.hpp>
#include <array>
#include <cstring>
#include <string>

#if defined(__GNUC__) && __GNUC__ >= 8
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wclass-memaccess"
#endif

using namespace boost::decimal;

template <typename ResultsType, typename StringsType>
void test_to_chars_scientific(const ResultsType& decimals, const StringsType& strings)
{
    for (std::size_t i {}; i < decimals.size(); ++i)
    {
        for (std::size_t j {}; j < decimals.size(); ++j)
        {
            BOOST_TEST_EQ(decimals[i], decimals[j]);
        }
    }

    for (std::size_t i {}; i < decimals.size(); ++i)
    {
        char buffer[64] {};
        const auto r {to_chars(buffer, buffer + sizeof(buffer), decimals[i], chars_format::cohort_preserving_scientific)};
        BOOST_TEST(r);
        *r.ptr = '\0';

        BOOST_TEST_CSTR_EQ(buffer, strings[i]);
    }
}

// The cohorts will compare equal regardless so here we check bit-wise equality to be a successful roundtrip
template <typename T, std::size_t N, typename StringsType>
void test_roundtrip(const std::array<T, N>& decimals, const std::array<StringsType, N>& strings)
{
    using bit_type = std::conditional_t<std::is_same<T, decimal32_t>::value, std::uint32_t,
                        std::conditional_t<std::is_same<T, decimal64_t>::value, std::uint64_t, boost::int128::uint128_t>>;

    for (std::size_t i {}; i < decimals.size(); ++i)
    {
        bit_type initial_bits;
        std::memcpy(&initial_bits, &decimals[i], sizeof(initial_bits));

        char buffer[64] {};
        const auto r {to_chars(buffer, buffer + sizeof(buffer), decimals[i], chars_format::cohort_preserving_scientific)};
        BOOST_TEST(r);
        *r.ptr = '\0';
        BOOST_TEST_CSTR_EQ(buffer, strings[i]);

        T return_val;
        const auto return_r {from_chars(buffer, buffer + sizeof(buffer), return_val, chars_format::cohort_preserving_scientific)};
        BOOST_TEST(return_r);

        bit_type return_bits;
        std::memcpy(&return_bits, &return_val, sizeof(return_bits));

        BOOST_TEST_EQ(initial_bits, return_bits);
    }
}

template <typename TargetDecimalType, typename T, std::size_t N>
void test_invalid_values(const std::array<T, N>& strings)
{
    for (std::size_t i {}; i < strings.size(); ++i)
    {
        TargetDecimalType val;
        const auto r {from_chars(strings[i], strings[i] + sizeof(strings[i]), val, chars_format::cohort_preserving_scientific)};
        BOOST_TEST(!r);
    }
}

#ifdef _MSC_VER
#  pragma warning(push)
#  pragma warning(disable: 4127)
#endif

template <typename T, std::size_t N>
void test_invalid_to_chars(const std::array<T, N>& decimals)
{
    BOOST_DECIMAL_IF_CONSTEXPR (detail::is_fast_type_v<T>)
    {
        for (std::size_t i {}; i < decimals.size(); ++i)
        {
            char buffer[64] {};
            const auto r {to_chars(buffer, buffer + sizeof(buffer), decimals[i], chars_format::cohort_preserving_scientific)};
            BOOST_TEST(!r);
        }
    }

    for (std::size_t i {}; i < decimals.size(); ++i)
    {
        char buffer[64] {};
        const auto r {to_chars(buffer, buffer + sizeof(buffer), decimals[i], chars_format::cohort_preserving_scientific, 5)};
        BOOST_TEST(!r);
    }
}

#ifdef _MSC_VER
#  pragma warning(pop)
#endif

// cohort_preserving_fixed mirrors the scientific mode but uses an implied exponent of zero.
// to_chars can only render cohort members with a non-positive exponent; a positive exponent
// would require appending trailing zeros which destroys the cohort.
template <typename ResultsType, typename StringsType>
void test_to_chars_fixed(const ResultsType& decimals, const StringsType& strings)
{
    for (std::size_t i {}; i < decimals.size(); ++i)
    {
        for (std::size_t j {}; j < decimals.size(); ++j)
        {
            BOOST_TEST_EQ(decimals[i], decimals[j]);
        }
    }

    for (std::size_t i {}; i < decimals.size(); ++i)
    {
        char buffer[64] {};
        const auto r {to_chars(buffer, buffer + sizeof(buffer), decimals[i], chars_format::cohort_preserving_fixed)};
        BOOST_TEST(r);
        *r.ptr = '\0';

        BOOST_TEST_CSTR_EQ(buffer, strings[i]);
    }
}

template <typename T, std::size_t N, typename StringsType>
void test_roundtrip_fixed(const std::array<T, N>& decimals, const std::array<StringsType, N>& strings)
{
    using bit_type = std::conditional_t<std::is_same<T, decimal32_t>::value, std::uint32_t,
                        std::conditional_t<std::is_same<T, decimal64_t>::value, std::uint64_t, boost::int128::uint128_t>>;

    for (std::size_t i {}; i < decimals.size(); ++i)
    {
        bit_type initial_bits;
        std::memcpy(&initial_bits, &decimals[i], sizeof(initial_bits));

        char buffer[64] {};
        const auto r {to_chars(buffer, buffer + sizeof(buffer), decimals[i], chars_format::cohort_preserving_fixed)};
        BOOST_TEST(r);
        *r.ptr = '\0';
        BOOST_TEST_CSTR_EQ(buffer, strings[i]);

        T return_val;
        const auto return_r {from_chars(buffer, buffer + sizeof(buffer), return_val, chars_format::cohort_preserving_fixed)};
        BOOST_TEST(return_r);

        bit_type return_bits;
        std::memcpy(&return_bits, &return_val, sizeof(return_bits));

        BOOST_TEST_EQ(initial_bits, return_bits);
    }
}

template <typename TargetDecimalType, std::size_t N>
void test_invalid_values_fixed(const std::array<const char*, N>& strings)
{
    for (std::size_t i {}; i < strings.size(); ++i)
    {
        TargetDecimalType val;
        const auto len {std::strlen(strings[i])};
        const auto r {from_chars(strings[i], strings[i] + len, val, chars_format::cohort_preserving_fixed)};
        BOOST_TEST(!r);
    }
}

// A positive exponent can not be rendered without appending trailing zeros
template <typename T, std::size_t N>
void test_reject_positive_exponent(const std::array<T, N>& decimals)
{
    for (std::size_t i {}; i < decimals.size(); ++i)
    {
        char buffer[64] {};
        const auto r {to_chars(buffer, buffer + sizeof(buffer), decimals[i], chars_format::cohort_preserving_fixed)};
        BOOST_TEST(!r);
    }
}

#ifdef _MSC_VER
#  pragma warning(push)
#  pragma warning(disable: 4127)
#endif

template <typename T, std::size_t N>
void test_invalid_to_chars_fixed(const std::array<T, N>& decimals)
{
    BOOST_DECIMAL_IF_CONSTEXPR (detail::is_fast_type_v<T>)
    {
        for (std::size_t i {}; i < decimals.size(); ++i)
        {
            char buffer[64] {};
            const auto r {to_chars(buffer, buffer + sizeof(buffer), decimals[i], chars_format::cohort_preserving_fixed)};
            BOOST_TEST(!r);
        }
    }

    for (std::size_t i {}; i < decimals.size(); ++i)
    {
        char buffer[64] {};
        const auto r {to_chars(buffer, buffer + sizeof(buffer), decimals[i], chars_format::cohort_preserving_fixed, 5)};
        BOOST_TEST(!r);
    }
}

#ifdef _MSC_VER
#  pragma warning(pop)
#endif

template <typename T>
const std::array<T, 7> decimals = {
    T{3, 2},
    T{30, 1},
    T{300, 0},
    T{3000, -1},
    T{30000, -2},
    T{300000, -3},
    T{3000000, -4},
};

constexpr std::array<const char*, 7> strings = {
    "3e+02",
    "3.0e+02",
    "3.00e+02",
    "3.000e+02",
    "3.0000e+02",
    "3.00000e+02",
    "3.000000e+02",
};

template <typename T>
const std::array<T, 6> decimals_with_exp = {
    T {42, 50},
    T {420, 49},
    T {4200, 48},
    T {42000, 47},
    T {420000, 46},
    T {4200000, 45}
};

constexpr std::array<const char*, 6> decimals_with_exp_strings = {
    "4.2e+51",
    "4.20e+51",
    "4.200e+51",
    "4.2000e+51",
    "4.20000e+51",
    "4.200000e+51",
};

template <typename T>
const std::array<T, 5> negative_values = {
    T {-321, -49},
    T {-3210, -50},
    T {-32100, -51},
    T {-321000, -52},
    T {-3210000, -53}
};

constexpr std::array<const char*, 5> negative_values_strings = {
    "-3.21e-47",
    "-3.210e-47",
    "-3.2100e-47",
    "-3.21000e-47",
    "-3.210000e-47"
};

constexpr std::array<const char*, 3> invalid_decimal32_strings = {
    "+3.2e+20",
    "3.421",
    "9.999999999999999e+05",
};

// Members of the cohort of 300 with a non-positive exponent (renderable in fixed notation)
template <typename T>
const std::array<T, 5> fixed_decimals = {
    T{300, 0},
    T{3000, -1},
    T{30000, -2},
    T{300000, -3},
    T{3000000, -4},
};

constexpr std::array<const char*, 5> fixed_strings = {
    "300",
    "300.0",
    "300.00",
    "300.000",
    "300.0000",
};

// Members of the cohort of 0.003 (exercises the leading-zero rendering path)
template <typename T>
const std::array<T, 4> fixed_fractional = {
    T{3, -3},
    T{30, -4},
    T{300, -5},
    T{3000, -6},
};

constexpr std::array<const char*, 4> fixed_fractional_strings = {
    "0.003",
    "0.0030",
    "0.00300",
    "0.003000",
};

// Negative members of the cohort of -3.21
template <typename T>
const std::array<T, 4> fixed_negative = {
    T{-321, -2},
    T{-3210, -3},
    T{-32100, -4},
    T{-321000, -5},
};

constexpr std::array<const char*, 4> fixed_negative_strings = {
    "-3.21",
    "-3.210",
    "-3.2100",
    "-3.21000",
};

// Cohort members of 300 with a positive exponent; to_chars must reject each
template <typename T>
const std::array<T, 2> positive_exponent_decimals = {
    T{3, 2},
    T{30, 1},
};

constexpr std::array<const char*, 4> fixed_invalid_decimal32_strings = {
    "+3.21",             // A leading '+' is not permitted
    "3.0e+02",           // An explicit exponent is not allowed with an implied exponent of zero
    "9999999.9",         // Eight significant digits exceed the precision of decimal32_t
    // 3e-102 is one step below the decimal32_t exponent floor (q_min == -101); it must be rejected
    // rather than silently rounded to zero
    "0.000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000003",
};

// Build "0.<scale-1 zeros>3", i.e. 3 x 10^-scale rendered in fixed notation
static std::string tiny_fixed(int scale)
{
    std::string s {"0."};
    s.append(static_cast<std::size_t>(scale - 1), '0');
    s.push_back('3');
    return s;
}

// A cohort member whose exponent is below the type's floor can not be stored, so it must be
// rejected rather than silently underflowing to zero. last_representable_scale is the largest
// scale still held exactly (101 for decimal32_t, 398 for decimal64_t, 6176 for decimal128_t).
template <typename T>
void test_fixed_exponent_floor(int last_representable_scale)
{
    const auto ok_str {tiny_fixed(last_representable_scale)};
    T ok_val;
    const auto ok_r {from_chars(ok_str.data(), ok_str.data() + ok_str.size(), ok_val, chars_format::cohort_preserving_fixed)};
    BOOST_TEST(ok_r);

    const auto bad_str {tiny_fixed(last_representable_scale + 1)};
    T bad_val;
    const auto bad_r {from_chars(bad_str.data(), bad_str.data() + bad_str.size(), bad_val, chars_format::cohort_preserving_fixed)};
    BOOST_TEST(!bad_r);
    BOOST_TEST(bad_r.ec == std::errc::value_too_large);
}

// The scientific mode shares the same guard on the upper end: a cohort member whose exponent
// exceeds the type's ceiling would overflow to infinity, so it is rejected instead.
void test_scientific_exponent_ceiling()
{
    const char* ok_str {"3e+90"};
    decimal32_t ok_val;
    const auto ok_r {from_chars(ok_str, ok_str + std::strlen(ok_str), ok_val, chars_format::cohort_preserving_scientific)};
    BOOST_TEST(ok_r);

    const char* bad_str {"3e+91"};
    decimal32_t bad_val;
    const auto bad_r {from_chars(bad_str, bad_str + std::strlen(bad_str), bad_val, chars_format::cohort_preserving_scientific)};
    BOOST_TEST(!bad_r);
    BOOST_TEST(bad_r.ec == std::errc::value_too_large);
}

// A significand of two or more digits is split after the leading digit. A significand of
// exactly 10 is the boundary case, and must render as 1.0e+xx rather than 10e+xx.
template <typename T>
void test_leading_digit_split()
{
    const T values[] {T{9U, 0, false}, T{10U, 0, false}, T{11U, 0, false}, T{10U, -3, false}, T{10U, 3, false}, T{10U, 0, true}};
    const char* const expected[] {"9e+00", "1.0e+01", "1.1e+01", "1.0e-02", "1.0e+04", "-1.0e+01"};

    for (std::size_t i {}; i < sizeof(values) / sizeof(values[0]); ++i)
    {
        char buffer[64] {};
        const auto r {to_chars(buffer, buffer + sizeof(buffer), values[i], chars_format::cohort_preserving_scientific)};
        BOOST_TEST(r);
        *r.ptr = '\0';
        BOOST_TEST_CSTR_EQ(buffer, expected[i]);

        T restored;
        const auto back {from_chars(buffer, r.ptr, restored, chars_format::cohort_preserving_scientific)};
        BOOST_TEST(back);
        BOOST_TEST(std::memcmp(&values[i], &restored, sizeof(T)) == 0);
    }
}

// Cohorts do not apply to non-finite values, but the sign still does
template <typename T>
void test_nonfinite_sign()
{
    const T values[] {std::numeric_limits<T>::infinity(),
                      -std::numeric_limits<T>::infinity(),
                      std::numeric_limits<T>::quiet_NaN(),
                      -std::numeric_limits<T>::quiet_NaN()};

    const char* const expected[] {"inf", "-inf", "nan", "-nan(ind)"};

    for (const auto fmt : {chars_format::cohort_preserving_scientific, chars_format::cohort_preserving_fixed})
    {
        for (std::size_t i {}; i < sizeof(values) / sizeof(values[0]); ++i)
        {
            char buffer[64] {};
            const auto r {to_chars(buffer, buffer + sizeof(buffer), values[i], fmt)};
            BOOST_TEST(r);
            *r.ptr = '\0';
            BOOST_TEST_CSTR_EQ(buffer, expected[i]);
        }
    }
}

int main()
{
    test_to_chars_scientific(decimals<decimal32_t>, strings);
    test_to_chars_scientific(decimals<decimal64_t>, strings);
    test_to_chars_scientific(decimals<decimal128_t>, strings);

    test_to_chars_scientific(decimals_with_exp<decimal32_t>, decimals_with_exp_strings);
    test_to_chars_scientific(decimals_with_exp<decimal64_t>, decimals_with_exp_strings);
    test_to_chars_scientific(decimals_with_exp<decimal128_t>, decimals_with_exp_strings);

    test_to_chars_scientific(negative_values<decimal32_t>, negative_values_strings);
    test_to_chars_scientific(negative_values<decimal64_t>, negative_values_strings);
    test_to_chars_scientific(negative_values<decimal128_t>, negative_values_strings);

    test_roundtrip(decimals<decimal32_t>, strings);
    test_roundtrip(decimals<decimal64_t>, strings);
    test_roundtrip(decimals<decimal128_t>, strings);

    test_roundtrip(decimals_with_exp<decimal32_t>, decimals_with_exp_strings);
    test_roundtrip(decimals_with_exp<decimal64_t>, decimals_with_exp_strings);
    test_roundtrip(decimals_with_exp<decimal128_t>, decimals_with_exp_strings);

    test_roundtrip(negative_values<decimal32_t>, negative_values_strings);
    test_roundtrip(negative_values<decimal64_t>, negative_values_strings);
    test_roundtrip(negative_values<decimal128_t>, negative_values_strings);

    test_invalid_values<decimal32_t>(invalid_decimal32_strings);

    // Every value for fast types are invalid
    test_invalid_values<decimal_fast32_t>(strings);
    test_invalid_values<decimal_fast64_t>(decimals_with_exp_strings);
    test_invalid_values<decimal_fast128_t>(negative_values_strings);
    test_invalid_to_chars(decimals<decimal_fast32_t>);
    test_invalid_to_chars(decimals<decimal_fast64_t>);
    test_invalid_to_chars(decimals<decimal_fast128_t>);

    // Specified precision is not allowed
    test_invalid_to_chars(decimals<decimal32_t>);
    test_invalid_to_chars(decimals<decimal64_t>);
    test_invalid_to_chars(decimals<decimal128_t>);

    // cohort_preserving_fixed
    test_to_chars_fixed(fixed_decimals<decimal32_t>, fixed_strings);
    test_to_chars_fixed(fixed_decimals<decimal64_t>, fixed_strings);
    test_to_chars_fixed(fixed_decimals<decimal128_t>, fixed_strings);

    test_to_chars_fixed(fixed_fractional<decimal32_t>, fixed_fractional_strings);
    test_to_chars_fixed(fixed_fractional<decimal64_t>, fixed_fractional_strings);
    test_to_chars_fixed(fixed_fractional<decimal128_t>, fixed_fractional_strings);

    test_to_chars_fixed(fixed_negative<decimal32_t>, fixed_negative_strings);
    test_to_chars_fixed(fixed_negative<decimal64_t>, fixed_negative_strings);
    test_to_chars_fixed(fixed_negative<decimal128_t>, fixed_negative_strings);

    test_roundtrip_fixed(fixed_decimals<decimal32_t>, fixed_strings);
    test_roundtrip_fixed(fixed_decimals<decimal64_t>, fixed_strings);
    test_roundtrip_fixed(fixed_decimals<decimal128_t>, fixed_strings);

    test_roundtrip_fixed(fixed_fractional<decimal32_t>, fixed_fractional_strings);
    test_roundtrip_fixed(fixed_fractional<decimal64_t>, fixed_fractional_strings);
    test_roundtrip_fixed(fixed_fractional<decimal128_t>, fixed_fractional_strings);

    test_roundtrip_fixed(fixed_negative<decimal32_t>, fixed_negative_strings);
    test_roundtrip_fixed(fixed_negative<decimal64_t>, fixed_negative_strings);
    test_roundtrip_fixed(fixed_negative<decimal128_t>, fixed_negative_strings);

    // A positive exponent can not be preserved in fixed notation
    test_reject_positive_exponent(positive_exponent_decimals<decimal32_t>);
    test_reject_positive_exponent(positive_exponent_decimals<decimal64_t>);
    test_reject_positive_exponent(positive_exponent_decimals<decimal128_t>);

    test_invalid_values_fixed<decimal32_t>(fixed_invalid_decimal32_strings);

    // Every value for fast types is invalid
    test_invalid_values_fixed<decimal_fast32_t>(fixed_strings);
    test_invalid_values_fixed<decimal_fast64_t>(fixed_fractional_strings);
    test_invalid_values_fixed<decimal_fast128_t>(fixed_negative_strings);
    test_invalid_to_chars_fixed(fixed_decimals<decimal_fast32_t>);
    test_invalid_to_chars_fixed(fixed_decimals<decimal_fast64_t>);
    test_invalid_to_chars_fixed(fixed_decimals<decimal_fast128_t>);

    // Specified precision is not allowed
    test_invalid_to_chars_fixed(fixed_decimals<decimal32_t>);
    test_invalid_to_chars_fixed(fixed_decimals<decimal64_t>);
    test_invalid_to_chars_fixed(fixed_decimals<decimal128_t>);

    // An exponent outside the representable range must be rejected, not silently rounded to 0 or inf
    test_fixed_exponent_floor<decimal32_t>(101);
    test_fixed_exponent_floor<decimal64_t>(398);
    test_fixed_exponent_floor<decimal128_t>(6176);
    test_scientific_exponent_ceiling();

    test_leading_digit_split<decimal32_t>();
    test_leading_digit_split<decimal64_t>();
    test_leading_digit_split<decimal128_t>();

    test_nonfinite_sign<decimal32_t>();
    test_nonfinite_sign<decimal64_t>();
    test_nonfinite_sign<decimal128_t>();

    return boost::report_errors();
}
