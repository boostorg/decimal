// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//
// IEEE 754-2019 4.3 and 5.4.1: sqrt is correctly rounded in the current mode. The 32 and 64
// bit kernels truncated the root, an odd exponent multiplied by a rounded sqrt(10), and no
// kernel read the mode.

#include <boost/decimal.hpp>
#include <boost/core/lightweight_test.hpp>
#include <iostream>

using namespace boost::decimal;
using namespace boost::decimal::literals;

template <typename T> struct fast_of;
template <> struct fast_of<decimal32_t> { using type = decimal_fast32_t; };
template <> struct fast_of<decimal64_t> { using type = decimal_fast64_t; };
template <> struct fast_of<decimal128_t> { using type = decimal_fast128_t; };

// The value of the mode from the floor and the ceiling of the root. A root is never a tie.
template <typename T>
constexpr auto expected(const rounding_mode mode, const T floor, const T ceil, const bool near_up) -> T
{
    return mode == rounding_mode::fe_dec_upward ? ceil :
           mode == rounding_mode::fe_dec_toward_zero || mode == rounding_mode::fe_dec_downward ? floor :
           near_up ? ceil : floor;
}

template <typename T>
void check_mode(const T x, const rounding_mode mode, const T value)
{
    using F = typename fast_of<T>::type;
    fesetround(mode);
    BOOST_TEST_EQ(sqrt(x), value);
    BOOST_TEST_EQ(sqrt(static_cast<F>(x)), static_cast<F>(value));
}

// floor and ceil come from an integer square root at the precision of the type
template <typename T>
void check(const T x, const T floor, const T ceil, const bool near_up)
{
    #ifdef BOOST_DECIMAL_NO_CONSTEVAL_DETECTION
    // fesetround has no effect here, thus only the compile-time mode is tested
    check_mode(x, _boost_decimal_global_rounding_mode, expected(_boost_decimal_global_rounding_mode, floor, ceil, near_up));
    #else
    check_mode(x, rounding_mode::fe_dec_to_nearest, expected(rounding_mode::fe_dec_to_nearest, floor, ceil, near_up));
    check_mode(x, rounding_mode::fe_dec_to_nearest_from_zero, expected(rounding_mode::fe_dec_to_nearest_from_zero, floor, ceil, near_up));
    check_mode(x, rounding_mode::fe_dec_toward_zero, floor);
    check_mode(x, rounding_mode::fe_dec_downward, floor);
    check_mode(x, rounding_mode::fe_dec_upward, ceil);
    #endif
}

template <typename T>
void check(const int n, const T floor, const T ceil, const bool near_up)
{
    check(T {n, 0}, floor, ceil, near_up);
}

