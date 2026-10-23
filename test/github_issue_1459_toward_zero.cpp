// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//
// The same checks under the compile-time toward zero mode, where both signs are finite,
// and without constant evaluation detection, where only that mode exists

#define BOOST_DECIMAL_FE_DEC_TOWARD_ZERO
#define BOOST_DECIMAL_NO_CONSTEVAL_DETECTION

#include "github_issue_1459.cpp"
