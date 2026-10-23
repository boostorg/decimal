// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//
// See: https://github.com/boostorg/decimal/issues/1424

#include <boost/decimal.hpp>
#include <boost/core/lightweight_test.hpp>
#include <iostream>
#include <limits>
#include <cstdint>

using namespace boost::decimal;

// Quantum exponents that are valid for every decimal type, used to build the members
// of the zero cohort that must all compare equal to each other (IEEE 754-2008 3.5.1)
static constexpr int cohort_exponents[] {0, 1, -1, 2, -2, 6, -6, 20, -20, 60, -60};

// Every relational operator, in both argument orders, must agree that two numerically
// equal values are equal (IEEE 754-2008 5.11)
template <typename T, typename U>
void check_equal(const char* context, const T lhs, const U rhs)
{
    const bool eq {lhs == rhs};
    const bool ne {lhs != rhs};
    const bool lt {lhs < rhs};
    const bool gt {lhs > rhs};
    const bool le {lhs <= rhs};
    const bool ge {lhs >= rhs};

    const bool r_eq {rhs == lhs};
    const bool r_ne {rhs != lhs};
    const bool r_lt {rhs < lhs};
    const bool r_gt {rhs > lhs};
    const bool r_le {rhs <= lhs};
    const bool r_ge {rhs >= lhs};

    const bool ok {eq && !ne && !lt && !gt && le && ge &&
                   r_eq && !r_ne && !r_lt && !r_gt && r_le && r_ge};

    if (!ok)
    {
        std::cerr << "Expected equal: " << context
                  << "\n  lhs OP rhs -> ==:" << eq << " !=:" << ne << " <:" << lt
                  << " >:" << gt << " <=:" << le << " >=:" << ge
                  << "\n  rhs OP lhs -> ==:" << r_eq << " !=:" << r_ne << " <:" << r_lt
                  << " >:" << r_gt << " <=:" << r_le << " >=:" << r_ge << std::endl;
    }

    BOOST_TEST(ok);
}

// Same contract as check_equal, plus the three way comparison which only exists
// between two decimal types
template <typename T, typename U>
void check_equal_decimals(const char* context, const T lhs, const U rhs)
{
    check_equal(context, lhs, rhs);

    #ifdef BOOST_DECIMAL_HAS_SPACESHIP_OPERATOR
    BOOST_TEST((lhs <=> rhs) == std::partial_ordering::equivalent);
    BOOST_TEST((rhs <=> lhs) == std::partial_ordering::equivalent);
    #endif
}

// Guards against the zero handling swallowing genuine ordering
template <typename T, typename U>
void check_less(const char* context, const T lo, const U hi)
{
    const bool ok {(lo < hi) && !(hi < lo) &&
                   (lo <= hi) && !(hi <= lo) &&
                   (hi > lo) && !(lo > hi) &&
                   (hi >= lo) && !(lo >= hi) &&
                   (lo != hi) && !(lo == hi)};

    if (!ok)
    {
        std::cerr << "Expected strictly less: " << context << std::endl;
    }

    BOOST_TEST(ok);
}

// The reproducer exactly as filed: a BSON/MongoDB decimal128 zero is stored with
// coefficient 0 and quantum exponent 0, which is biased exponent 6176
void test_issue_reproducer()
{
    boost::int128::uint128_t stored_zero {};
    stored_zero.high = UINT64_C(0x3040000000000000);
    stored_zero.low = UINT64_C(0x0000000000000000);

    const decimal128_t decoded {from_bid_d128(stored_zero)};
    const decimal128_t zero {0, 0};

    check_equal_decimals("issue 1424 reproducer", decoded, zero);
    check_equal("issue 1424 reproducer vs int", decoded, 0);

    // The other extreme of the same cohort: 0E-6176, which is biased exponent 0
    boost::int128::uint128_t min_exp_zero {};
    min_exp_zero.high = UINT64_C(0x0000000000000000);
    min_exp_zero.low = UINT64_C(0x0000000000000000);

    const decimal128_t min_decoded {from_bid_d128(min_exp_zero)};
    check_equal_decimals("issue 1424 0E-6176", min_decoded, zero);
    check_equal_decimals("issue 1424 0E-6176 vs 0E0", min_decoded, decoded);
    check_equal("issue 1424 0E-6176 vs int", min_decoded, 0);
}

