// Copyright 2025 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_DECIMAL_BUILTIN_DECIMAL128_T_HPP
#define BOOST_DECIMAL_BUILTIN_DECIMAL128_T_HPP

#include <boost/decimal/detail/hardware_wrapper_template.hpp>

#ifdef BOOST_DECIMAL_HAS_BUILTIN_DECIMAL

namespace boost {
namespace decimal {

using builtin_decimal128_t = detail::hardware_wrapper<std::decimal::decimal128>;

} // namespace decimal
} // namespace boost

#endif // ifdef BOOST_DECIMAL_HAS_BUILTIN_DECIMAL

#endif // BOOST_DECIMAL_BUILTIN_DECIMAL128_T_HPP
