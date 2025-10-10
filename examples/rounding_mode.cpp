// Copyright 2024 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include <boost/decimal.hpp>
#include <cassert>

int main()
{
    #ifndef BOOST_DECIMAL_NO_CONSTEVAL_DETECTION

    using namespace boost::decimal::literals;

    BOOST_DECIMAL_ATTRIBUTE_UNUSED auto default_rounding_mode = boost::decimal::fegetround(); // Default is fe_dec_to_nearest

    BOOST_DECIMAL_ATTRIBUTE_UNUSED auto new_rounding_mode = boost::decimal::fesetround(boost::decimal::rounding_mode::fe_dec_upward);

    assert(default_rounding_mode != new_rounding_mode);

    const auto lhs {"5e+50"_DF};
    const auto rhs {"4e+40"_DF};

    // With upward rounding the result will be "5.000001e+50"_DF
    // Even though the difference in order of magnitude is greater than the precision of the type,
    // any addition in this mode will result in at least a one ULP difference
    BOOST_DECIMAL_ATTRIBUTE_UNUSED const auto upward_res {lhs + rhs};
    assert(upward_res == "5.000001e+50"_DF);

    boost::decimal::fesetround(boost::decimal::rounding_mode::fe_dec_downward);

    // Similar to above in the downward rounding mode any subtraction will result in at least a one ULP difference
    BOOST_DECIMAL_ATTRIBUTE_UNUSED const auto downward_res {lhs - rhs};
    assert(downward_res == "4.999999e+50"_DF);

    #endif // BOOST_DECIMAL_NO_CONSTEVAL_DETECTION

    return 0;
}