// Every member of the zero cohort produced by the constructor, and its negation,
// compares equal to every other member
template <typename T>
void test_constructed_cohorts(const char* name)
{
    const T reference {0, 0};
    const T neg_reference {-reference};

    for (const auto lhs_exp : cohort_exponents)
    {
        const T pos {0, lhs_exp};
        const T neg {-T{0, lhs_exp}};

        check_equal_decimals(name, reference, pos);
        check_equal_decimals(name, reference, neg);
        check_equal_decimals(name, neg_reference, pos);
        check_equal_decimals(name, neg_reference, neg);
        check_equal(name, pos, 0);
        check_equal(name, neg, 0);
        check_equal(name, pos, 0U);
        check_equal(name, neg, 0LL);

        for (const auto rhs_exp : cohort_exponents)
        {
            check_equal_decimals(name, pos, T{0, rhs_exp});
            check_equal_decimals(name, pos, -T{0, rhs_exp});
            check_equal_decimals(name, neg, T{0, rhs_exp});
            check_equal_decimals(name, neg, -T{0, rhs_exp});
        }
    }
}

// The same cohort members, but round tripped through the interchange encodings that
// a database or wire format would hand us
template <typename T>
void test_encoded_cohorts(const char* name)
{
    const T reference {0, 0};

    for (const auto exp : cohort_exponents)
    {
        const T pos {0, exp};
        const T neg {-T{0, exp}};

        check_equal_decimals(name, reference, from_bid<T>(to_bid(pos)));
        check_equal_decimals(name, reference, from_bid<T>(to_bid(neg)));
        check_equal(name, from_bid<T>(to_bid(pos)), 0);
        check_equal(name, from_bid<T>(to_bid(neg)), 0);

        // to_dpd emits BID under BOOST_DECIMAL_FAST_MATH, because the per type isfinite
        // reports false there, so the DPD round trip is not usable in that configuration
        #ifndef BOOST_DECIMAL_FAST_MATH
        check_equal_decimals(name, reference, from_dpd<T>(to_dpd(pos)));
        check_equal_decimals(name, reference, from_dpd<T>(to_dpd(neg)));
        check_equal(name, from_dpd<T>(to_dpd(pos)), 0);
        check_equal(name, from_dpd<T>(to_dpd(neg)), 0);
        #endif
    }
}

// Raw BID zero encodings across the whole biased exponent range of each format.
// These are the patterns an external producer is free to emit.
void test_raw_bid_zeros()
{
    // decimal32_t: sign in bit 31, an 8 bit biased exponent in bits 30-23
    for (const std::uint32_t biased : {UINT32_C(0), UINT32_C(1), UINT32_C(101), UINT32_C(190), UINT32_C(191)})
    {
        const std::uint32_t bits {biased << 23U};
        const std::uint32_t neg_bits {bits | UINT32_C(0x80000000)};

        check_equal_decimals("raw bid32", from_bid<decimal32_t>(bits), decimal32_t{0, 0});
        check_equal_decimals("raw bid32 negative", from_bid<decimal32_t>(neg_bits), decimal32_t{0, 0});
        check_equal_decimals("raw bid32 fast", from_bid<decimal_fast32_t>(bits), decimal_fast32_t{0, 0});
        check_equal_decimals("raw bid32 fast negative", from_bid<decimal_fast32_t>(neg_bits), decimal_fast32_t{0, 0});
        check_equal("raw bid32 vs int", from_bid<decimal32_t>(bits), 0);
        check_equal("raw bid32 negative vs int", from_bid<decimal32_t>(neg_bits), 0);
    }

    // decimal64_t: sign in bit 63, a 10 bit biased exponent in bits 62-53
    for (const std::uint64_t biased : {UINT64_C(0), UINT64_C(1), UINT64_C(398), UINT64_C(766), UINT64_C(767)})
    {
        const std::uint64_t bits {biased << 53U};
        const std::uint64_t neg_bits {bits | UINT64_C(0x8000000000000000)};

        check_equal_decimals("raw bid64", from_bid<decimal64_t>(bits), decimal64_t{0, 0});
        check_equal_decimals("raw bid64 negative", from_bid<decimal64_t>(neg_bits), decimal64_t{0, 0});
        check_equal_decimals("raw bid64 fast", from_bid<decimal_fast64_t>(bits), decimal_fast64_t{0, 0});
        check_equal_decimals("raw bid64 fast negative", from_bid<decimal_fast64_t>(neg_bits), decimal_fast64_t{0, 0});
        check_equal("raw bid64 vs int", from_bid<decimal64_t>(bits), 0);
        check_equal("raw bid64 negative vs int", from_bid<decimal64_t>(neg_bits), 0);
    }

    // decimal128_t: sign in bit 127, a 14 bit biased exponent in bits 126-113
    for (const std::uint64_t biased : {UINT64_C(0), UINT64_C(1), UINT64_C(6176), UINT64_C(12286), UINT64_C(12287)})
    {
        boost::int128::uint128_t bits {};
        bits.high = biased << 49U;
        bits.low = UINT64_C(0);

        boost::int128::uint128_t neg_bits {bits};
        neg_bits.high |= UINT64_C(0x8000000000000000);

        check_equal_decimals("raw bid128", from_bid<decimal128_t>(bits), decimal128_t{0, 0});
        check_equal_decimals("raw bid128 negative", from_bid<decimal128_t>(neg_bits), decimal128_t{0, 0});
        check_equal_decimals("raw bid128 fast", from_bid<decimal_fast128_t>(bits), decimal_fast128_t{0, 0});
        check_equal_decimals("raw bid128 fast negative", from_bid<decimal_fast128_t>(neg_bits), decimal_fast128_t{0, 0});
        check_equal("raw bid128 vs int", from_bid<decimal128_t>(bits), 0);
        check_equal("raw bid128 negative vs int", from_bid<decimal128_t>(neg_bits), 0);
    }
}

