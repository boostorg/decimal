// Copyright 2024 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#define BOOST_DECIMAL_FE_DEC_DOWNWARD
#include <boost/decimal.hpp>

int main()
{
    using namespace boost::decimal::literals;

    constexpr auto lhs {"5e+50"_DF};
    constexpr auto rhs {"4e+40"_DF};
    constexpr auto downward_res {lhs - rhs};
    static_assert(downward_res == "4.999999e+50"_DF, "Incorrectly rounded result");

    return 0;
}
