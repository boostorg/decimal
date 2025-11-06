// Copyright 2024 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//
// This example shows how we are able to use adl with Boost.Decimal to allow a template function
// to use both built-in binary floating point types, as well as Boost.Decimal types

#include "test.hpp"
#include <boost/decimal.hpp>
#include <cmath>

template <typename T>
void sin_identity(T val)
{
    // ADL allows builtin and decimal types to both be used
    // Boost.Decimal is not allowed to overload std::sin so it must be provided in its own namespace
    // You must also include using std::sin to ensure that it is found for the float, double, and long double cases.
    // It is preferred to have using statements for the functions you intend to use instead of using namespace XXX.
    using std::sin;
    using boost::decimal::sin;

    // sin(x) = -sin(-x)
    // The call here MUST be unqualified, or you will get compiler errors
    // For example calling std::sin here would not allow any of the decimal types to be used
    BOOST_DECIMAL_TEST_EQ(sin(val), -sin(-val));
}

int main()
{
    // Because of the two using statements in the above function we can now call it with built-in floating point,
    // or our decimal types as show below
        
    sin_identity(-0.5F);
    sin_identity(-0.5);
    sin_identity(-0.5L);

    sin_identity(boost::decimal::decimal32_t{"-0.5"});
    sin_identity(boost::decimal::decimal64_t{"-0.5"});
    sin_identity(boost::decimal::decimal128_t{"-0.5"});

    return boost::decimal::test::report_errors();
}