// Zeros of different widths, and of the IEEE versus fast flavours, also compare equal
template <typename T, typename U>
void test_mixed_zeros(const char* name)
{
    for (const auto lhs_exp : cohort_exponents)
    {
        const T pos {0, lhs_exp};
        const T neg {-T{0, lhs_exp}};

        for (const auto rhs_exp : cohort_exponents)
        {
            const U other_pos {0, rhs_exp};
            const U other_neg {-U{0, rhs_exp}};

            check_equal_decimals(name, pos, other_pos);
            check_equal_decimals(name, pos, other_neg);
            check_equal_decimals(name, neg, other_pos);
            check_equal_decimals(name, neg, other_neg);
        }
    }
}

// A zero of either sign still orders correctly against non-zero values
template <typename T>
void test_ordering_is_preserved(const char* name)
{
    const T pos_zero {0, 0};
    const T neg_zero {-pos_zero};
    const T pos_zero_cohort {0, 20};
    const T neg_zero_cohort {-T{0, -20}};

    const T tiny {1, -20};
    const T neg_tiny {-tiny};
    const T one {1, 0};
    const T neg_one {-one};

    check_less(name, pos_zero, tiny);
    check_less(name, neg_zero, tiny);
    check_less(name, pos_zero_cohort, tiny);
    check_less(name, neg_zero_cohort, tiny);
    check_less(name, neg_tiny, pos_zero);
    check_less(name, neg_tiny, neg_zero);
    check_less(name, neg_tiny, pos_zero_cohort);
    check_less(name, neg_tiny, neg_zero_cohort);
    check_less(name, neg_one, pos_zero);
    check_less(name, neg_one, neg_zero);
    check_less(name, pos_zero, one);
    check_less(name, neg_zero, one);
    check_less(name, neg_one, one);

    check_less(name, pos_zero, 1);
    check_less(name, neg_zero, 1);
    check_less(name, neg_one, 0);
    check_less(name, neg_one, 1);
    check_less(name, -1, pos_zero);
    check_less(name, -1, neg_zero);

    // Cohorts of a non-zero value are unaffected
    check_equal_decimals(name, one, T{10000, -4});
    check_less(name, T{10000, -4}, T{2, 0});
    check_less(name, T{-10000, -4}, pos_zero);
    check_less(name, T{-10000, -4}, neg_zero);

    // The non-finite ordering that shares these code paths is unaffected
    #ifndef BOOST_DECIMAL_FAST_MATH
    const T pos_inf {std::numeric_limits<T>::infinity()};
    const T neg_inf {-pos_inf};
    const T quiet_nan {std::numeric_limits<T>::quiet_NaN()};

    check_less(name, pos_zero, pos_inf);
    check_less(name, neg_zero, pos_inf);
    check_less(name, neg_inf, pos_zero);
    check_less(name, neg_inf, neg_zero);
    check_less(name, neg_inf, pos_inf);

    BOOST_TEST(!(pos_inf < pos_inf));
    BOOST_TEST(!(neg_inf < neg_inf));
    BOOST_TEST(!(quiet_nan < pos_zero));
    BOOST_TEST(!(pos_zero < quiet_nan));
    BOOST_TEST(!(quiet_nan == pos_zero));
    BOOST_TEST(quiet_nan != pos_zero);
    #endif
}

