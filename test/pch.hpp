// Copyright 2026 Shen-Ta Hsieh
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

// Precompiled by b2 once per build variant and force-included into each test (see Jamfile)

// The MSVC standard library reads this when the pch parses <limits>, not when the Boost.Math
// tests define it later, so it must come first here (the define in those tests is the same)
#define _SILENCE_CXX23_DENORM_DEPRECATION_WARNING

#include <boost/decimal.hpp>
