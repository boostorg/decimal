// Copyright 2024 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

// Global module fragment required for non-module preprocessing
module;

// import std exports declarations but not macros. The C headers below provide the
// feature-test macros and object-like macros the library uses (assert, errno,
// FE_ rounding, FLT_/DBL_ limits, LC_ locale, INT64_C, the stdout/stderr streams,
// ...) so they are included regardless of whether import std is used. import std
// (and std.compat) deliberately do not provide stdout/stderr/stdin, so <cstdio>
// must be textual here for boost::decimal::printf's fprintf(stdout, ...).
#include <version>
#include <cassert>
#include <cerrno>
#include <cfenv>
#include <cfloat>
#include <cinttypes>
#include <climits>
#include <clocale>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cwchar>

// Platform intrinsic headers and the MSVC built-in 128-bit header are not part of
// the standard library module, so they are always brought in here.
#if __has_include(<__msvc_int128.hpp>) && _MSVC_LANG >= 202002L
#  include <__msvc_int128.hpp>
#endif

#if defined(_MSC_VER)
#  include <intrin.h>
#elif defined(__x86_64__)
#  include <x86intrin.h>
#  include <emmintrin.h>
#elif defined(__i386__)
#  include <emmintrin.h>
#elif defined(__ARM_NEON__)
#  include <arm_neon.h>
#endif

// When the standard library module is available these are provided by import
// std, otherwise they are supplied here in the global module fragment.
#ifndef BOOST_DECIMAL_USE_STD_MODULE

#include <algorithm>
#include <array>
#include <bit>
#include <cctype>
#include <charconv>
#include <compare>
#include <complex>
#include <concepts>
#include <functional>
#include <initializer_list>
#include <iomanip>
#include <ios>
#include <iosfwd>
#include <iostream>
#include <iterator>
#include <limits>
#include <locale>
#include <memory>
#include <new>
#include <ostream>
#include <sstream>
#include <string>
#include <string_view>
#include <system_error>
#include <tuple>
#include <type_traits>
#include <utility>

// <format> and <stdfloat> are newer library features that are not available on
// every toolchain yet, so they are only pulled in when present. <format> uses the
// same feature gate as format.hpp so it is supplied whenever that header needs it.
#if __has_include(<format>) && defined(__cpp_lib_format) && __cpp_lib_format >= 201907L
#  include <format>
#endif

#if __has_include(<stdfloat>)
#  include <stdfloat>
#endif

#endif // BOOST_DECIMAL_USE_STD_MODULE

#define BOOST_DECIMAL_BUILD_MODULE

// The vendored Boost.Int128 keys its inline-constexpr and export macros off its
// own build-module flag; set it so its detail constants get inline (external)
// linkage rather than static, otherwise extern "C++" exposes them as TU-local.
// It also declares its builtin 128-bit aliases only in the interface unit (a
// module consumer receives them through the import), so mark this unit as such.
#define BOOST_DECIMAL_DETAIL_INT128_BUILD_MODULE
#define BOOST_DECIMAL_DETAIL_INT128_INTERFACE_UNIT

export module boost.decimal;

#ifdef BOOST_DECIMAL_USE_STD_MODULE
import std;
#endif

// Wrap the exported declarations in extern "C++" so they attach to the global
// module (classic mangling), keeping the module and header forms ABI compatible.
extern "C++" {

// Forward declarations are not available so add the contents of fwd.hpp here

export namespace boost::decimal {

class decimal32_t;
class decimal64_t;
class decimal128_t;

class decimal_fast32_t;
class decimal_fast64_t;
class decimal_fast128_t;

} // namespace boost::decimal

export namespace std {

template <>
class numeric_limits<boost::decimal::decimal32_t>;

template <>
class numeric_limits<boost::decimal::decimal_fast32_t>;

template <>
class numeric_limits<boost::decimal::decimal64_t>;

template <>
class numeric_limits<boost::decimal::decimal_fast64_t>;

template <>
class numeric_limits<boost::decimal::decimal128_t>;

template <>
class numeric_limits<boost::decimal::decimal_fast128_t>;

} // Namespace std

// MSVC wants <boost/decimal> to be imported but also does not support importing it...
#ifdef _MSC_VER
#  pragma warning( push )
#  pragma warning( disable : 5244 )
#elif defined(__clang__)
#  pragma clang diagnostic push
#  pragma clang diagnostic ignored "-Winclude-angled-in-module-purview"
#endif

#include <boost/decimal.hpp>

#ifdef _MSC_VER
#  pragma warning( pop )
#elif defined(__clang__)
#  pragma clang diagnostic pop
#endif

} // extern "C++"
