// Copyright 2025 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//
// This example, when run with the pretty printers, shows how various values are represented

#include <boost/decimal/decimal32_t.hpp>    // For type decimal32_t
#include <boost/decimal/cmath.hpp>          // For nan function to write payload to nans
#include <limits>

int main()
{
    using boost::decimal::decimal32_t;

    const decimal32_t max {std::numeric_limits<decimal32_t>::max()};
    const decimal32_t min {std::numeric_limits<decimal32_t>::min()};
    const decimal32_t short_num {"3.140"};
    const decimal32_t pos_inf {std::numeric_limits<decimal32_t>::infinity()};
    const decimal32_t neg_inf {-std::numeric_limits<decimal32_t>::infinity()};

    const decimal32_t qnan {std::numeric_limits<decimal32_t>::quiet_NaN()};
    const decimal32_t snan {std::numeric_limits<decimal32_t>::signaling_NaN()};

    const decimal32_t payload_nan {boost::decimal::nand32("7")};

    // Break Here:
    static_cast<void>(max);
    static_cast<void>(min);
    static_cast<void>(short_num);
    static_cast<void>(pos_inf);
    static_cast<void>(neg_inf);
    static_cast<void>(qnan);
    static_cast<void>(snan);
    static_cast<void>(payload_nan);

    return 0;
}
