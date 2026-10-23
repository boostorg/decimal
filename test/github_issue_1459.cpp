// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//
// https://github.com/boostorg/decimal/issues/1459
//
// IEEE 754-2019 7.4: an overflow is the largest finite value in the toward zero mode, and
// in the directed mode which points at zero for the sign. Every overflow gave an infinity.

#include <boost/decimal.hpp>
#include <boost/core/lightweight_test.hpp>
#include <limits>

using namespace boost::decimal;

// BOOST_DECIMAL_IF_CONSTEXPR is a plain if before C++17, so the constant condition warns
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4127)
#endif

// The largest finite value or an infinity, with the sign of the result
template <typename T>
void check_overflow(const T value, const bool neg, const bool finite)
{
    BOOST_TEST_EQ(signbit(value), neg);
    BOOST_TEST_EQ(isinf(value), !finite);
    BOOST_TEST_EQ(abs(value) == (std::numeric_limits<T>::max)(), finite);
}

template <typename T>
void check(const rounding_mode mode)
{
    fesetround(mode);
    const T m {(std::numeric_limits<T>::max)()};
    const T zero {0, 0};
    const T two {2, 0};
    const bool pos {mode == rounding_mode::fe_dec_toward_zero || mode == rounding_mode::fe_dec_downward};
    const bool neg {mode == rounding_mode::fe_dec_toward_zero || mode == rounding_mode::fe_dec_upward};

    check_overflow(m * m, false, pos);
    check_overflow(-m * m, true, neg);
    check_overflow(m + m, false, pos);
    check_overflow(-m - m, true, neg);
    check_overflow(m * two, false, pos);
    check_overflow(-m * two, true, neg);
    check_overflow(fma(m, m, zero), false, pos);
    check_overflow(fma(-m, m, zero), true, neg);
    check_overflow(T {1, 99999}, false, pos);
    check_overflow(T {-1, 99999}, true, neg);
    check_overflow(T {"1e99999"}, false, pos);
    check_overflow(T {"-1e99999"}, true, neg);

    // The largest long double overflows the 32 and 64 bit types, but not the 128 bit types
    BOOST_DECIMAL_IF_CONSTEXPR (std::numeric_limits<long double>::max_exponent10 > std::numeric_limits<T>::max_exponent10)
    {
        check_overflow(T {std::numeric_limits<long double>::max()}, false, pos);
        check_overflow(T {-std::numeric_limits<long double>::max()}, true, neg);
    }
}

template <typename T>
void test()
{
    #ifdef BOOST_DECIMAL_NO_CONSTEVAL_DETECTION
    // fesetround has no effect here, thus only the compile-time mode is tested
    check<T>(_boost_decimal_global_rounding_mode);
    #else
    check<T>(rounding_mode::fe_dec_to_nearest);
    check<T>(rounding_mode::fe_dec_downward);
    check<T>(rounding_mode::fe_dec_upward);
    check<T>(rounding_mode::fe_dec_toward_zero);
    check<T>(rounding_mode::fe_dec_to_nearest_from_zero);
    #endif
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif

int main()
{
    constexpr bool finite_positive {_boost_decimal_global_rounding_mode == rounding_mode::fe_dec_toward_zero ||
                                    _boost_decimal_global_rounding_mode == rounding_mode::fe_dec_downward};
    static_assert(isinf(decimal32_t {1, 99999}) != finite_positive, "");
    static_assert(isinf(decimal128_t {1, 99999}) != finite_positive, "");

    test<decimal32_t>();
    test<decimal_fast32_t>();
    test<decimal64_t>();
    test<decimal_fast64_t>();
    test<decimal128_t>();
    test<decimal_fast128_t>();

    fesetround(rounding_mode::fe_dec_to_nearest);

    return boost::report_errors();
}
