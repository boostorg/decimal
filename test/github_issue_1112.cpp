// Copyright 2025 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//
// See: https://github.com/cppalliance/decimal/issues/1112

#include <boost/decimal.hpp>
#include <boost/core/lightweight_test.hpp>

using namespace boost::decimal;

template <typename T>
void reproducer()
{
    const auto dec_res {static_cast<int>(nearbyint(T{2325, -1}))};
    const auto dbl_res {static_cast<int>(std::nearbyint(232.5))};
    BOOST_TEST_EQ(dec_res, dbl_res);
}

int main()
{
    reproducer<decimal32_t>();
    reproducer<decimal64_t>();
    reproducer<decimal128_t>();

    return boost::report_errors();
}
