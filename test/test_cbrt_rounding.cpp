// Copyright 2026 Shen-Ta Hsieh
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include <boost/decimal.hpp>
#include <boost/core/lightweight_test.hpp>
#include <random>

using namespace boost::decimal;
using namespace boost::decimal::literals;

// fesetround has an effect only with the detection of constant evaluation
#ifndef BOOST_DECIMAL_NO_CONSTEVAL_DETECTION

// The order of the expected values in each row
constexpr rounding_mode modes[] {rounding_mode::fe_dec_to_nearest,
                                 rounding_mode::fe_dec_to_nearest_from_zero,
                                 rounding_mode::fe_dec_upward,
                                 rounding_mode::fe_dec_downward,
                                 rounding_mode::fe_dec_toward_zero};

// cbrt(-x) in a mode is -cbrt(x) in the mode that goes the other way
constexpr int mirror[] {0, 1, 3, 2, 4};

template <typename T>
struct row
{
    T x;
    T expected[5];
};

// Each fast type must give the same result as the type with the same precision
template <typename Fast, typename T, std::size_t N>
void test_table(const row<T> (&table)[N])
{
    for (int i {0}; i < 5; ++i)
    {
        fesetround(modes[i]);

        for (const auto& r : table)
        {
            BOOST_TEST_EQ(cbrt(r.x), r.expected[i]);
            BOOST_TEST_EQ(cbrt(-r.x), -r.expected[mirror[i]]);

            // The fast types have no subnormal values
            if (isnormal(r.x))
            {
                BOOST_TEST_EQ(cbrt(static_cast<Fast>(r.x)), static_cast<Fast>(r.expected[i]));
                BOOST_TEST_EQ(cbrt(static_cast<Fast>(-r.x)), static_cast<Fast>(-r.expected[mirror[i]]));
            }
        }
    }

    fesetround(rounding_mode::fe_dec_to_nearest);
}

// The rows hold exact cubes, roots near a midpoint or near a value of the type, a root that
// rounds up to 10^p, and the smallest and largest values. GMP mpz_rootrem gave the expected values.
void test_tables()
{
    constexpr row<decimal32_t> table32[] {
        {1000003e-6_DF, {1000001e-6_DF, 1000001e-6_DF, 1000001e-6_DF, 1000000e-6_DF, 1000000e-6_DF}},
        {8000006e-6_DF, {2000000e-6_DF, 2000000e-6_DF, 2000001e-6_DF, 2000000e-6_DF, 2000000e-6_DF}},
        {4913e-45_DF, {1700000e-20_DF, 1700000e-20_DF, 1700000e-20_DF, 1700000e-20_DF, 1700000e-20_DF}},
        {9999999e-7_DF, {1000000e-6_DF, 1000000e-6_DF, 1000000e-6_DF, 9999999e-7_DF, 9999999e-7_DF}},
        {4861090e-29_DF, {3649594e-14_DF, 3649594e-14_DF, 3649595e-14_DF, 3649594e-14_DF, 3649594e-14_DF}},
        {5305798e7_DF, {3757655e-2_DF, 3757655e-2_DF, 3757655e-2_DF, 3757654e-2_DF, 3757654e-2_DF}},
        {6810860e27_DF, {1895545e5_DF, 1895545e5_DF, 1895545e5_DF, 1895544e5_DF, 1895544e5_DF}},
        {3341587e-12_DF, {1495033e-8_DF, 1495033e-8_DF, 1495034e-8_DF, 1495033e-8_DF, 1495033e-8_DF}},
        {1e-101_DF, {2154435e-40_DF, 2154435e-40_DF, 2154435e-40_DF, 2154434e-40_DF, 2154434e-40_DF}},
        {9999999e90_DF, {2154435e26_DF, 2154435e26_DF, 2154435e26_DF, 2154434e26_DF, 2154434e26_DF}},
    };

    constexpr row<decimal64_t> table64[] {
        {1000000000000003e-15_DD, {1000000000000001e-15_DD, 1000000000000001e-15_DD, 1000000000000001e-15_DD, 1000000000000000e-15_DD, 1000000000000000e-15_DD}},
        {8000000000000006e-15_DD, {2000000000000000e-15_DD, 2000000000000000e-15_DD, 2000000000000001e-15_DD, 2000000000000000e-15_DD, 2000000000000000e-15_DD}},
        {4913e-45_DD, {1700000000000000e-29_DD, 1700000000000000e-29_DD, 1700000000000000e-29_DD, 1700000000000000e-29_DD, 1700000000000000e-29_DD}},
        {9999999999999999e-16_DD, {1000000000000000e-15_DD, 1000000000000000e-15_DD, 1000000000000000e-15_DD, 9999999999999999e-16_DD, 9999999999999999e-16_DD}},
        {7085277855655817e18_DD, {1920667976462010e-4_DD, 1920667976462010e-4_DD, 1920667976462011e-4_DD, 1920667976462010e-4_DD, 1920667976462010e-4_DD}},
        {9390795588510859e-22_DD, {9792662697721637e-18_DD, 9792662697721637e-18_DD, 9792662697721637e-18_DD, 9792662697721636e-18_DD, 9792662697721636e-18_DD}},
        {1377124219302522e20_DD, {5164057136204600e-4_DD, 5164057136204600e-4_DD, 5164057136204600e-4_DD, 5164057136204599e-4_DD, 5164057136204599e-4_DD}},
        {7417273165421645e-15_DD, {1950210204420324e-15_DD, 1950210204420324e-15_DD, 1950210204420325e-15_DD, 1950210204420324e-15_DD, 1950210204420324e-15_DD}},
        {1e-398_DD, {2154434690031884e-148_DD, 2154434690031884e-148_DD, 2154434690031884e-148_DD, 2154434690031883e-148_DD, 2154434690031883e-148_DD}},
        {9999999999999999e369_DD, {2154434690031884e113_DD, 2154434690031884e113_DD, 2154434690031884e113_DD, 2154434690031883e113_DD, 2154434690031883e113_DD}},
    };

    const row<decimal128_t> table128[] {
        {1000000000000000000000000000000003e-33_DL, {1000000000000000000000000000000001e-33_DL, 1000000000000000000000000000000001e-33_DL, 1000000000000000000000000000000001e-33_DL, 1000000000000000000000000000000000e-33_DL, 1000000000000000000000000000000000e-33_DL}},
        {8000000000000000000000000000000006e-33_DL, {2000000000000000000000000000000000e-33_DL, 2000000000000000000000000000000000e-33_DL, 2000000000000000000000000000000001e-33_DL, 2000000000000000000000000000000000e-33_DL, 2000000000000000000000000000000000e-33_DL}},
        {4913e-45_DL, {1700000000000000000000000000000000e-47_DL, 1700000000000000000000000000000000e-47_DL, 1700000000000000000000000000000000e-47_DL, 1700000000000000000000000000000000e-47_DL, 1700000000000000000000000000000000e-47_DL}},
        {9999999999999999999999999999999999e-34_DL, {1000000000000000000000000000000000e-33_DL, 1000000000000000000000000000000000e-33_DL, 1000000000000000000000000000000000e-33_DL, 9999999999999999999999999999999999e-34_DL, 9999999999999999999999999999999999e-34_DL}},
        {1795810271052573571919401089952576e2_DL, {5641832007856239919461722196442353e-22_DL, 5641832007856239919461722196442353e-22_DL, 5641832007856239919461722196442354e-22_DL, 5641832007856239919461722196442353e-22_DL, 5641832007856239919461722196442353e-22_DL}},
        {3878134764831116754599870380406953e-8_DL, {3384861986473590309793963209415330e-25_DL, 3384861986473590309793963209415330e-25_DL, 3384861986473590309793963209415330e-25_DL, 3384861986473590309793963209415329e-25_DL, 3384861986473590309793963209415329e-25_DL}},
        {8412468202700558666833399585980310e27_DL, {2033797981163578516164548816660236e-13_DL, 2033797981163578516164548816660236e-13_DL, 2033797981163578516164548816660236e-13_DL, 2033797981163578516164548816660235e-13_DL, 2033797981163578516164548816660235e-13_DL}},
        {7026831854968835617543580324618774e0_DL, {1915372232765098551718880927835536e-22_DL, 1915372232765098551718880927835536e-22_DL, 1915372232765098551718880927835537e-22_DL, 1915372232765098551718880927835536e-22_DL, 1915372232765098551718880927835536e-22_DL}},
        {1e-6176_DL, {2154434690031883721759293566519350e-2092_DL, 2154434690031883721759293566519350e-2092_DL, 2154434690031883721759293566519351e-2092_DL, 2154434690031883721759293566519350e-2092_DL, 2154434690031883721759293566519350e-2092_DL}},
        {9999999999999999999999999999999999e6111_DL, {2154434690031883721759293566519350e2015_DL, 2154434690031883721759293566519350e2015_DL, 2154434690031883721759293566519351e2015_DL, 2154434690031883721759293566519350e2015_DL, 2154434690031883721759293566519350e2015_DL}},
    };

    test_table<decimal_fast32_t>(table32);
    test_table<decimal_fast64_t>(table64);
    test_table<decimal_fast128_t>(table128);
}

