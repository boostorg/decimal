// Copyright 2026 Shen-Ta Hsieh
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//
// Inputs that give a wrong result if an error bound of a first try gets smaller, in all rounding modes.
// tools/cmath_accuracy/rounding_grid checked each expected value against MPFR.

#include <boost/decimal.hpp>
#include <boost/core/lightweight_test.hpp>
#include <iostream>

using namespace boost::decimal;
using namespace boost::decimal::literals;

namespace {

enum class fn
{
    exp_, expm1_, exp2_, log_, log2_, log10_, log1p_, sin_, cos_, tan_, asin_, acos_, atan_, sinh_, cosh_, tanh_, asinh_,
    acosh_, atanh_, erf_, erfc_, tgamma_, lgamma_, zeta_, ellint_1_, ellint_2_, comp_ellint_1_, comp_ellint_2_, atan2_,
    pow_, pow_int_, beta_, hermite_, laguerre_, legendre_, assoc_laguerre_, assoc_legendre_
};

constexpr rounding_mode modes[] {rounding_mode::fe_dec_to_nearest, rounding_mode::fe_dec_upward,
                                 rounding_mode::fe_dec_downward, rounding_mode::fe_dec_to_nearest_from_zero,
                                 rounding_mode::fe_dec_toward_zero};

template <typename T>
struct hard_case
{
    fn f;
    T a;
    T b;
    int n;
    unsigned m;
    int mode;
    T want;
    bool fast;
};

template <typename T>
auto eval(const fn f, const T a, const T b, const int n, const unsigned m) -> T
{
    const auto un {static_cast<unsigned>(n)};
    switch (f)
    {
        case fn::exp_: return exp(a);
        case fn::expm1_: return expm1(a);
        case fn::exp2_: return exp2(a);
        case fn::log_: return log(a);
        case fn::log2_: return log2(a);
        case fn::log10_: return log10(a);
        case fn::log1p_: return log1p(a);
        case fn::sin_: return sin(a);
        case fn::cos_: return cos(a);
        case fn::tan_: return tan(a);
        case fn::asin_: return asin(a);
        case fn::acos_: return acos(a);
        case fn::atan_: return atan(a);
        case fn::sinh_: return sinh(a);
        case fn::cosh_: return cosh(a);
        case fn::tanh_: return tanh(a);
        case fn::asinh_: return asinh(a);
        case fn::acosh_: return acosh(a);
        case fn::atanh_: return atanh(a);
        case fn::erf_: return erf(a);
        case fn::erfc_: return erfc(a);
        case fn::tgamma_: return tgamma(a);
        case fn::lgamma_: return lgamma(a);
        case fn::zeta_: return riemann_zeta(a);
        case fn::ellint_1_: return ellint_1(a, b);
        case fn::ellint_2_: return ellint_2(a, b);
        case fn::comp_ellint_1_: return comp_ellint_1(a);
        case fn::comp_ellint_2_: return comp_ellint_2(a);
        case fn::atan2_: return atan2(a, b);
        case fn::pow_: return pow(a, b);
        case fn::pow_int_: return pow(a, n);
        case fn::beta_: return beta(a, b);
        case fn::hermite_: return hermite(un, a);
        case fn::laguerre_: return laguerre(un, a);
        case fn::legendre_: return legendre(un, a);
        case fn::assoc_laguerre_: return assoc_laguerre(un, m, a);
        case fn::assoc_legendre_: return assoc_legendre(un, m, a);
    }
    return T {};
}

template <typename T>
void check(const fn f, const T a, const T b, const int n, const unsigned m, const int mode, const T want)
{
    #ifdef BOOST_DECIMAL_NO_CONSTEVAL_DETECTION
    // fesetround has an effect only with the detection of constant evaluation
    if (mode != 0)
    {
        return;
    }
    #endif
    fesetround(modes[mode]);
    const auto got {eval(f, a, b, n, m)};
    fesetround(modes[0]);
    if (!BOOST_TEST(got == want && signbit(got) == signbit(want)))
    {
        std::cerr << "fn " << static_cast<int>(f) << " a " << a << " b " << b << " n " << n << " m " << m << " mode "
                  << mode << " got " << got << " want " << want << std::endl;
    }
}

template <typename T, typename Fast, std::size_t N>
void run(const hard_case<T> (&cases)[N])
{
    for (const auto& c : cases)
    {
        check<T>(c.f, c.a, c.b, c.n, c.m, c.mode, c.want);
        if (c.fast)
        {
            check<Fast>(c.f, static_cast<Fast>(c.a), static_cast<Fast>(c.b), c.n, c.m, c.mode, static_cast<Fast>(c.want));
        }
    }
}

const hard_case<decimal32_t> d32_cases[] {
    {fn::exp_, "5000000e-13"_DF, "0e0"_DF, 0, 0, 0, "1000001e-6"_DF, true},
    {fn::exp_, "5000000e-13"_DF, "0e0"_DF, 0, 0, 3, "1000001e-6"_DF, true},
    {fn::exp_, "9999999e-13"_DF, "0e0"_DF, 0, 0, 1, "1000002e-6"_DF, true},
    {fn::sinh_, "-4803100e-6"_DF, "0e0"_DF, 0, 0, 1, "-6093974e-5"_DF, true},
    {fn::sinh_, "-4803100e-6"_DF, "0e0"_DF, 0, 0, 2, "-6093975e-5"_DF, true},
    {fn::sinh_, "-4803100e-6"_DF, "0e0"_DF, 0, 0, 4, "-6093974e-5"_DF, true},
    {fn::tanh_, "6282600e-8"_DF, "0e0"_DF, 0, 0, 1, "6274347e-8"_DF, true},
    {fn::tanh_, "6282600e-8"_DF, "0e0"_DF, 0, 0, 2, "6274346e-8"_DF, true},
    {fn::tanh_, "6282600e-8"_DF, "0e0"_DF, 0, 0, 4, "6274346e-8"_DF, true},
    {fn::exp_, "-5522681e-8"_DF, "0e0"_DF, 0, 0, 1, "9462706e-7"_DF, true},
    {fn::exp_, "-5522681e-8"_DF, "0e0"_DF, 0, 0, 2, "9462705e-7"_DF, true},
    {fn::exp_, "-5522681e-8"_DF, "0e0"_DF, 0, 0, 4, "9462705e-7"_DF, true},
    {fn::exp_, "-3679210e-9"_DF, "0e0"_DF, 0, 0, 0, "9963276e-7"_DF, true},
    {fn::exp_, "-3679210e-9"_DF, "0e0"_DF, 0, 0, 3, "9963276e-7"_DF, true},
    {fn::exp2_, "-5394789e-9"_DF, "0e0"_DF, 0, 0, 1, "9962677e-7"_DF, true},
    {fn::exp2_, "-5394789e-9"_DF, "0e0"_DF, 0, 0, 2, "9962676e-7"_DF, true},
    {fn::exp2_, "-5394789e-9"_DF, "0e0"_DF, 0, 0, 4, "9962676e-7"_DF, true},
    {fn::exp2_, "-1477860e-9"_DF, "0e0"_DF, 0, 0, 0, "9989762e-7"_DF, true},
    {fn::exp2_, "-1477860e-9"_DF, "0e0"_DF, 0, 0, 3, "9989762e-7"_DF, true},
    {fn::sinh_, "-8259310e-7"_DF, "0e0"_DF, 0, 0, 1, "-9230895e-7"_DF, true},
    {fn::sinh_, "-8259310e-7"_DF, "0e0"_DF, 0, 0, 2, "-9230896e-7"_DF, true},
    {fn::sinh_, "-8259310e-7"_DF, "0e0"_DF, 0, 0, 4, "-9230895e-7"_DF, true},
    {fn::cosh_, "5290311e-6"_DF, "0e0"_DF, 0, 0, 1, "9920509e-5"_DF, true},
    {fn::cosh_, "5290311e-6"_DF, "0e0"_DF, 0, 0, 2, "9920508e-5"_DF, true},
    {fn::cosh_, "5290311e-6"_DF, "0e0"_DF, 0, 0, 4, "9920508e-5"_DF, true},
    {fn::cosh_, "1639435e-5"_DF, "0e0"_DF, 0, 0, 0, "6590916e0"_DF, true},
    {fn::cosh_, "1639435e-5"_DF, "0e0"_DF, 0, 0, 3, "6590916e0"_DF, true},
    {fn::tanh_, "3010677e-7"_DF, "0e0"_DF, 0, 0, 1, "2922894e-7"_DF, true},
    {fn::tanh_, "3010677e-7"_DF, "0e0"_DF, 0, 0, 2, "2922893e-7"_DF, true},
    {fn::tanh_, "3010677e-7"_DF, "0e0"_DF, 0, 0, 4, "2922893e-7"_DF, true},
};

const hard_case<decimal64_t> d64_cases[] {
    {fn::tanh_, "3000000000000000e-20"_DD, "0e0"_DD, 0, 0, 1, "2999999999100001e-20"_DD, true},
    {fn::tanh_, "3000000000000000e-20"_DD, "0e0"_DD, 0, 0, 2, "2999999999100000e-20"_DD, true},
    {fn::tanh_, "3000000000000000e-20"_DD, "0e0"_DD, 0, 0, 4, "2999999999100000e-20"_DD, true},
    {fn::tanh_, "7500000001000000e-16"_DD, "0e0"_DD, 0, 0, 1, "6351489524469460e-16"_DD, true},
    {fn::tanh_, "7500000001000000e-16"_DD, "0e0"_DD, 0, 0, 2, "6351489524469459e-16"_DD, true},
    {fn::tanh_, "7500000001000000e-16"_DD, "0e0"_DD, 0, 0, 4, "6351489524469459e-16"_DD, true},
    {fn::log2_, "2000000000001000e-16"_DD, "0e0"_DD, 0, 0, 1, "-2321928094886641e-15"_DD, true},
    {fn::log2_, "2000000000001000e-16"_DD, "0e0"_DD, 0, 0, 2, "-2321928094886642e-15"_DD, true},
    {fn::log2_, "2000000000001000e-16"_DD, "0e0"_DD, 0, 0, 4, "-2321928094886641e-15"_DD, true},
    {fn::expm1_, "8660000000000009e-16"_DD, "0e0"_DD, 0, 0, 0, "1377382279962067e-15"_DD, true},
    {fn::expm1_, "8660000000000009e-16"_DD, "0e0"_DD, 0, 0, 3, "1377382279962067e-15"_DD, true},
    {fn::expm1_, "3999999000000000e-15"_DD, "0e0"_DD, 0, 0, 0, "5359809543502150e-14"_DD, true},
    {fn::expm1_, "3999999000000000e-15"_DD, "0e0"_DD, 0, 0, 3, "5359809543502150e-14"_DD, true},
    {fn::expm1_, "-6999999999990000e-16"_DD, "0e0"_DD, 0, 0, 1, "-5034146962080938e-16"_DD, true},
    {fn::expm1_, "-6999999999990000e-16"_DD, "0e0"_DD, 0, 0, 2, "-5034146962080939e-16"_DD, true},
    {fn::expm1_, "-6999999999990000e-16"_DD, "0e0"_DD, 0, 0, 4, "-5034146962080938e-16"_DD, true},
    {fn::cosh_, "1000000000000000e-22"_DD, "0e0"_DD, 0, 0, 1, "1000000000000006e-15"_DD, true},
    {fn::cosh_, "1000000000000000e-22"_DD, "0e0"_DD, 0, 0, 2, "1000000000000005e-15"_DD, true},
    {fn::cosh_, "1000000000000000e-22"_DD, "0e0"_DD, 0, 0, 4, "1000000000000005e-15"_DD, true},
    {fn::sinh_, "8749999990000000e-16"_DD, "0e0"_DD, 0, 0, 1, "9910066357364261e-16"_DD, true},
    {fn::sinh_, "8749999990000000e-16"_DD, "0e0"_DD, 0, 0, 2, "9910066357364260e-16"_DD, true},
    {fn::sinh_, "8749999990000000e-16"_DD, "0e0"_DD, 0, 0, 4, "9910066357364260e-16"_DD, true},
    {fn::sinh_, "2312585092994046e-15"_DD, "0e0"_DD, 0, 0, 0, "5000748343733383e-15"_DD, true},
    {fn::sinh_, "2312585092994046e-15"_DD, "0e0"_DD, 0, 0, 3, "5000748343733383e-15"_DD, true},
    {fn::sinh_, "-2312585092994046e-15"_DD, "0e0"_DD, 0, 0, 0, "-5000748343733383e-15"_DD, true},
    {fn::log1p_, "-1000000000000000e-30"_DD, "0e0"_DD, 0, 0, 0, "-1000000000000001e-30"_DD, true},
    {fn::log1p_, "-1000000000000000e-30"_DD, "0e0"_DD, 0, 0, 3, "-1000000000000001e-30"_DD, true},
    {fn::log1p_, "-1000000000000000e-29"_DD, "0e0"_DD, 0, 0, 1, "-1000000000000005e-29"_DD, true},
    {fn::log1p_, "1599999999999991e-15"_DD, "0e0"_DD, 0, 0, 1, "9555114450274329e-16"_DD, true},
    {fn::log1p_, "1599999999999991e-15"_DD, "0e0"_DD, 0, 0, 2, "9555114450274328e-16"_DD, true},
    {fn::log1p_, "1599999999999991e-15"_DD, "0e0"_DD, 0, 0, 4, "9555114450274328e-16"_DD, true},
    {fn::exp_, "1000000000000000e-30"_DD, "0e0"_DD, 0, 0, 1, "1000000000000002e-15"_DD, true},
    {fn::exp_, "1000000000000000e-30"_DD, "0e0"_DD, 0, 0, 2, "1000000000000001e-15"_DD, true},
    {fn::exp_, "1000000000000000e-30"_DD, "0e0"_DD, 0, 0, 4, "1000000000000001e-15"_DD, true},
    {fn::expm1_, "-2572691559521949e-14"_DD, "0e0"_DD, 0, 0, 1, "-9999999999932866e-16"_DD, true},
    {fn::expm1_, "-2572691559521949e-14"_DD, "0e0"_DD, 0, 0, 2, "-9999999999932867e-16"_DD, true},
    {fn::expm1_, "-2572691559521949e-14"_DD, "0e0"_DD, 0, 0, 4, "-9999999999932866e-16"_DD, true},
    {fn::expm1_, "7974313892826290e-16"_DD, "0e0"_DD, 0, 0, 1, "1219831715723149e-15"_DD, true},
    {fn::expm1_, "7974313892826290e-16"_DD, "0e0"_DD, 0, 0, 2, "1219831715723148e-15"_DD, true},
    {fn::expm1_, "7974313892826290e-16"_DD, "0e0"_DD, 0, 0, 4, "1219831715723148e-15"_DD, true},
    {fn::expm1_, "7780793528972014e-13"_DD, "0e0"_DD, 0, 0, 0, "8233214800880547e322"_DD, true},
    {fn::expm1_, "7780793528972014e-13"_DD, "0e0"_DD, 0, 0, 3, "8233214800880547e322"_DD, true},
    {fn::exp_, "3199422158394802e-13"_DD, "0e0"_DD, 0, 0, 1, "8894854880896344e123"_DD, true},
    {fn::exp_, "3199422158394802e-13"_DD, "0e0"_DD, 0, 0, 2, "8894854880896343e123"_DD, true},
    {fn::exp_, "3199422158394802e-13"_DD, "0e0"_DD, 0, 0, 4, "8894854880896343e123"_DD, true},
    {fn::exp_, "-5000000000000001e-32"_DD, "0e0"_DD, 0, 0, 0, "9999999999999999e-16"_DD, true},
    {fn::exp_, "-5000000000000001e-32"_DD, "0e0"_DD, 0, 0, 3, "9999999999999999e-16"_DD, true},
    {fn::exp_, "4999999999999998e-31"_DD, "0e0"_DD, 0, 0, 0, "1000000000000000e-15"_DD, true},
    {fn::exp_, "4999999999999998e-31"_DD, "0e0"_DD, 0, 0, 3, "1000000000000000e-15"_DD, true},
    {fn::exp_, "9672152000000000e-14"_DD, "0e0"_DD, 0, 0, 0, "1013030257732202e27"_DD, true},
    {fn::exp_, "9672152000000000e-14"_DD, "0e0"_DD, 0, 0, 3, "1013030257732202e27"_DD, true},
    {fn::exp2_, "-9082059000000000e-17"_DD, "0e0"_DD, 0, 0, 1, "9389885103522843e-16"_DD, true},
    {fn::exp2_, "-9082059000000000e-17"_DD, "0e0"_DD, 0, 0, 2, "9389885103522842e-16"_DD, true},
    {fn::exp2_, "-9082059000000000e-17"_DD, "0e0"_DD, 0, 0, 4, "9389885103522842e-16"_DD, true},
    {fn::sinh_, "9018570000000000e-15"_DD, "0e0"_DD, 0, 0, 1, "4127481958617472e-12"_DD, true},
    {fn::sinh_, "9018570000000000e-15"_DD, "0e0"_DD, 0, 0, 2, "4127481958617471e-12"_DD, true},
    {fn::sinh_, "9018570000000000e-15"_DD, "0e0"_DD, 0, 0, 4, "4127481958617471e-12"_DD, true},
    {fn::cosh_, "9672152000000000e-14"_DD, "0e0"_DD, 0, 0, 0, "5065151288661012e26"_DD, true},
    {fn::cosh_, "9672152000000000e-14"_DD, "0e0"_DD, 0, 0, 3, "5065151288661012e26"_DD, true},
    {fn::tanh_, "-1968754408393735e-15"_DD, "0e0"_DD, 0, 0, 1, "-9617522572728491e-16"_DD, true},
    {fn::tanh_, "-1968754408393735e-15"_DD, "0e0"_DD, 0, 0, 2, "-9617522572728492e-16"_DD, true},
    {fn::tanh_, "-1968754408393735e-15"_DD, "0e0"_DD, 0, 0, 4, "-9617522572728491e-16"_DD, true},
    {fn::tanh_, "-3809892287671726e-16"_DD, "0e0"_DD, 0, 0, 0, "-3635662482549072e-16"_DD, true},
    {fn::tanh_, "-3809892287671726e-16"_DD, "0e0"_DD, 0, 0, 3, "-3635662482549072e-16"_DD, true},
    {fn::log1p_, "3811562386486627e-14"_DD, "0e0"_DD, 0, 0, 0, "3666521974514295e-15"_DD, true},
    {fn::log1p_, "3811562386486627e-14"_DD, "0e0"_DD, 0, 0, 3, "3666521974514295e-15"_DD, true},
    {fn::sinh_, "4292420000000000e-16"_DD, "0e0"_DD, 0, 0, 1, "4425451781581935e-16"_DD, true},
    {fn::sinh_, "4292420000000000e-16"_DD, "0e0"_DD, 0, 0, 2, "4425451781581934e-16"_DD, true},
    {fn::sinh_, "4292420000000000e-16"_DD, "0e0"_DD, 0, 0, 4, "4425451781581934e-16"_DD, true},
    {fn::cosh_, "9347293207478785e-15"_DD, "0e0"_DD, 0, 0, 0, "5733870344714094e-12"_DD, true},
    {fn::cosh_, "9347293207478785e-15"_DD, "0e0"_DD, 0, 0, 3, "5733870344714094e-12"_DD, true},
    {fn::tanh_, "6870523367548064e-15"_DD, "0e0"_DD, 0, 0, 0, "9999978453895833e-16"_DD, true},
    {fn::tanh_, "6870523367548064e-15"_DD, "0e0"_DD, 0, 0, 3, "9999978453895833e-16"_DD, true},
    {fn::tanh_, "1124000000000000e-16"_DD, "0e0"_DD, 0, 0, 0, "1119290350057122e-16"_DD, true},
    {fn::tanh_, "1124000000000000e-16"_DD, "0e0"_DD, 0, 0, 3, "1119290350057122e-16"_DD, true},
};

const hard_case<decimal128_t> d128_cases[] {
    {fn::tanh_, "8750000000000000000000000000000002e-34"_DL, "0e0"_DL, 0, 0, 0, "7039056039366210605876302696313539e-34"_DL, true},
    {fn::tanh_, "8750000000000000000000000000000002e-34"_DL, "0e0"_DL, 0, 0, 3, "7039056039366210605876302696313539e-34"_DL, true},
    {fn::tanh_, "-8750000000000000000000000000000002e-34"_DL, "0e0"_DL, 0, 0, 0, "-7039056039366210605876302696313539e-34"_DL, true},
    {fn::tanh_, "1200000000000000000000000000000000e-48"_DL, "0e0"_DL, 0, 0, 1, "1199999999999999999999999999999425e-48"_DL, true},
    {fn::tanh_, "1200000000000000000000000000000000e-48"_DL, "0e0"_DL, 0, 0, 2, "1199999999999999999999999999999424e-48"_DL, true},
    {fn::tanh_, "1200000000000000000000000000000000e-48"_DL, "0e0"_DL, 0, 0, 4, "1199999999999999999999999999999424e-48"_DL, true},
    {fn::tanh_, "3000000000000000000000000000000000e-49"_DL, "0e0"_DL, 0, 0, 1, "2999999999999999999999999999999911e-49"_DL, true},
    {fn::tanh_, "3000000000000000000000000000000000e-49"_DL, "0e0"_DL, 0, 0, 2, "2999999999999999999999999999999910e-49"_DL, true},
    {fn::tanh_, "3000000000000000000000000000000000e-49"_DL, "0e0"_DL, 0, 0, 4, "2999999999999999999999999999999910e-49"_DL, true},
    {fn::log_, "9999999999999999999999999999999990e-34"_DL, "0e0"_DL, 0, 0, 0, "-1000000000000000000000000000000001e-66"_DL, true},
    {fn::log_, "9999999999999999999999999999999990e-34"_DL, "0e0"_DL, 0, 0, 3, "-1000000000000000000000000000000001e-66"_DL, true},
    {fn::log_, "9999999999999999999999999999999980e-34"_DL, "0e0"_DL, 0, 0, 1, "-2000000000000000000000000000000002e-66"_DL, true},
    {fn::log10_, "8749999999999999999999999999999995e-34"_DL, "0e0"_DL, 0, 0, 0, "-5799194697768675492900042558084313e-35"_DL, true},
    {fn::log10_, "8749999999999999999999999999999995e-34"_DL, "0e0"_DL, 0, 0, 3, "-5799194697768675492900042558084313e-35"_DL, true},
    {fn::log10_, "8999999999999999999999999900000000e-34"_DL, "0e0"_DL, 0, 0, 0, "-4575749056067512540994419831526362e-35"_DL, true},
    {fn::expm1_, "1000000000000000000000000000000000e-53"_DL, "0e0"_DL, 0, 0, 1, "1000000000000000000005000000000001e-53"_DL, true},
    {fn::expm1_, "1000000000000000000000000000000000e-53"_DL, "0e0"_DL, 0, 0, 2, "1000000000000000000005000000000000e-53"_DL, true},
    {fn::expm1_, "1000000000000000000000000000000000e-53"_DL, "0e0"_DL, 0, 0, 4, "1000000000000000000005000000000000e-53"_DL, true},
    {fn::sinh_, "1000000000000000000000000000000000e-40"_DL, "0e0"_DL, 0, 0, 1, "1000000000000001666666666666667501e-40"_DL, true},
    {fn::sinh_, "1000000000000000000000000000000000e-40"_DL, "0e0"_DL, 0, 0, 2, "1000000000000001666666666666667500e-40"_DL, true},
    {fn::sinh_, "1000000000000000000000000000000000e-40"_DL, "0e0"_DL, 0, 0, 4, "1000000000000001666666666666667500e-40"_DL, true},
    {fn::sinh_, "3000000000000000000000000000000000e-49"_DL, "0e0"_DL, 0, 0, 1, "3000000000000000000000000000000046e-49"_DL, true},
    {fn::sinh_, "3000000000000000000000000000000000e-49"_DL, "0e0"_DL, 0, 0, 2, "3000000000000000000000000000000045e-49"_DL, true},
    {fn::sinh_, "3000000000000000000000000000000000e-49"_DL, "0e0"_DL, 0, 0, 4, "3000000000000000000000000000000045e-49"_DL, true},
    {fn::log1p_, "1562499999999999900000000000000000e-35"_DL, "0e0"_DL, 0, 0, 1, "1550418653596525316623866142706223e-35"_DL, true},
    {fn::log1p_, "1562499999999999900000000000000000e-35"_DL, "0e0"_DL, 0, 0, 2, "1550418653596525316623866142706222e-35"_DL, true},
    {fn::log1p_, "1562499999999999900000000000000000e-35"_DL, "0e0"_DL, 0, 0, 4, "1550418653596525316623866142706222e-35"_DL, true},
    {fn::log1p_, "-1562500000000000000000000000000003e-35"_DL, "0e0"_DL, 0, 0, 1, "-1574835696813916860754951146082829e-35"_DL, true},
    {fn::log1p_, "-1562500000000000000000000000000003e-35"_DL, "0e0"_DL, 0, 0, 2, "-1574835696813916860754951146082830e-35"_DL, true},
    {fn::log1p_, "-1562500000000000000000000000000003e-35"_DL, "0e0"_DL, 0, 0, 4, "-1574835696813916860754951146082829e-35"_DL, true},
    {fn::exp_, "1000000000000000000000000000000000e-52"_DL, "0e0"_DL, 0, 0, 1, "1000000000000000000100000000000001e-33"_DL, true},
    {fn::exp_, "1000000000000000000000000000000000e-52"_DL, "0e0"_DL, 0, 0, 2, "1000000000000000000100000000000000e-33"_DL, true},
    {fn::exp_, "1000000000000000000000000000000000e-52"_DL, "0e0"_DL, 0, 0, 4, "1000000000000000000100000000000000e-33"_DL, true},
    {fn::exp_, "-1000000000000000000000000000000000e-50"_DL, "0e0"_DL, 0, 0, 0, "9999999999999999900000000000000000e-34"_DL, true},
    {fn::exp_, "-1000000000000000000000000000000000e-50"_DL, "0e0"_DL, 0, 0, 3, "9999999999999999900000000000000000e-34"_DL, true},
    {fn::exp_, "-2000000000000000000000000000000000e-50"_DL, "0e0"_DL, 0, 0, 1, "9999999999999999800000000000000002e-34"_DL, true},
    {fn::expm1_, "5901024876129822236461206678093713e-34"_DL, "0e0"_DL, 0, 0, 0, "8041733113390086391409633640962786e-34"_DL, true},
    {fn::expm1_, "5901024876129822236461206678093713e-34"_DL, "0e0"_DL, 0, 0, 3, "8041733113390086391409633640962786e-34"_DL, true},
    {fn::expm1_, "-6300000000000000000000000000000000e-32"_DL, "0e0"_DL, 0, 0, 1, "-9999999999999999999999999995640389e-34"_DL, true},
    {fn::expm1_, "-6300000000000000000000000000000000e-32"_DL, "0e0"_DL, 0, 0, 2, "-9999999999999999999999999995640390e-34"_DL, true},
    {fn::expm1_, "-6300000000000000000000000000000000e-32"_DL, "0e0"_DL, 0, 0, 4, "-9999999999999999999999999995640389e-34"_DL, true},
    {fn::expm1_, "9900000000000000000000000000000000e-63"_DL, "0e0"_DL, 0, 0, 1, "9900000000000000000000000000049006e-63"_DL, true},
    {fn::expm1_, "9900000000000000000000000000000000e-63"_DL, "0e0"_DL, 0, 0, 2, "9900000000000000000000000000049005e-63"_DL, true},
    {fn::expm1_, "9900000000000000000000000000000000e-63"_DL, "0e0"_DL, 0, 0, 4, "9900000000000000000000000000049005e-63"_DL, true},
    {fn::sinh_, "1832012756492626236581499281557213e-34"_DL, "0e0"_DL, 0, 0, 1, "1842277852262446185144207584087379e-34"_DL, true},
    {fn::sinh_, "1832012756492626236581499281557213e-34"_DL, "0e0"_DL, 0, 0, 2, "1842277852262446185144207584087378e-34"_DL, true},
    {fn::sinh_, "1832012756492626236581499281557213e-34"_DL, "0e0"_DL, 0, 0, 4, "1842277852262446185144207584087378e-34"_DL, true},
    {fn::cosh_, "2314121822360046985684795279430829e-33"_DL, "0e0"_DL, 0, 0, 0, "5107444148594391504694706104514296e-33"_DL, true},
    {fn::cosh_, "2314121822360046985684795279430829e-33"_DL, "0e0"_DL, 0, 0, 3, "5107444148594391504694706104514296e-33"_DL, true},
    {fn::tanh_, "8028970589538156309399842784685738e-33"_DL, "0e0"_DL, 0, 0, 1, "9999997875999059360463179944183048e-34"_DL, true},
    {fn::tanh_, "8028970589538156309399842784685738e-33"_DL, "0e0"_DL, 0, 0, 2, "9999997875999059360463179944183047e-34"_DL, true},
    {fn::tanh_, "8028970589538156309399842784685738e-33"_DL, "0e0"_DL, 0, 0, 4, "9999997875999059360463179944183047e-34"_DL, true},
};

} // namespace

int main()
{
    run<decimal32_t, decimal_fast32_t>(d32_cases);
    run<decimal64_t, decimal_fast64_t>(d64_cases);
    run<decimal128_t, decimal_fast128_t>(d128_cases);

    return boost::report_errors();
}
