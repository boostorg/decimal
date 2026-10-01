// Copyright 2026 Shen-Ta Hsieh
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include <boost/decimal.hpp>
#include <boost/core/lightweight_test.hpp>
#include <limits>

using namespace boost::decimal;

// A value below the smallest subnormal rounds in the current mode: a one digit significand, a
// value whose digits all drop, and a coefficient type narrower or wider than the significand.
template <typename T>
void test()
{
    using sig_type = typename T::significand_type;
    using wide_type = decimal128_t::significand_type;
    constexpr int etiny {std::numeric_limits<T>::min_exponent10 - std::numeric_limits<T>::digits10 + 1};
    const T zero {0};

    // Half of the smallest step is a tie, and the even neighbour is zero
    fesetround(rounding_mode::fe_dec_to_nearest);
    BOOST_TEST_EQ(T(sig_type{5}, etiny - 1), zero);
    BOOST_TEST_EQ(T(sig_type{5}, etiny - 1, true), -zero);
    BOOST_TEST(signbit(T(sig_type{5}, etiny - 1, true)));
    BOOST_TEST_EQ(T(sig_type{5}, etiny + 1) * T(sig_type{1}, -2), zero);
    // An int coefficient far below: decimal128_t read past the pow10 table here
    BOOST_TEST_EQ(T(11, etiny - 70), zero);
    BOOST_TEST_EQ(T(10, etiny - 55), zero);
    // A 128-bit coefficient far below: the divisor of 32 and 64-bit types lost its high bits
    BOOST_TEST_EQ(T(wide_type{123}, etiny - 33), zero);
    BOOST_TEST_EQ(T(wide_type{UINT64_C(3976006815679288586)}, etiny - 38), zero);

    #ifndef BOOST_DECIMAL_NO_CONSTEVAL_DETECTION
    // fesetround has an effect only with the detection of constant evaluation
    const T smallest {std::numeric_limits<T>::denorm_min()};

    // A positive value goes up to the smallest step, also when all its digits drop
    fesetround(rounding_mode::fe_dec_upward);
    BOOST_TEST_EQ(T(sig_type{1}, etiny - 2), smallest);
    BOOST_TEST_EQ(T(sig_type{1}, etiny + 1) * T(sig_type{1}, -3), smallest);
    BOOST_TEST_EQ(T(sig_type{11}, etiny - 70), smallest);
    BOOST_TEST_EQ(T(11, etiny - 70), smallest);
    BOOST_TEST_EQ(T(wide_type{123}, etiny - 33), smallest);
    // A zero stays zero
    BOOST_TEST_EQ(T(sig_type{0}, etiny - 70), zero);
    BOOST_TEST_EQ(T(sig_type{5}, etiny + 5) * T(sig_type{0}, -70), zero);

    // A negative value goes down to minus the smallest step, also when all its digits drop
    fesetround(rounding_mode::fe_dec_downward);
    BOOST_TEST_EQ(T(sig_type{1}, etiny - 2, true), -smallest);
    BOOST_TEST_EQ(T(sig_type{7}, etiny - 1, true), -smallest);
    BOOST_TEST_EQ(T(sig_type{1}, etiny + 1, true) * T(sig_type{1}, -3), -smallest);
    BOOST_TEST_EQ(T(sig_type{11}, etiny - 70, true), -smallest);

    // A tie goes away from zero, but a value less than half of the smallest step goes to zero
    fesetround(rounding_mode::fe_dec_to_nearest_from_zero);
    BOOST_TEST_EQ(T(sig_type{5}, etiny - 1), smallest);
    BOOST_TEST_EQ(T(sig_type{5}, etiny - 1, true), -smallest);
    BOOST_TEST_EQ(T(sig_type{4}, etiny - 1), zero);
    BOOST_TEST_EQ(T(sig_type{5}, etiny + 1) * T(sig_type{1}, -2), smallest);
    BOOST_TEST_EQ(T(11, etiny - 70), zero);
    BOOST_TEST_EQ(T(wide_type{123}, etiny - 33), zero);

    // All values go to zero and keep their sign
    fesetround(rounding_mode::fe_dec_toward_zero);
    BOOST_TEST_EQ(T(sig_type{9}, etiny - 1), zero);
    BOOST_TEST(signbit(T(sig_type{9}, etiny - 1, true)));
    BOOST_TEST_EQ(T(sig_type{1}, etiny + 1) * T(sig_type{9}, -2), zero);
    BOOST_TEST_EQ(T(sig_type{11}, etiny - 70, true), -zero);
    BOOST_TEST_EQ(T(-11, etiny - 70), -zero);
    BOOST_TEST_EQ(T(wide_type{123}, etiny - 33), zero);

    fesetround(rounding_mode::fe_dec_to_nearest);
    #endif
}

int main()
{
    test<decimal32_t>();
    test<decimal64_t>();
    test<decimal128_t>();

    return boost::report_errors();
}
