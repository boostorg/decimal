// Copyright 2025 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_DECIMAL_BUILTIN_DECIMAL128_T_HPP
#define BOOST_DECIMAL_BUILTIN_DECIMAL128_T_HPP

#include <boost/decimal/detail/hardware_wrapper_template.hpp>
#include <boost/decimal/fwd.hpp>

// TODO(mborland): Define numeric_limits
// In all other cases we write the significand from a 128-bit integer, but we can't do that here
// I think the best would be to find the relevant bit patterns and then memcpy them into the BasisType

#endif // BOOST_DECIMAL_BUILTIN_DECIMAL128_T_HPP
