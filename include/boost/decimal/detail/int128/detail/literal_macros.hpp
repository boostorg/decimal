// Copyright 2022 Peter Dimov
// Copyright 2025 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_DECIMAL_DETAIL_INT128_DETAIL_LITERAL_MACROS_HPP
#define BOOST_DECIMAL_DETAIL_INT128_DETAIL_LITERAL_MACROS_HPP

// Convenience macros for the user-defined literals. This header intentionally has
// no includes and declares nothing, so module consumers can pull it in on its own
// to obtain the macros (macros are never part of a module's exported interface).

#define BOOST_DECIMAL_DETAIL_INT128_STRINGIFY(x) #x
#define BOOST_DECIMAL_DETAIL_INT128_UINT128_C(x) boost::int128::literals::operator""_u128(BOOST_DECIMAL_DETAIL_INT128_STRINGIFY(x))
#define BOOST_DECIMAL_DETAIL_INT128_INT128_C(x) boost::int128::literals::operator""_i128(BOOST_DECIMAL_DETAIL_INT128_STRINGIFY(x))

#endif // BOOST_DECIMAL_DETAIL_INT128_DETAIL_LITERAL_MACROS_HPP
