// Copyright 2024 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include "test.hpp"
#include <boost/decimal/decimal32_t.hpp>
#include <boost/decimal/decimal_fast32_t.hpp>
#include <boost/decimal/dpd_conversion.hpp>
#include <boost/decimal/bid_conversion.hpp>
#include <iostream>

int main()
{
    using boost::decimal::decimal32_t;
    using boost::decimal::decimal_fast32_t;
    using boost::decimal::from_bid;
    using boost::decimal::from_dpd;
    using boost::decimal::to_bid;
    using boost::decimal::to_dpd;

    const decimal_fast32_t fast_type {5};
    const std::uint32_t BID_bits {to_bid(fast_type)};
    const std::uint32_t DPD_bits {to_dpd(fast_type)};

    std::cout << std::hex
              << "BID format: " << BID_bits << '\n'
              << "DPD format: " << DPD_bits << std::endl;

    const decimal32_t bid_decimal {from_bid<decimal32_t>(BID_bits)};
    const decimal32_t dpd_decimal {from_dpd<decimal32_t>(DPD_bits)};

    BOOST_DECIMAL_TEST_NE(bid_decimal, dpd_decimal);

    return boost::decimal::test::report_errors();
}
