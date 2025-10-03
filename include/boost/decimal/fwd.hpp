// Copyright 2023 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_DECIMAL_FWD_HPP
#define BOOST_DECIMAL_FWD_HPP

#include <boost/decimal/detail/config.hpp>

#ifndef BOOST_DECIMAL_BUILD_MODULE
#include <limits>

namespace boost {
namespace decimal {

class decimal32_t;
class decimal_fast32_t;
class decimal64_t;
class decimal_fast64_t;
class decimal128_t;
class decimal_fast128_t;

#ifdef BOOST_DECIMAL_HAS_BUILTIN_DECIMAL

namespace detail {

template <typename BasisType>
class hardware_wrapper;

} // namespace detail

using builtin_decimal32_t = detail::hardware_wrapper<std::decimal::decimal32>;
using builtin_decimal64_t = detail::hardware_wrapper<std::decimal::decimal64>;
using builtin_decimal128_t = detail::hardware_wrapper<std::decimal::decimal128>;

#endif

} // namespace decimal
} // namespace boost

namespace std {

template <>
#ifdef _MSC_VER
class numeric_limits<boost::decimal::decimal32_t>;
#else
struct numeric_limits<boost::decimal::decimal32_t>;
#endif

template <>
#ifdef _MSC_VER
class numeric_limits<boost::decimal::decimal_fast32_t>;
#else
struct numeric_limits<boost::decimal::decimal_fast32_t>;
#endif

template <>
#ifdef _MSC_VER
class numeric_limits<boost::decimal::decimal64_t>;
#else
struct numeric_limits<boost::decimal::decimal64_t>;
#endif

template <>
#ifdef _MSC_VER
class numeric_limits<boost::decimal::decimal_fast64_t>;
#else
struct numeric_limits<boost::decimal::decimal_fast64_t>;
#endif

template <>
#ifdef _MSC_VER
class numeric_limits<boost::decimal::decimal128_t>;
#else
struct numeric_limits<boost::decimal::decimal128_t>;
#endif

template <>
#ifdef _MSC_VER
class numeric_limits<boost::decimal::decimal_fast128_t>;
#else
struct numeric_limits<boost::decimal::decimal_fast128_t>;
#endif

template <>
#ifdef _MSC_VER
class numeric_limits<boost::decimal::builtin_decimal32_t>;
#else
struct numeric_limits<boost::decimal::builtin_decimal32_t>;
#endif

template <>
#ifdef _MSC_VER
class numeric_limits<boost::decimal::builtin_decimal64_t>;
#else
struct numeric_limits<boost::decimal::builtin_decimal64_t>;
#endif

template <>
#ifdef _MSC_VER
class numeric_limits<boost::decimal::builtin_decimal128_t>;
#else
struct numeric_limits<boost::decimal::builtin_decimal128_t>;
#endif

} // Namespace std

#endif // BOOST_DECIMAL_BUILD_MODULE

#endif // BOOST_DECIMAL_FWD_HPP
