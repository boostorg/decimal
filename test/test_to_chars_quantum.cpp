// Copyright 2025 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include <boost/decimal.hpp>
#include <boost/core/lightweight_test.hpp>
#include <array>

using namespace boost::decimal;

template <typename T>
void test_to_chars_scientific()
{
    const std::array<T, 7> decimals = {
        T{3, 2},
        T{30, 1},
        T{300, 0},
        T{3000, -1},
        T{30000, -2},
        T{300000, -3},
        T{3000000, -4},
    };

    const std::array<const char*, 7> strings = {
        "3e+02",
        "3.0e+02",
        "3.00e+02",
        "3.000e+02",
        "3.0000e+02",
        "3.00000e+02",
        "3.000000e+02",
    };

    for (std::size_t i {}; i < decimals.size(); ++i)
    {
        char buffer[64] {};
        const auto r {to_chars(buffer, buffer + sizeof(buffer), decimals[i], chars_format::scientific, quantum_preservation::on)};
        BOOST_TEST(r);
        BOOST_TEST_CSTR_EQ(buffer, strings[i]);
    }

    for (std::size_t i {}; i < decimals.size(); ++i)
    {
        char quantum_buffer[64] {};
        const auto r_quantum {to_chars(quantum_buffer, quantum_buffer + sizeof(quantum_buffer), decimals[i], chars_format::scientific, quantum_preservation::off)};
        BOOST_TEST(r_quantum);

        char buffer[64] {};
        const auto r {to_chars(buffer, buffer + sizeof(buffer), decimals[i], chars_format::scientific)};
        BOOST_TEST(r);

        BOOST_TEST_CSTR_EQ(quantum_buffer, buffer);
    }
}

int main()
{
    test_to_chars_scientific<decimal32_t>();
    test_to_chars_scientific<decimal64_t>();
    test_to_chars_scientific<decimal128_t>();

    return boost::report_errors();
}
