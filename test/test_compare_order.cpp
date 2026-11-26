// Copyright 2026 Shen-Ta Hsieh
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//
// The order of values at the limits of the comparison paths: signs, cohorts, the smallest and
// largest values, and exponent gaps around the size of the products

#include <boost/decimal.hpp>
#include <boost/core/lightweight_test.hpp>
#include <limits>

using namespace boost::decimal;

template <typename T>
void test_pair(const T& low, const T& high)
{
    BOOST_TEST(low < high);
    BOOST_TEST(low <= high);
    BOOST_TEST(high > low);
    BOOST_TEST(high >= low);
    BOOST_TEST(low != high);
    BOOST_TEST(!(high < low));
    BOOST_TEST(!(high <= low));
    BOOST_TEST(!(low > high));
    BOOST_TEST(!(low >= high));
    BOOST_TEST(!(low == high));

    // Negation reverses the order
    BOOST_TEST(-high < -low);
    BOOST_TEST(-high <= -low);
    BOOST_TEST(!(-low < -high));

    // Different signs
    if (high > T{0})
    {
        BOOST_TEST(-high < high);
        BOOST_TEST(!(high < -high));
    }
}

template <typename T>
void test_same(const T& a, const T& b)
{
    BOOST_TEST(a == b);
    BOOST_TEST(a <= b);
    BOOST_TEST(a >= b);
    BOOST_TEST(!(a != b));
    BOOST_TEST(!(a < b));
    BOOST_TEST(!(a > b));
    BOOST_TEST(-a == -b);
    BOOST_TEST(!(-a < -b));
    BOOST_TEST(!(-b < -a));
}

template <typename T>
void test_limits()
{
    using limits = std::numeric_limits<T>;

    // The smallest normal value has the smallest biased exponent in the fast types
    test_pair(T{0}, (limits::min)());
    test_pair((limits::min)(), T{1});
    test_pair(T{1}, (limits::max)());
    test_pair(limits::lowest(), (limits::min)());
    BOOST_TEST(-(limits::min)() < (limits::min)());
    BOOST_TEST(!((limits::min)() < -(limits::min)()));
    BOOST_TEST(-(limits::min)() <= (limits::min)());
    BOOST_TEST(!((limits::min)() <= -(limits::min)()));
    BOOST_TEST(limits::lowest() < (limits::max)());

    // Zeros are equal whatever their signs and exponents are
    test_same(T{0}, -T{0});
    test_same(T(0, 5), T(0, -5));
    test_same(T(0, 5), -T(0, -5));
    BOOST_TEST(-T{0} == 0);
    BOOST_TEST(T{5} == 5);
    BOOST_TEST(T{-5} == -5);
    BOOST_TEST(T{5} != -5);
    BOOST_TEST(T{-5} != 5);
    const T zero_cohort {0, 5};
    BOOST_TEST(-T{1} < zero_cohort);
    BOOST_TEST(zero_cohort < T{1});
    BOOST_TEST(-T{1} < -zero_cohort);

    // Fast math does not test for values that are not finite
    #ifndef BOOST_DECIMAL_FAST_MATH
    // Infinities
    test_pair((limits::max)(), limits::infinity());
    BOOST_TEST(-limits::infinity() < limits::lowest());
    BOOST_TEST(-limits::infinity() < limits::infinity());
    BOOST_TEST(!(limits::infinity() < limits::infinity()));
    BOOST_TEST(limits::infinity() <= limits::infinity());
    BOOST_TEST(!(limits::infinity() < -limits::infinity()));

    // NaN is not ordered
    BOOST_TEST(!(limits::quiet_NaN() < T{1}));
    BOOST_TEST(!(T{1} < limits::quiet_NaN()));
    BOOST_TEST(!(limits::quiet_NaN() <= T{1}));
    BOOST_TEST(!(T{1} <= limits::quiet_NaN()));
    BOOST_TEST(!(limits::quiet_NaN() == limits::quiet_NaN()));
    #endif
}

// Exponent gaps from 0 to 40, on the two sides of equality
template <typename T>
void test_gaps()
{
    using sig_type = typename T::significand_type;
    const int digits {std::numeric_limits<T>::digits10};

    for (int gap {0}; gap <= 40; ++gap)
    {
        const T large {sig_type{1U}, gap};
        const T small_value {sig_type{9U}, gap - 1};
        test_pair(small_value, large);
        test_pair(T{sig_type{1U}, 0}, T{sig_type{2U}, gap});

        if (gap < digits)
        {
            sig_type scaled {1U};
            for (int i {0}; i < gap; ++i)
            {
                scaled = static_cast<sig_type>(scaled * 10U);
            }

            test_same(T{sig_type{1U}, gap}, T{scaled, 0});
            BOOST_TEST((T{sig_type{1U}, gap} != -T{scaled, 0}));
            BOOST_TEST((-T{sig_type{1U}, gap} != T{scaled, 0}));
            test_pair(T{scaled, 0}, T{static_cast<sig_type>(scaled + 1U), 0});
            test_pair(T{sig_type{1U}, gap}, T{static_cast<sig_type>(scaled + 1U), 0});
            if (gap > 0)
            {
                test_pair(T{static_cast<sig_type>(scaled - 1U), 0}, T{sig_type{1U}, gap});
            }
        }
    }
}

// A significand of all digits with a larger exponent has a product above two words
template <typename T>
void test_wide_gaps()
{
    using sig_type = typename T::significand_type;

    sig_type nines {0U};
    for (int i {0}; i < std::numeric_limits<T>::digits10; ++i)
    {
        nines = static_cast<sig_type>(nines * 10U + 9U);
    }

    for (int gap {1}; gap <= 40; ++gap)
    {
        test_pair(T{nines, 0}, T{nines, gap});
        test_pair(T{sig_type{1U}, 0}, T{nines, gap});
    }
}

