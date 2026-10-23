// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//
// https://github.com/boostorg/decimal/issues/1440
//
// fma lost the addend when the product's exponent sat far below the addend's. The add
// kernels size their alignment shift with max_shift, which was computed from the result
// type's precision. That is right for operator+ and operator-, whose operands carry the
// result's precision, but fma hands over the wider promoted components, so a shift that
// max_shift admitted could still overflow the promoted significand and the multiply
// silently wrapped.

#include <boost/decimal.hpp>
#include <boost/core/lightweight_test.hpp>
#include <limits>

using namespace boost::decimal;

// The band the report identified: 1e-24 to 1e-30 for decimal32_t, 1e-45 to 1e-61 for
// decimal64_t. Once x sits below the last digit of 1, which is 10^-(digits-1), the whole
// answer is 1; above that the sum keeps x, so only the agreement check applies there.
template <typename T>
void test_addend_survives()
{
    constexpr auto digits {std::numeric_limits<T>::digits10};
    const T one {1, 0};

    for (int exponent {-1}; exponent >= -80; --exponent)
    {
        const T x {1, exponent};

        BOOST_TEST_EQ(fma(x, one, one), x * one + one);

        if (exponent <= -digits)
        {
            if (!BOOST_TEST_EQ(fma(x, one, one), one))
            {
                std::cerr << "fma(1e" << exponent << ", 1, 1) lost the addend" << std::endl;
            }
        }
    }
}

// The report's one-digit sweep: fma must agree with the plain expression everywhere here,
// because the product never needs more digits than the format holds.
template <typename T>
void test_matches_plain_expression()
{
    for (int exponent {-1}; exponent >= -80; --exponent)
    {
        for (int x_sig {1}; x_sig <= 9; ++x_sig)
        {
            const T x {x_sig, exponent};

            for (int y_sig {1}; y_sig <= 9; ++y_sig)
            {
                const T y {y_sig, 0};

                for (int z_sig {1}; z_sig <= 9; ++z_sig)
                {
                    const T z {z_sig, 0};
                    BOOST_TEST_EQ(fma(x, y, z), x * y + z);
                }
            }
        }
    }
}

// A wide spread with full-width factors, so the exact product needs about twice the
// digits of the format and the alignment shift is at its largest. The threshold is where
// the product drops below z's last digit; both factors carry digits, so it is not simply
// the exponent of x. Values confirmed against Decimal.fma at precision 7 and 16.
template <typename T>
void test_wide_spread()
{
    constexpr auto digits {std::numeric_limits<T>::digits10};
    constexpr auto vanishes_at {digits == 7 ? -20 : -47};
    const T full {digits == 7 ? 9999999LL : 9999999999999999LL, 0};
    const T z {1, 0};

    for (int exponent {-1}; exponent >= -70; --exponent)
    {
        const T x {digits == 7 ? 1234567LL : 1234567890123456LL, exponent};

        BOOST_TEST_EQ(fma(x, full, z), x * full + z);

        // Once the product sits below z's last digit, z is the whole answer
        if (exponent <= vanishes_at)
        {
            if (!BOOST_TEST_EQ(fma(x, full, z), z))
            {
                std::cerr << "exponent " << exponent << std::endl;
            }
        }
    }
}

int main()
{
    test_addend_survives<decimal32_t>();
    test_addend_survives<decimal_fast32_t>();
    test_addend_survives<decimal64_t>();
    test_addend_survives<decimal_fast64_t>();
    test_addend_survives<decimal128_t>();
    test_addend_survives<decimal_fast128_t>();

    test_matches_plain_expression<decimal32_t>();
    test_matches_plain_expression<decimal64_t>();

    test_wide_spread<decimal32_t>();
    test_wide_spread<decimal64_t>();

    return boost::report_errors();
}
