// Copyright 2023 Peter Dimov
// Copyright 2025 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include <boost/decimal/charconv.hpp>
#include <limits>

void test_odr_use( int const* );

template<typename T> void test()
{
    test_odr_use( &boost::decimal::limits<T>::max_chars );
    test_odr_use( &std::numeric_limits<T>::digits10 );
}

void f1()
{
    test<boost::decimal::decimal32_t>();
    test<boost::decimal::decimal64_t>();
    test<boost::decimal::decimal128_t>();

    test<boost::decimal::decimal_fast32_t>();
    test<boost::decimal::decimal_fast64_t>();
    test<boost::decimal::decimal_fast128_t>();
}
