// Copyright 2026 Shen-Ta Hsieh
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

// boost/decimal.hpp comes first, so no standard header defines the feature test macro before it
#include <boost/decimal.hpp>
#include <boost/core/lightweight_test.hpp>
#include <limits>

#if __has_include(<compare>) && (__cplusplus > 201703L || (defined(_MSVC_LANG) && _MSVC_LANG > 201703L))
#  include <compare>
#endif

#if defined(__cpp_lib_three_way_comparison) && __cpp_lib_three_way_comparison >= 201907L

#ifndef BOOST_DECIMAL_HAS_SPACESHIP_OPERATOR
#  error "The library does not give operator<=>, but the standard library supports it"
#endif

using namespace boost::decimal;

template <typename T>
void test()
{
    const T one {1};
    const T two {2};

    BOOST_TEST((one <=> two) == std::partial_ordering::less);
    BOOST_TEST((two <=> one) == std::partial_ordering::greater);
    BOOST_TEST((one <=> one) == std::partial_ordering::equivalent);
    BOOST_TEST((one <=> 2) == std::partial_ordering::less);
    BOOST_TEST((2 <=> one) == std::partial_ordering::greater);

    #ifndef BOOST_DECIMAL_FAST_MATH
    const T nan {std::numeric_limits<T>::quiet_NaN()};
    BOOST_TEST((nan <=> one) == std::partial_ordering::unordered);
    BOOST_TEST((nan <=> 1) == std::partial_ordering::unordered);
    BOOST_TEST((1 <=> nan) == std::partial_ordering::unordered);
    #endif
}

int main()
{
    test<decimal32_t>();
    test<decimal64_t>();
    test<decimal128_t>();
    test<decimal_fast32_t>();
    test<decimal_fast64_t>();
    test<decimal_fast128_t>();

    return boost::report_errors();
}

#else

int main()
{
    return 0;
}

#endif
