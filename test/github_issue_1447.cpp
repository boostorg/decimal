// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//
// https://github.com/boostorg/decimal/issues/1447
//
// The single word branch of impl::div_mod left the words above the lowest one with the
// words of the dividend, thus a remainder of a shift of 1 to 9 digits came out wrong.

#include <boost/decimal.hpp>
#include <boost/decimal/detail/u256.hpp>
#include <boost/core/lightweight_test.hpp>

using namespace boost::decimal;
using namespace boost::decimal::literals;

// 10^shift divides this dividend with no remainder for every shift from 1 to 9, and the
// dividend stays above 2^32, thus more than one word holds it.
void test_exact_division()
{
    const detail::u256 ten {UINT64_C(10)};

    detail::u256 dividend {UINT64_C(9373841430626804)};
    detail::u256 divisor {UINT64_C(1)};

    for (int shift {1}; shift <= 9; ++shift)
    {
        dividend = dividend * ten;
        divisor = divisor * ten;

        const auto result {detail::impl::div_mod(dividend, divisor)};

        BOOST_TEST(result.remainder == detail::u256{UINT64_C(0)});
        BOOST_TEST(result.quotient * divisor == dividend);
    }
}

// A remainder which is not zero must come through as itself. The quotient was correct
// before the correction, thus only the remainder shows the defect.
void test_inexact_division()
{
    const detail::u256 ten {UINT64_C(10)};

    const detail::u256 dividend {UINT64_C(9373841430626804007)};
    detail::u256 divisor {UINT64_C(1)};

    for (int shift {1}; shift <= 9; ++shift)
    {
        divisor = divisor * ten;

        const auto result {detail::impl::div_mod(dividend, divisor)};

        BOOST_TEST(result.remainder < divisor);
        BOOST_TEST(result.quotient * divisor + result.remainder == dividend);
    }
}

// A tie which is exact goes to the even last digit. The exact x*y+z here is
// -9.3738414306268045e-11, and the last digit of the value below the tie is 4.
void test_fma_exact_tie()
{
    const auto x {-6.645514330428021e+01_DD};
    const auto y {1.419781212105145e+02_DD};
    const auto z {9.435176391117113e+03_DD};

    BOOST_TEST_EQ(fma(x, y, z), -9.373841430626804e-11_DD);
    BOOST_TEST_EQ(fma(-x, y, -z), 9.373841430626804e-11_DD);
}

// The type holds the exact value of x*y+z of each triple below, and no rounding mode
// may move an exact result.
void test_fma_exact_results()
{
    // fesetround changes the mode only when the library can find a constant evaluation.
    #ifndef BOOST_DECIMAL_NO_CONSTEVAL_DETECTION

    const auto x64 {-3.794224970655122e+03_DD};
    const auto y64 {1.616581331280940e+03_DD};
    const auto z64 {6.133673254240982e+06_DD};
    const auto exact64 {-6.062798323197468e-08_DD};

    const auto x128 {8.631676437904993259081658098560000e+01_DL};
    const auto y128 {6.588286699825668036197412089736755e+00_DL};
    const auto z128 {-5.686795907304806584806883802739733e+02_DL};
    const auto exact128 {5.686751361164512835752045042445728e-27_DL};

    const auto x128f {8.631676437904993259081658098560000e+01_DLF};
    const auto y128f {6.588286699825668036197412089736755e+00_DLF};
    const auto z128f {-5.686795907304806584806883802739733e+02_DLF};
    const auto exact128f {5.686751361164512835752045042445728e-27_DLF};

    const rounding_mode modes[] {rounding_mode::fe_dec_downward,
                                 rounding_mode::fe_dec_upward,
                                 rounding_mode::fe_dec_toward_zero,
                                 rounding_mode::fe_dec_to_nearest,
                                 rounding_mode::fe_dec_to_nearest_from_zero};

    for (const auto mode : modes)
    {
        fesetround(mode);

        BOOST_TEST_EQ(fma(x64, y64, z64), exact64);
        BOOST_TEST_EQ(fma(-x64, y64, -z64), -exact64);
        BOOST_TEST_EQ(fma(x128, y128, z128), exact128);
        BOOST_TEST_EQ(fma(-x128, y128, -z128), -exact128);
        BOOST_TEST_EQ(fma(x128f, y128f, z128f), exact128f);
        BOOST_TEST_EQ(fma(-x128f, y128f, -z128f), -exact128f);
    }

    fesetround(rounding_mode::fe_dec_to_nearest);

    #endif
}

int main()
{
    test_exact_division();
    test_inexact_division();
    test_fma_exact_tie();
    test_fma_exact_results();

    return boost::report_errors();
}
