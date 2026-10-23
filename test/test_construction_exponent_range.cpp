// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//
// A quantum exponent above the format maximum does not mean the value overflows: as long
// as the coefficient has room to absorb the excess, the same number is representable in a
// lower member of its cohort (IEEE 754-2008 3.5.1). Constructing such a value has to fold
// the overflow into the coefficient rather than return infinity.

#include <boost/decimal.hpp>
#include <boost/core/lightweight_test.hpp>
#include <iostream>
#include <limits>
#include <cstdint>

using namespace boost::decimal;

template <typename T>
struct significand_of
{
    using type = std::uint64_t;
};

template <>
struct significand_of<decimal128_t>
{
    using type = boost::int128::uint128_t;
};

template <>
struct significand_of<decimal_fast128_t>
{
    using type = boost::int128::uint128_t;
};

// The quantum exponent of the largest cohort member, and the digits of precision
template <typename T>
struct format_traits
{
    static constexpr int precision {std::numeric_limits<T>::digits10};
    static constexpr int max_quantum {std::numeric_limits<T>::max_exponent10 - precision + 1};
};

// Two cohort members of the same value reduce to the same pair, so this gives an oracle
// that does not go through the constructor being tested
template <typename Sig>
void reduce(Sig& sig, int& exp)
{
    while (sig != Sig{0} && sig % 10U == 0U)
    {
        sig /= 10U;
        ++exp;
    }
}

template <typename T>
void check_value(const char* name, typename significand_of<T>::type coeff, const int exp, const bool expect_finite)
{
    const T value {coeff, exp};

    if (!expect_finite)
    {
        // Fast math promises no non-finite values, so there is no overflow to observe
        #ifndef BOOST_DECIMAL_FAST_MATH
        if (!BOOST_TEST(isinf(value)))
        {
            std::cerr << "Expected overflow: " << name << " coefficient e" << exp << " -> " << value << std::endl;
        }
        #else
        static_cast<void>(name);
        #endif
        return;
    }

    if (!BOOST_TEST(isfinite(value)))
    {
        std::cerr << "Unexpected overflow: " << name << " coefficient e" << exp << std::endl;
        return;
    }

    int decoded_exp {};
    auto decoded_sig {frexp10(value, &decoded_exp)};
    reduce(decoded_sig, decoded_exp);

    auto expected_sig {coeff};
    auto expected_exp {exp};
    reduce(expected_sig, expected_exp);

    const bool ok {decoded_sig == static_cast<decltype(decoded_sig)>(expected_sig) && decoded_exp == expected_exp};
    if (!BOOST_TEST(ok))
    {
        std::cerr << "Wrong value: " << name << " built from coefficient e" << exp
                  << " -> " << value << std::endl;
    }
}

// Sweeps the whole grid of coefficient digit counts against the amount by which the
// requested quantum exponent overshoots the maximum
template <typename T>
void test_overflow_fold(const char* name)
{
    using sig_type = typename significand_of<T>::type;
    constexpr int precision {format_traits<T>::precision};
    constexpr int max_quantum {format_traits<T>::max_quantum};

    for (int digits {1}; digits <= precision; ++digits)
    {
        sig_type coeff {1};
        for (int i {1}; i < digits; ++i)
        {
            coeff *= 10U;
        }

        // A trailing 1 keeps the coefficient from being a power of ten, so the reduce
        // above cannot hide a wrong shift
        if (digits > 1)
        {
            coeff += sig_type{1};
        }

        for (int overshoot {1}; overshoot <= precision + 2; ++overshoot)
        {
            const bool representable {digits + overshoot <= precision};
            check_value<T>(name, coeff, max_quantum + overshoot, representable);
        }

        // At and below the maximum quantum exponent nothing needs folding
        check_value<T>(name, coeff, max_quantum, true);
        check_value<T>(name, coeff, max_quantum - 1, true);
    }
}

// The sign has to survive both the fold and the genuine overflow
template <typename T>
void test_sign_is_preserved(const char* name)
{
    constexpr int precision {format_traits<T>::precision};
    constexpr int max_quantum {format_traits<T>::max_quantum};

    const T folded {-1, max_quantum + 1};
    if (!BOOST_TEST(isfinite(folded) && signbit(folded) && folded < 0))
    {
        std::cerr << "Fold lost the sign: " << name << std::endl;
    }

    #ifndef BOOST_DECIMAL_FAST_MATH
    const T overflowed {-1, max_quantum + precision};
    if (!BOOST_TEST(isinf(overflowed) && signbit(overflowed)))
    {
        std::cerr << "Overflow lost the sign: " << name << std::endl;
    }
    #else
    static_cast<void>(precision);
    #endif
}

