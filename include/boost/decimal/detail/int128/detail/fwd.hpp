// Copyright 2025 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_DECIMAL_DETAIL_INT128_DETAIL_FWD_HPP
#define BOOST_DECIMAL_DETAIL_INT128_DETAIL_FWD_HPP

#include <boost/decimal/detail/int128/detail/config.hpp>

namespace boost {
namespace int128 {

BOOST_DECIMAL_DETAIL_INT128_EXPORT struct uint128;
BOOST_DECIMAL_DETAIL_INT128_EXPORT struct int128;

// Decimal-local: Boost.Decimal refers to the types by their pre-rename names
BOOST_DECIMAL_DETAIL_INT128_EXPORT using uint128_t = uint128;
BOOST_DECIMAL_DETAIL_INT128_EXPORT using int128_t = int128;

} // namespace int128
} // namespace boost

#endif // BOOST_DECIMAL_DETAIL_INT128_DETAIL_FWD_HPP
