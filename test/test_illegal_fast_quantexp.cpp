// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

// quantexp is defined in terms of cohorts, which the fast types do not have,
// so it must be rejected at compile time rather than reporting a magnitude

#include <boost/decimal.hpp>

int main()
{
    const boost::decimal::decimal_fast32_t x {UINT32_C(1234567), -4};

    return quantexp(x);
}
