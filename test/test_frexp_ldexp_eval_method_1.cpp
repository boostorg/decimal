// Copyright 2026 Shen-Ta Hsieh
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//
// The same checks with BOOST_DECIMAL_DEC_EVAL_METHOD 1. frexp and ldexp compute in the type of the value,
// so the results do not change.

#define BOOST_DECIMAL_DEC_EVAL_METHOD 1

#include "test_frexp_ldexp.cpp"
