// Copyright 2024 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include <boost/decimal.hpp>
#include <iostream>
#include <iomanip>

int main()
{
    using namespace boost::decimal;

    constexpr decimal32_t val_1 {100};         // Construction from an integer
    constexpr decimal32_t val_2 {10, 1};       // Construction from a signed integer and exponent
    constexpr decimal32_t val_3 {1U, 2, false}; // Construction from an unsigned integer, exponent, and sign

    std::cout << "Val_1: " << val_1 << '\n'
              << "Val_2: " << val_2 << '\n'
              << "Val_3: " << val_3 << '\n';

    if (val_1 == val_2 && val_2 == val_3 && val_1 == val_3)
    {
        std::cout << "All equal values" << std::endl;
    }

    constexpr decimal64_t val_4 {decimal64_t{2, -1} + decimal64_t{1, -1}};
    constexpr double float_val_4 {0.2 + 0.1};
    const decimal64_t val_5 { float_val_4 };

    std::cout << std::setprecision(17) << "Val_4: " << val_4 << '\n'
              <<  "Float: " << float_val_4 << '\n'
              <<  "Val_5: " << val_5 << '\n';

    if (val_4 == val_5)
    {
        std::cout << "Floats are equal" << std::endl;
    }
    else
    {
        std::cout << "Floats are not equal" << std::endl;
    }

    // Demonstration of the overflow and underflow handling
    constexpr decimal64_t oveflow_value {100, 10000};
    if (isinf(oveflow_value))
    {
        std::cout << "Overflow constructs infinity" << std::endl;
    }

    constexpr decimal64_t underflow_value {100, -10000};
    if (underflow_value == 0)
    {
        std::cout << "Underflow constructs zero" << std::endl;
    }

    const decimal32_t non_finite_from_float {std::numeric_limits<double>::quiet_NaN()};
    if (isnan(non_finite_from_float))
    {
        std::cout << "NaN constructs NaN" << std::endl;
    }

    return 0;
}