// Products just above 2^128, whose low two words are less than the other significand
void test_product_overflow()
{
    using namespace boost::decimal::literals;

    test_pair(100000_DL, 3402823669209384634633746074317683e5_DL);
    test_pair(100000_DLF, 3402823669209384634633746074317683e5_DLF);
    test_pair(10000000000000000000000000_DL, 34028236692094e25_DL);
    test_pair(10000000000000000000000000_DLF, 34028236692094e25_DLF);
    // The coefficient 10^34 + 1, which is above the largest one and which only the bits can hold
    const decimal128_t above_max {from_bits(boost::int128::uint128_t{UINT64_C(0x3041ED09BEAD87C0), UINT64_C(0x378D8E6400000001)})};
    BOOST_TEST(decimal128_t(1, 34) < above_max);
    BOOST_TEST(!(decimal128_t(1, 34) > above_max));
    BOOST_TEST(decimal128_t(1, 34) != above_max);

    BOOST_TEST(1_DLF < 340283);
    BOOST_TEST(1_DL < 340283);
}

template <typename A, typename B>
void test_mixed_pair(const A& low, const B& high)
{
    BOOST_TEST(low < high);
    BOOST_TEST(low <= high);
    BOOST_TEST(high > low);
    BOOST_TEST(high >= low);
    BOOST_TEST(low != high);
    BOOST_TEST(!(high < low));
    BOOST_TEST(!(low > high));
    BOOST_TEST(!(low == high));
    BOOST_TEST(-high < -low);
    BOOST_TEST(!(-low < -high));
}

// Two types, with gaps on the two sides of equality and with signs and zeros
template <typename A, typename B>
void test_mixed_types()
{
    for (int gap {0}; gap <= 40; ++gap)
    {
        test_mixed_pair(A(9, gap - 1), B(1, gap));
        test_mixed_pair(B(9, gap - 1), A(1, gap));
        test_mixed_pair(A(1, 0), B(2, gap));
        test_mixed_pair(B(1, 0), A(2, gap));
        test_mixed_pair(-A(1, gap), B(1, gap));
    }

    BOOST_TEST(A(1, 3) == B(1000, 0));
    BOOST_TEST(B(1, 3) == A(1000, 0));
    BOOST_TEST(A(1, 3) != -B(1000, 0));
    BOOST_TEST(A(0, 5) == -B(0, -5));
    BOOST_TEST(!(-A(0, 5) < B(0, -5)));
    BOOST_TEST(!(A(0, 5) < -B(0, -5)));
    BOOST_TEST(-A(1, 0) < B(0, 0));
    BOOST_TEST(A(0, 0) < B(1, -40));

    #ifndef BOOST_DECIMAL_FAST_MATH
    test_mixed_pair(A(1, 0), std::numeric_limits<B>::infinity());
    test_mixed_pair(-std::numeric_limits<A>::infinity(), B(1, 0));
    test_mixed_pair(-std::numeric_limits<A>::infinity(), std::numeric_limits<B>::infinity());
    BOOST_TEST(std::numeric_limits<A>::infinity() == std::numeric_limits<B>::infinity());
    BOOST_TEST(!(std::numeric_limits<A>::infinity() < std::numeric_limits<B>::infinity()));
    BOOST_TEST(!(-std::numeric_limits<A>::infinity() > -std::numeric_limits<B>::infinity()));
    BOOST_TEST(!(std::numeric_limits<A>::quiet_NaN() < B(1, 0)));
    BOOST_TEST(!(A(1, 0) < std::numeric_limits<B>::quiet_NaN()));
    BOOST_TEST(std::numeric_limits<A>::quiet_NaN() != B(1, 0));
    #endif
}

int main()
{
    test_limits<decimal32_t>();
    test_limits<decimal64_t>();
    test_limits<decimal128_t>();
    test_limits<decimal_fast32_t>();
    test_limits<decimal_fast64_t>();
    test_limits<decimal_fast128_t>();

    test_gaps<decimal32_t>();
    test_gaps<decimal64_t>();
    test_gaps<decimal128_t>();
    test_gaps<decimal_fast32_t>();
    test_gaps<decimal_fast64_t>();
    test_gaps<decimal_fast128_t>();

    test_wide_gaps<decimal32_t>();
    test_wide_gaps<decimal64_t>();
    test_wide_gaps<decimal128_t>();
    test_wide_gaps<decimal_fast32_t>();
    test_wide_gaps<decimal_fast64_t>();
    test_wide_gaps<decimal_fast128_t>();

    test_product_overflow();

    test_mixed_types<decimal32_t, decimal64_t>();
    test_mixed_types<decimal32_t, decimal128_t>();
    test_mixed_types<decimal64_t, decimal128_t>();
    test_mixed_types<decimal32_t, decimal_fast32_t>();
    test_mixed_types<decimal64_t, decimal_fast64_t>();
    test_mixed_types<decimal128_t, decimal_fast128_t>();
    test_mixed_types<decimal_fast32_t, decimal_fast64_t>();
    test_mixed_types<decimal_fast32_t, decimal64_t>();
    test_mixed_types<decimal_fast64_t, decimal128_t>();
    test_mixed_types<decimal64_t, decimal_fast128_t>();
    test_mixed_types<decimal_fast64_t, decimal_fast128_t>();
    test_mixed_types<decimal32_t, decimal_fast64_t>();
    test_mixed_types<decimal32_t, decimal_fast128_t>();
    test_mixed_types<decimal_fast32_t, decimal128_t>();
    test_mixed_types<decimal_fast32_t, decimal_fast128_t>();

    return boost::report_errors();
}