// The boundary between the largest finite value and infinity has to stay where it is
template <typename T>
void test_boundary(const char* name)
{
    constexpr int precision {format_traits<T>::precision};
    constexpr int max_quantum {format_traits<T>::max_quantum};

    // 1e(max_exponent10) is exactly representable, one decade past it is not
    const T largest_power {1, max_quantum + precision - 1};
    if (!BOOST_TEST(isfinite(largest_power)))
    {
        std::cerr << "Largest power of ten overflowed: " << name << std::endl;
    }

    #ifndef BOOST_DECIMAL_FAST_MATH
    const T past_largest {1, max_quantum + precision};
    if (!BOOST_TEST(isinf(past_largest)))
    {
        std::cerr << "Value past the maximum did not overflow: " << name << std::endl;
    }
    #endif

    BOOST_TEST(largest_power < std::numeric_limits<T>::max());
    BOOST_TEST(isfinite(std::numeric_limits<T>::max()));
}

// The underflow half of the same branch is untouched by the fold
template <typename T>
void test_underflow_unchanged(const char* name)
{
    static_cast<void>(name);

    constexpr int min_quantum {std::numeric_limits<T>::min_exponent10 - format_traits<T>::precision + 1};

    const T flushed {1, min_quantum - 400};
    const T zero {0, 0};

    BOOST_TEST_EQ(flushed, zero);
    BOOST_TEST(isfinite(std::numeric_limits<T>::min()));
    BOOST_TEST(std::numeric_limits<T>::min() > 0);
    BOOST_TEST(isfinite(std::numeric_limits<T>::denorm_min()));
    BOOST_TEST(std::numeric_limits<T>::denorm_min() > 0);

    // Only the compliant types hold subnormals; the fast types flush them to zero
    const T subnormal {1, min_quantum};
    BOOST_TEST_EQ(subnormal > 0, !detail::is_fast_type_v<T>);
}

// The pattern from the DPD round trip test, which was overflowing for decimal128_t
template <typename T>
void test_wide_significand_at_high_exponent(const char* name)
{
    constexpr int exp {std::numeric_limits<T>::max_exponent - std::numeric_limits<std::int64_t>::digits10 - 1};

    for (std::int64_t coeff {1}; coeff < std::numeric_limits<std::int64_t>::max() / 3; coeff *= 3)
    {
        const T value {coeff, exp};
        const bool expect_finite {detail::num_digits(coeff) + (exp - format_traits<T>::max_quantum)
                                  <= format_traits<T>::precision};

        if (!expect_finite)
        {
            continue;
        }

        if (BOOST_TEST(isfinite(value)))
        {
            BOOST_TEST_EQ(from_bid<T>(to_bid(value)), value);
            BOOST_TEST_EQ(from_dpd<T>(to_dpd(value)), value);
        }
        else
        {
            std::cerr << "Unexpected overflow at high exponent: " << name << std::endl;
        }
    }
}

// When the coefficient has room, only the excess goes into it, thus the quantum exponent is the maximum, the nearest
// to the requested one (IEEE 754-2019 5.2). A decimal128_t product of more than precision / 2 digits does not get here.
template <typename T>
void test_overflow_quantum(const char* name)
{
    using sig_type = typename significand_of<T>::type;
    constexpr int precision {format_traits<T>::precision};
    constexpr int max_quantum {format_traits<T>::max_quantum};

    sig_type coeff {1};
    for (int digits {1}; digits < precision; ++digits)
    {
        sig_type scaled {coeff};
        for (int overshoot {1}; digits + overshoot <= precision; ++overshoot)
        {
            scaled *= 10U;
            const T value {coeff, max_quantum + overshoot};
            const T product {T {coeff, max_quantum} * T {1, overshoot}};
            const bool value_ok {BOOST_TEST_EQ(value, T(scaled, max_quantum))};
            const bool quantum_ok {BOOST_TEST_EQ(quantexp(value), max_quantum)};
            const bool product_ok {(std::is_same<T, decimal128_t>::value && 2 * digits > precision) || (BOOST_TEST_EQ(product, value) && BOOST_TEST_EQ(quantexp(product), max_quantum))};
            if (!value_ok || !quantum_ok || !product_ok)
            {
                std::cerr << "Wrong quantum: " << name << " " << digits << " digits e" << max_quantum + overshoot << std::endl;
            }
        }

        coeff = coeff * 10U + 1U;
    }
}

template <typename T>
void test_all(const char* name)
{
    test_overflow_fold<T>(name);
    test_sign_is_preserved<T>(name);
    test_boundary<T>(name);
    test_underflow_unchanged<T>(name);
    test_wide_significand_at_high_exponent<T>(name);
}

int main()
{
    test_all<decimal32_t>("decimal32_t");
    test_all<decimal64_t>("decimal64_t");
    test_all<decimal128_t>("decimal128_t");
    test_overflow_quantum<decimal32_t>("decimal32_t");
    test_overflow_quantum<decimal64_t>("decimal64_t");
    test_overflow_quantum<decimal128_t>("decimal128_t");
    test_all<decimal_fast32_t>("decimal_fast32_t");
    test_all<decimal_fast64_t>("decimal_fast64_t");
    test_all<decimal_fast128_t>("decimal_fast128_t");

    return boost::report_errors();
}