// decimal128_t holds the cube of each 8-digit value exactly, thus it can compare the
// root of a decimal32_t value with the bounds of its interval of correct results.
template <typename T>
void test_bounds()
{
    std::mt19937_64 gen {42};
    std::uniform_int_distribution<std::uint32_t> sig_dist {1U, 9999999U};
    std::uniform_int_distribution<int> exp_dist {-95, 90};

    for (int n {0}; n < 2000; ++n)
    {
        const T x {sig_dist(gen), exp_dist(gen)};
        const decimal128_t xw {static_cast<decimal128_t>(x)};
        T roots[5] {};

        for (int i {0}; i < 5; ++i)
        {
            fesetround(modes[i]);

            const T r {cbrt(x)};
            roots[i] = r;
            const decimal128_t rw {static_cast<decimal128_t>(r)};
            const decimal128_t lo {static_cast<decimal128_t>(nextafter(r, T{0}))};
            const decimal128_t hi {static_cast<decimal128_t>(nextafter(r, std::numeric_limits<T>::infinity()))};

            if (i < 2)
            {
                const decimal128_t mlo {(lo + rw) / 2};
                const decimal128_t mhi {(rw + hi) / 2};
                BOOST_TEST(mlo * mlo * mlo < xw && xw < mhi * mhi * mhi);
            }
            else if (i == 2)
            {
                BOOST_TEST(lo * lo * lo < xw && xw <= rw * rw * rw);
            }
            else
            {
                BOOST_TEST(rw * rw * rw <= xw && xw < hi * hi * hi);
            }

        }

        for (int i {0}; i < 5; ++i)
        {
            fesetround(modes[i]);
            BOOST_TEST_EQ(cbrt(-x), -roots[mirror[i]]);
        }
    }

    fesetround(rounding_mode::fe_dec_to_nearest);
}

int main()
{
    test_tables();
    test_bounds<decimal32_t>();
    test_bounds<decimal_fast32_t>();

    return boost::report_errors();
}

#else

int main()
{
    return 0;
}

#endif
