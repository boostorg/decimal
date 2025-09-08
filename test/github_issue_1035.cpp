// Copyright 2025 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//
// See: https://github.com/cppalliance/decimal/issues/1035

#include <boost/decimal.hpp>
#include <boost/core/lightweight_test.hpp>

using namespace boost::decimal;
using namespace boost::decimal::literals;

int main()
{
    const auto previously_inf {"5e+95"_DF};
    BOOST_TEST_EQ(previously_inf, "500000e+90"_DF);

    #ifdef BOOST_DECIMAL_NO_CONSTEVAL_DETECTION
    return boost::report_errors();
    #else

    fesetround(rounding_mode::fe_dec_downward);
    BOOST_TEST_EQ("5e+50"_DF - "4e+40"_DF, "4.999999e+50"_DF);
    BOOST_TEST_EQ("5e+95"_DF - "4e-100"_DF, "4.999999e+95"_DF);
    BOOST_TEST_EQ("-5e+95"_DF + "4e-100"_DF, "-4.999999e+95"_DF);

    fesetround(rounding_mode::fe_dec_upward);
    BOOST_TEST_EQ("5e+50"_DF + "4e+40"_DF, "5.000001e+50"_DF);
    BOOST_TEST_EQ("5e+95"_DF + "4e-100"_DF, "5.000001e+95"_DF);
    BOOST_TEST_EQ("-5e+95"_DF - "4e-100"_DF, "-5.000001e+95"_DF);

    return boost::report_errors();

    #endif
}
