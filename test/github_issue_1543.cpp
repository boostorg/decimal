// Copyright 2026 Shen-Ta Hsieh
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//
// See: https://github.com/boostorg/decimal/issues/1543

#include <boost/decimal.hpp>
#include <boost/core/lightweight_test.hpp>
#include <limits>
#include <stdexcept>
#include <string>

using namespace boost::decimal;

// A negative overflow is an overflow too, the same as a positive one
template <typename T, typename Function>
void test(Function stod)
{
    #ifndef BOOST_DECIMAL_FAST_MATH
    for (const std::string str : {"1e999999", "-1e999999", "INF", "-INF"})
    {
        #ifndef BOOST_DECIMAL_DISABLE_EXCEPTIONS
        BOOST_TEST_THROWS(stod(str, nullptr), std::out_of_range);
        #else
        BOOST_TEST(isnan(stod(str, nullptr)));
        #endif
    }
    #endif

    std::size_t idx {};
    BOOST_TEST_EQ(stod("-1.5e2", &idx), T{-150});
    BOOST_TEST_EQ(idx, 6U);

    // Toward zero, an overflow gives the largest finite value, and it still throws.
    // fesetround has an effect only with the detection of constant evaluation
    #if !defined(BOOST_DECIMAL_FAST_MATH) && !defined(BOOST_DECIMAL_NO_CONSTEVAL_DETECTION) && !defined(BOOST_DECIMAL_DISABLE_EXCEPTIONS)
    fesetround(rounding_mode::fe_dec_toward_zero);
    const std::string over {"1e" + std::to_string(std::numeric_limits<T>::max_exponent10 + 1)};
    for (const std::string& str : {over, "-" + over, std::string("-1e1000000")})
    {
        BOOST_TEST_THROWS(stod(str, nullptr), std::out_of_range);
    }
    BOOST_TEST_EQ(stod("9.99e" + std::to_string(std::numeric_limits<T>::max_exponent10), nullptr), T(999, std::numeric_limits<T>::max_exponent10 - 2));
    fesetround(rounding_mode::fe_dec_to_nearest);
    #endif
}

int main()
{
    test<decimal32_t>(stod32);
    test<decimal_fast32_t>(stod32f);
    test<decimal64_t>(stod64);
    test<decimal_fast64_t>(stod64f);
    test<decimal128_t>(stod128);
    test<decimal_fast128_t>(stod128f);

    return boost::report_errors();
}
