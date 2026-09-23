// Copyright 2026 Shen-Ta Hsieh
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include <boost/decimal.hpp>
#include <boost/core/lightweight_test.hpp>
#include <cfenv>
#include <cmath>
#include <limits>

using namespace boost::decimal;

template <typename Decimal, typename Float>
void test_nonfinite()
{
    // The zero and the non-finite values have their own paths in the constructor, and the
    // volatile source keeps them out of a constant fold, so that the constructor really runs
    volatile Float volatile_inf {static_cast<Float>(HUGE_VAL)};
    volatile Float volatile_nan {static_cast<Float>(std::numeric_limits<double>::quiet_NaN())};
    const Float inf {volatile_inf};
    const Float nan {volatile_nan};
    std::feclearexcept(FE_ALL_EXCEPT);
    BOOST_TEST_EQ(Decimal(Float(0)), Decimal(0));
    BOOST_TEST(isinf(Decimal(inf)) && !signbit(Decimal(inf)));
    BOOST_TEST(isinf(Decimal(-inf)) && signbit(Decimal(-inf)));
    BOOST_TEST(isnan(Decimal(nan)));

    // A constructor which did arithmetic on the value, as inf - inf is, would raise this
    BOOST_TEST_EQ(std::fetestexcept(FE_INVALID), 0);

    // A conversion or a load of a signaling NaN can raise FE_INVALID, so only the result counts
    volatile double volatile_snan {std::numeric_limits<double>::signaling_NaN()};
    BOOST_TEST(isnan(Decimal(static_cast<Float>(volatile_snan))));
}

template <typename Decimal>
void test_all_floats()
{
    test_nonfinite<Decimal, float>();
    test_nonfinite<Decimal, double>();
    #ifndef BOOST_DECIMAL_UNSUPPORTED_LONG_DOUBLE
    test_nonfinite<Decimal, long double>();
    #endif
    #if defined(BOOST_DECIMAL_HAS_FLOAT128) && (!defined(__clang_major__) || __clang_major__ >= 13)
    test_nonfinite<Decimal, __float128>();
    #endif

    // Arithmetic on the value would also fail this constant expression
    #ifdef BOOST_DECIMAL_HAS_CONSTEXPR_BITCAST
    constexpr Decimal constant_inf {std::numeric_limits<double>::infinity()};
    static_assert(constant_inf > Decimal {1}, "the constructor lost the infinity");
    #endif
}

int main()
{
    test_all_floats<decimal32_t>();
    test_all_floats<decimal64_t>();
    test_all_floats<decimal128_t>();
    test_all_floats<decimal_fast32_t>();
    test_all_floats<decimal_fast64_t>();
    test_all_floats<decimal_fast128_t>();

    return boost::report_errors();
}
