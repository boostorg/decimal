// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//
// See: https://github.com/cppalliance/decimal/issues/1306

#include <boost/decimal.hpp>
#include <boost/core/lightweight_test.hpp>
#include <limits>

using namespace boost::decimal;

int main()
{
    BOOST_TEST_EQ(decimal64_t{"1.3e-394"} * decimal64_t{"1e-4"}, decimal64_t{"1e-398"});
    BOOST_TEST_EQ(decimal64_t{"1.5e-394"} * decimal64_t{"1e-4"}, decimal64_t{"2e-398"});

    return boost::report_errors();
}
