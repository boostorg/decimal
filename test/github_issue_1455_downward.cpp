// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//
// https://github.com/boostorg/decimal/issues/1455
//
// The same checks in the fixed downward mode without constant evaluation detection,
// thus every path reads the compile-time mode and every exact cancel is -0.

#define BOOST_DECIMAL_FE_DEC_DOWNWARD
#define BOOST_DECIMAL_NO_CONSTEVAL_DETECTION

#include "github_issue_1455.cpp"
