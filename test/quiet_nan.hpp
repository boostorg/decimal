// Copyright 2026 Shen-Ta Hsieh
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_DECIMAL_TEST_QUIET_NAN_HPP
#define BOOST_DECIMAL_TEST_QUIET_NAN_HPP

#include <boost/decimal.hpp>

// x is the quiet form of the signaling NaN s, with the same sign and the payload s keeps in the evaluation type
template <typename T>
bool is_quiet_of(const T x, const T s)
{
    return isnan(x) && !issignaling(x) && signbit(x) == signbit(s) && read_payload(x) == read_payload(static_cast<T>(static_cast<boost::decimal::detail::evaluation_type_t<T>>(s)));
}

#endif // BOOST_DECIMAL_TEST_QUIET_NAN_HPP
