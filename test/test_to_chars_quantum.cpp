// Copyright 2025 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include <boost/decimal.hpp>
#include <boost/core/lightweight_test.hpp>
#include <array>

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
        const auto r {to_chars(buffer, buffer + sizeof(buffer), decimals[i], chars_format::scientific, quantum_preservation::on)};
        BOOST_TEST(r);
        *r.ptr = '\0';

        BOOST_TEST_CSTR_EQ(buffer, strings[i]);
    }

    for (std::size_t i {}; i < decimals.size(); ++i)
    {
        char quantum_buffer[64] {};
        const auto r_quantum {to_chars(quantum_buffer, quantum_buffer + sizeof(quantum_buffer), decimals[i], chars_format::scientific, quantum_preservation::off)};
        BOOST_TEST(r_quantum);
        *r_quantum.ptr = '\0';

        char buffer[64] {};
        const auto r {to_chars(buffer, buffer + sizeof(buffer), decimals[i], chars_format::scientific)};
        BOOST_TEST(r);
        *r.ptr = '\0';

        BOOST_TEST_CSTR_EQ(quantum_buffer, buffer);
        BOOST_TEST_CSTR_EQ(quantum_buffer, strings[0]);
    }
}

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

    return boost::report_errors();
}
