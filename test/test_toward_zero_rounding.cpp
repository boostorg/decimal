// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

// The mode is compile time, thus the test runs on every platform
#define BOOST_DECIMAL_FE_DEC_TOWARD_ZERO

#include <boost/decimal.hpp>
#include <boost/core/lightweight_test.hpp>

using namespace boost::decimal;
using namespace boost::decimal::literals;

// A quotient of either sign drops its extra digits; downward gives -0.6666667
template <typename T>
void test(const T two, const T three, const T quotient)
{
    BOOST_TEST_EQ(two / three, quotient);
    BOOST_TEST_EQ(-two / three, -quotient);
}

int main()
{
    BOOST_TEST(_boost_decimal_global_rounding_mode == rounding_mode::fe_dec_toward_zero);
    BOOST_TEST(boost::decimal::fegetround() == rounding_mode::fe_dec_toward_zero);

    static_assert(-2_DF / 3_DF == -0.6666666_DF, "");
    static_assert(-2_DL / 3_DL == -0.6666666666666666666666666666666666_DL, "");

    test(2_DF, 3_DF, 0.6666666_DF);
    test(2_DFF, 3_DFF, 0.6666666_DFF);
    test(2_DD, 3_DD, 0.6666666666666666_DD);
    test(2_DDF, 3_DDF, 0.6666666666666666_DDF);
    test(2_DL, 3_DL, 0.6666666666666666666666666666666666_DL);
    test(2_DLF, 3_DLF, 0.6666666666666666666666666666666666_DLF);

    return boost::report_errors();
}
