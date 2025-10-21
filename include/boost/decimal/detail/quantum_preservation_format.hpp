// Copyright 2025 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_DECIMAL_COHORT_FORMAT_HPP
#define BOOST_DECIMAL_COHORT_FORMAT_HPP

#include <boost/decimal/detail/config.hpp>

namespace boost {
namespace decimal {

// IEEE 754 allows languages to decide how to allow cohorts to be printed.
// Precision and cohort preservation are mutually exclusive,
// so we offer it as an option through charconv.
// See Section 5.12.2
//
// on - preserves cohort information
// off - returns the same as to_chars with specified format and unspecified precision
BOOST_DECIMAL_EXPORT enum class quantum_preservation : unsigned
{
    off = 0,
    on = 1
};

} //namespace decimal
} //namespace boost

#endif // BOOST_DECIMAL_COHORT_FORMAT_HPP
