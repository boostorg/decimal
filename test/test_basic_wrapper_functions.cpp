// Copyright 2025 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#if __has_include(<decimal/decimal>)

#include <boost/decimal.hpp>
#include <boost/core/lightweight_test.hpp>

int main()
{
    return boost::report_errors();
}

#else

int main()
{
    return 0;
}

#endif