int main()
{
    constexpr auto mode {_boost_decimal_global_rounding_mode};
    static_assert(sqrt(decimal32_t {11, 0}) == expected(mode, decimal32_t {3316624, -6}, decimal32_t {3316625, -6}, true), "");
    static_assert(sqrt(decimal64_t {26, 0}) == expected(mode, decimal64_t {UINT64_C(5099019513592784), -15}, decimal64_t {UINT64_C(5099019513592785), -15}, true), "");

    // A failure prints every digit of the two values
    std::cerr.precision(std::numeric_limits<decimal128_t>::digits10);

    check<decimal32_t>(2, 1.414213_df, 1.414214_df, true);
    check<decimal32_t>(3, 1.732050_df, 1.732051_df, true);
    check<decimal32_t>(4, 2_df, 2_df, false);
    check<decimal32_t>(5, 2.236067_df, 2.236068_df, true);
    check<decimal32_t>(6, 2.449489_df, 2.449490_df, true);
    check<decimal32_t>(7, 2.645751_df, 2.645752_df, false);
    check<decimal32_t>(8, 2.828427_df, 2.828428_df, false);
    check<decimal32_t>(9, 3_df, 3_df, false);
    check<decimal32_t>(10, 3.162277_df, 3.162278_df, true);
    check<decimal32_t>(11, 3.316624_df, 3.316625_df, true);
    check<decimal32_t>(12, 3.464101_df, 3.464102_df, true);
    check<decimal32_t>(13, 3.605551_df, 3.605552_df, false);
    check<decimal32_t>(15, 3.872983_df, 3.872984_df, false);
    check<decimal32_t>(17, 4.123105_df, 4.123106_df, true);
    check<decimal32_t>(19, 4.358898_df, 4.358899_df, true);
    check<decimal32_t>(26, 5.099019_df, 5.099020_df, true);
    check<decimal32_t>(29, 5.385164_df, 5.385165_df, true);
    check<decimal32_t>(48, 6.928203_df, 6.928204_df, false);
    check<decimal32_t>(50, 7.071067_df, 7.071068_df, true);
    check<decimal32_t>(61, 7.810249_df, 7.810250_df, true);
    check<decimal32_t>(73, 8.544003_df, 8.544004_df, true);
    check<decimal32_t>(83, 9.110433_df, 9.110434_df, true);
    check<decimal32_t>(97, 9.848857_df, 9.848858_df, true);
    check<decimal32_t>(99, 9.949874_df, 9.949875_df, false);
    check<decimal32_t>(100, 10_df, 10_df, false);
    check<decimal32_t>(101, 10.04987_df, 10.04988_df, true);
    check<decimal32_t>(500, 22.36067_df, 22.36068_df, true);
    check<decimal32_t>(999, 31.60696_df, 31.60697_df, false);
    check<decimal32_t>(1000, 31.62277_df, 31.62278_df, true);
    // The Newton step lands two above the floor here, and one step down was not enough
    check(6.028760e-17_df, 7.764508e-9_df, 7.764509e-9_df, true);
    check(4.472279e-67_df, 6.687509e-34_df, 6.687510e-34_df, true);
    check(5.428215e+70_df, 2.329852e+35_df, 2.329853e+35_df, true);

    check<decimal64_t>(2, 1.414213562373095_dd, 1.414213562373096_dd, false);
    check<decimal64_t>(3, 1.732050807568877_dd, 1.732050807568878_dd, false);
    check<decimal64_t>(4, 2_dd, 2_dd, false);
    check<decimal64_t>(5, 2.236067977499789_dd, 2.236067977499790_dd, true);
    check<decimal64_t>(6, 2.449489742783178_dd, 2.449489742783179_dd, false);
    check<decimal64_t>(7, 2.645751311064590_dd, 2.645751311064591_dd, true);
    check<decimal64_t>(8, 2.828427124746190_dd, 2.828427124746191_dd, false);
    check<decimal64_t>(9, 3_dd, 3_dd, false);
    check<decimal64_t>(10, 3.162277660168379_dd, 3.162277660168380_dd, false);
    check<decimal64_t>(11, 3.316624790355399_dd, 3.316624790355400_dd, true);
    check<decimal64_t>(12, 3.464101615137754_dd, 3.464101615137755_dd, true);
    check<decimal64_t>(13, 3.605551275463989_dd, 3.605551275463990_dd, false);
    check<decimal64_t>(15, 3.872983346207416_dd, 3.872983346207417_dd, true);
    check<decimal64_t>(17, 4.123105625617660_dd, 4.123105625617661_dd, true);
    check<decimal64_t>(19, 4.358898943540673_dd, 4.358898943540674_dd, true);
    check<decimal64_t>(26, 5.099019513592784_dd, 5.099019513592785_dd, true);
    check<decimal64_t>(29, 5.385164807134504_dd, 5.385164807134505_dd, false);
    check<decimal64_t>(48, 6.928203230275509_dd, 6.928203230275510_dd, false);
    check<decimal64_t>(50, 7.071067811865475_dd, 7.071067811865476_dd, false);
    check<decimal64_t>(61, 7.810249675906654_dd, 7.810249675906655_dd, false);
    check<decimal64_t>(73, 8.544003745317531_dd, 8.544003745317532_dd, false);
    check<decimal64_t>(83, 9.110433579144298_dd, 9.110433579144299_dd, true);
    check<decimal64_t>(97, 9.848857801796104_dd, 9.848857801796105_dd, true);
    check<decimal64_t>(99, 9.949874371066199_dd, 9.949874371066200_dd, true);
    check<decimal64_t>(100, 10_dd, 10_dd, false);
    check<decimal64_t>(101, 10.04987562112089_dd, 10.04987562112090_dd, false);
    check<decimal64_t>(500, 22.36067977499789_dd, 22.36067977499790_dd, true);
    check<decimal64_t>(999, 31.60696125855821_dd, 31.60696125855822_dd, true);
    check<decimal64_t>(1000, 31.62277660168379_dd, 31.62277660168380_dd, false);

    check<decimal128_t>(2, 1.414213562373095048801688724209698_dl, 1.414213562373095048801688724209699_dl, false);
    check<decimal128_t>(3, 1.732050807568877293527446341505872_dl, 1.732050807568877293527446341505873_dl, false);
    check<decimal128_t>(4, 2_dl, 2_dl, false);
    check<decimal128_t>(5, 2.236067977499789696409173668731276_dl, 2.236067977499789696409173668731277_dl, false);
    check<decimal128_t>(6, 2.449489742783178098197284074705891_dl, 2.449489742783178098197284074705892_dl, false);
    check<decimal128_t>(7, 2.645751311064590590501615753639260_dl, 2.645751311064590590501615753639261_dl, false);
    check<decimal128_t>(8, 2.828427124746190097603377448419396_dl, 2.828427124746190097603377448419397_dl, false);
    check<decimal128_t>(9, 3_dl, 3_dl, false);
    check<decimal128_t>(10, 3.162277660168379331998893544432718_dl, 3.162277660168379331998893544432719_dl, true);
    check<decimal128_t>(11, 3.316624790355399849114932736670686_dl, 3.316624790355399849114932736670687_dl, true);
    check<decimal128_t>(12, 3.464101615137754587054892683011744_dl, 3.464101615137754587054892683011745_dl, true);
    check<decimal128_t>(13, 3.605551275463989293119221267470495_dl, 3.605551275463989293119221267470496_dl, true);
    check<decimal128_t>(15, 3.872983346207416885179265399782399_dl, 3.872983346207416885179265399782400_dl, true);
    check<decimal128_t>(17, 4.123105625617660549821409855974077_dl, 4.123105625617660549821409855974078_dl, false);
    check<decimal128_t>(19, 4.358898943540673552236981983859615_dl, 4.358898943540673552236981983859616_dl, true);
    check<decimal128_t>(26, 5.099019513592784830028224109022781_dl, 5.099019513592784830028224109022782_dl, true);
    check<decimal128_t>(29, 5.385164807134504031250710491540329_dl, 5.385164807134504031250710491540330_dl, true);
    check<decimal128_t>(48, 6.928203230275509174109785366023489_dl, 6.928203230275509174109785366023490_dl, false);
    check<decimal128_t>(50, 7.071067811865475244008443621048490_dl, 7.071067811865475244008443621048491_dl, false);
    check<decimal128_t>(61, 7.810249675906654394129722735759101_dl, 7.810249675906654394129722735759102_dl, false);
    check<decimal128_t>(73, 8.544003745317531167871648326239706_dl, 8.544003745317531167871648326239707_dl, false);
    check<decimal128_t>(83, 9.110433579144298881945626104688669_dl, 9.110433579144298881945626104688670_dl, false);
    check<decimal128_t>(97, 9.848857801796104721746211414917624_dl, 9.848857801796104721746211414917625_dl, false);
    check<decimal128_t>(99, 9.949874371066199547344798210012060_dl, 9.949874371066199547344798210012061_dl, false);
    check<decimal128_t>(100, 10_dl, 10_dl, false);
    check<decimal128_t>(101, 10.04987562112089027021926491275957_dl, 10.04987562112089027021926491275958_dl, true);
    check<decimal128_t>(500, 22.36067977499789696409173668731276_dl, 22.36067977499789696409173668731277_dl, false);
    check<decimal128_t>(999, 31.60696125855821654520421398569900_dl, 31.60696125855821654520421398569901_dl, false);
    check<decimal128_t>(1000, 31.62277660168379331998893544432718_dl, 31.62277660168379331998893544432719_dl, true);

    fesetround(rounding_mode::fe_dec_to_nearest);

    return boost::report_errors();
}
