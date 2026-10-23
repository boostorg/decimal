// Copyright 2025 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//
// This file demonstrates cohort preservation with <charconv> using fixed notation.
// cohort_preserving_fixed behaves like cohort_preserving_scientific but uses an implied
// exponent of zero, so it can only represent cohort members whose exponent is not positive.

#include <boost/decimal/decimal32_t.hpp>    // For the type decimal32_t
#include <boost/decimal/charconv.hpp>       // For decimal support for <charconv>
#include <boost/decimal/iostream.hpp>       // For decimal support for <iostream>
#include <iostream>
#include <array>
#include <cstring>
#include <string>

static constexpr std::size_t N {5};

// All the following decimal values compare equal, but since they have different numbers
// of trailing zeros in the significand they are not bitwise equal. Each has an exponent
// that is not positive, so each can be rendered in fixed notation without adding zeros.
constexpr std::array<boost::decimal::decimal32_t, N> decimals = {
    boost::decimal::decimal32_t{300, 0},
    boost::decimal::decimal32_t{3000, -1},
    boost::decimal::decimal32_t{30000, -2},
    boost::decimal::decimal32_t{300000, -3},
    boost::decimal::decimal32_t{3000000, -4},
};

// These strings represent the same values as the constructed ones shown above
constexpr std::array<const char*, N> strings = {
    "300",
    "300.0",
    "300.00",
    "300.000",
    "300.0000",
};

int main()
{
    using boost::decimal::decimal32_t;      // For type decimal32_t
    using boost::decimal::to_chars;         // decimal specific to_chars
    using boost::decimal::from_chars;       // decimal specific from_chars
    using boost::decimal::chars_format;     // chars_format enum with decimal specific option shown here
    using boost::decimal::formatting_limits;// Allows the user to correctly size buffers

    for (std::size_t i = 0; i < N; ++i)
    {
        decimal32_t string_val;
        const auto r_from = from_chars(strings[i], string_val, chars_format::cohort_preserving_fixed);

        if (!r_from)
        {
            // Unexpected failure
            return 1;
        }

        // The parsed value is bitwise identical to the constructed cohort member,
        // not merely equal under operator==
        std::uint32_t string_val_bits;
        std::uint32_t constructed_val_bits;
        std::memcpy(&string_val_bits, &string_val, sizeof(string_val_bits));
        std::memcpy(&constructed_val_bits, &decimals[i], sizeof(constructed_val_bits));

        if (string_val != decimals[i] || string_val_bits != constructed_val_bits)
        {
            std::cout << "Failed to preserve the cohort\n";
            return 1;
        }

        // Roundtrip the value back out with the same format
        char buffer[formatting_limits<decimal32_t>::cohort_preserving_fixed_max_chars] {};
        const auto r_to = to_chars(buffer, buffer + sizeof(buffer), string_val, chars_format::cohort_preserving_fixed);

        if (!r_to)
        {
            // Unexpected failure
            return 1;
        }

        *r_to.ptr = '\0'; // charconv does not null terminate per the C++ specification

        if (std::strcmp(strings[i], buffer) == 0)
        {
            std::cout << "Successful Roundtrip of value: " << buffer << '\n';
        }
        else
        {
            std::cout << "Failed\n";
            return 1;
        }
    }

    // A cohort member with a positive exponent (here 3 x 10^2, which also equals 300) can not
    // be written in fixed notation without appending trailing zeros, so to_chars rejects it.
    const decimal32_t positive_exponent_value {3, 2};
    char buffer[formatting_limits<decimal32_t>::cohort_preserving_fixed_max_chars] {};
    const auto r_reject = to_chars(buffer, buffer + sizeof(buffer), positive_exponent_value, chars_format::cohort_preserving_fixed);

    if (!r_reject)
    {
        std::cout << "\nA value with a positive exponent was correctly rejected\n";
    }
    else
    {
        std::cout << "\nExpected rejection did not occur\n";
        return 1;
    }

    return 0;
}