// The same non-finite ordering, but between two different decimal types
template <typename T, typename U>
void test_mixed_ordering_is_preserved(const char* name)
{
    const T pos_zero {0, 0};
    const T neg_zero {-pos_zero};

    check_less(name, neg_zero, U{1, 0});
    check_less(name, U{-1, 0}, neg_zero);
    check_less(name, pos_zero, U{1, -20});
    check_less(name, U{-1, -20}, pos_zero);

    #ifndef BOOST_DECIMAL_FAST_MATH
    const U pos_inf {std::numeric_limits<U>::infinity()};
    const U neg_inf {-pos_inf};
    const U quiet_nan {std::numeric_limits<U>::quiet_NaN()};

    check_less(name, pos_zero, pos_inf);
    check_less(name, neg_zero, pos_inf);
    check_less(name, neg_inf, pos_zero);
    check_less(name, neg_inf, neg_zero);

    BOOST_TEST(!(quiet_nan < pos_zero));
    BOOST_TEST(!(pos_zero < quiet_nan));
    BOOST_TEST(!(quiet_nan == neg_zero));
    #endif
}

int main()
{
    test_issue_reproducer();

    test_constructed_cohorts<decimal32_t>("decimal32_t cohort");
    test_constructed_cohorts<decimal64_t>("decimal64_t cohort");
    test_constructed_cohorts<decimal128_t>("decimal128_t cohort");
    test_constructed_cohorts<decimal_fast32_t>("decimal_fast32_t cohort");
    test_constructed_cohorts<decimal_fast64_t>("decimal_fast64_t cohort");
    test_constructed_cohorts<decimal_fast128_t>("decimal_fast128_t cohort");

    test_encoded_cohorts<decimal32_t>("decimal32_t encoded cohort");
    test_encoded_cohorts<decimal64_t>("decimal64_t encoded cohort");
    test_encoded_cohorts<decimal128_t>("decimal128_t encoded cohort");
    test_encoded_cohorts<decimal_fast32_t>("decimal_fast32_t encoded cohort");
    test_encoded_cohorts<decimal_fast64_t>("decimal_fast64_t encoded cohort");
    test_encoded_cohorts<decimal_fast128_t>("decimal_fast128_t encoded cohort");

    test_raw_bid_zeros();

    test_mixed_zeros<decimal32_t, decimal64_t>("d32 vs d64 zero");
    test_mixed_zeros<decimal32_t, decimal128_t>("d32 vs d128 zero");
    test_mixed_zeros<decimal64_t, decimal128_t>("d64 vs d128 zero");
    test_mixed_zeros<decimal32_t, decimal_fast32_t>("d32 vs fast32 zero");
    test_mixed_zeros<decimal64_t, decimal_fast64_t>("d64 vs fast64 zero");
    test_mixed_zeros<decimal128_t, decimal_fast128_t>("d128 vs fast128 zero");
    test_mixed_zeros<decimal_fast32_t, decimal_fast64_t>("fast32 vs fast64 zero");
    test_mixed_zeros<decimal_fast64_t, decimal_fast128_t>("fast64 vs fast128 zero");

    test_ordering_is_preserved<decimal32_t>("decimal32_t ordering");
    test_ordering_is_preserved<decimal64_t>("decimal64_t ordering");
    test_ordering_is_preserved<decimal128_t>("decimal128_t ordering");
    test_ordering_is_preserved<decimal_fast32_t>("decimal_fast32_t ordering");
    test_ordering_is_preserved<decimal_fast64_t>("decimal_fast64_t ordering");
    test_ordering_is_preserved<decimal_fast128_t>("decimal_fast128_t ordering");

    test_mixed_ordering_is_preserved<decimal32_t, decimal64_t>("d32 vs d64 ordering");
    test_mixed_ordering_is_preserved<decimal64_t, decimal128_t>("d64 vs d128 ordering");
    test_mixed_ordering_is_preserved<decimal32_t, decimal_fast32_t>("d32 vs fast32 ordering");
    test_mixed_ordering_is_preserved<decimal_fast64_t, decimal_fast128_t>("fast64 vs fast128 ordering");

    return boost::report_errors();
}
