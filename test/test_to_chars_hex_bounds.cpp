// Copyright 2026 Shen-Ta Hsieh
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include <boost/decimal.hpp>
#include <boost/core/lightweight_test.hpp>
#include <cstring>
#include <limits>

using namespace boost::decimal;

// A hex result with a precision stays inside the given range, and a range that has
// room for the result gives the same result as a large buffer
template <typename T>
void test()
{
    // Hex digits of all F round up to one more digit
    const T values[] {T{1}, -T{1}, T{123456, -3}, T{0xFFFU}, -T{0xFFFFFU, -3}, T{0x1FFFFFU, 50}, std::numeric_limits<T>::max(), -std::numeric_limits<T>::denorm_min()};

    for (const T value : values)
    {
        for (const int precision : {0, 1, 2, 3, 10})
        {
            char full[128];
            const auto expected {to_chars(full, full + sizeof(full), value, chars_format::hex, precision)};
            BOOST_TEST(expected);
            const auto expected_length {expected.ptr - full};

            for (std::ptrdiff_t length {1}; length <= expected_length; ++length)
            {
                char buffer[128];
                std::memset(buffer, '#', sizeof(buffer));
                const auto r {to_chars(buffer, buffer + length, value, chars_format::hex, precision)};
                BOOST_TEST_EQ(buffer[length], '#');
                BOOST_TEST_EQ(static_cast<bool>(r), length == expected_length);
                if (r)
                {
                    BOOST_TEST_EQ(r.ptr - buffer, expected_length);
                    BOOST_TEST(std::memcmp(buffer, full, static_cast<std::size_t>(expected_length)) == 0);
                }
            }
        }
    }
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
