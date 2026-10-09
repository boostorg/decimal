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
    {fn::sinh_, "-4803100e-6"_DF, "0e0"_DF, 0, 0, 1, "-6093974e-5"_DF, true},
    {fn::sinh_, "-4803100e-6"_DF, "0e0"_DF, 0, 0, 2, "-6093975e-5"_DF, true},
    {fn::sinh_, "-4803100e-6"_DF, "0e0"_DF, 0, 0, 4, "-6093974e-5"_DF, true},
};

const hard_case<decimal64_t> d64_cases[] {
    {fn::cosh_, "1000000000000000e-22"_DD, "0e0"_DD, 0, 0, 1, "1000000000000006e-15"_DD, true},
    {fn::cosh_, "1000000000000000e-22"_DD, "0e0"_DD, 0, 0, 2, "1000000000000005e-15"_DD, true},
    {fn::cosh_, "1000000000000000e-22"_DD, "0e0"_DD, 0, 0, 4, "1000000000000005e-15"_DD, true},
    {fn::sinh_, "8749999990000000e-16"_DD, "0e0"_DD, 0, 0, 1, "9910066357364261e-16"_DD, true},
    {fn::sinh_, "8749999990000000e-16"_DD, "0e0"_DD, 0, 0, 2, "9910066357364260e-16"_DD, true},
    {fn::sinh_, "8749999990000000e-16"_DD, "0e0"_DD, 0, 0, 4, "9910066357364260e-16"_DD, true},
    {fn::sinh_, "2312585092994046e-15"_DD, "0e0"_DD, 0, 0, 0, "5000748343733383e-15"_DD, true},
    {fn::sinh_, "2312585092994046e-15"_DD, "0e0"_DD, 0, 0, 3, "5000748343733383e-15"_DD, true},
    {fn::sinh_, "-2312585092994046e-15"_DD, "0e0"_DD, 0, 0, 0, "-5000748343733383e-15"_DD, true},
    {fn::sinh_, "4292420000000000e-16"_DD, "0e0"_DD, 0, 0, 1, "4425451781581935e-16"_DD, true},
    {fn::sinh_, "4292420000000000e-16"_DD, "0e0"_DD, 0, 0, 2, "4425451781581934e-16"_DD, true},
    {fn::sinh_, "4292420000000000e-16"_DD, "0e0"_DD, 0, 0, 4, "4425451781581934e-16"_DD, true},
    {fn::cosh_, "9347293207478785e-15"_DD, "0e0"_DD, 0, 0, 0, "5733870344714094e-12"_DD, true},
    {fn::cosh_, "9347293207478785e-15"_DD, "0e0"_DD, 0, 0, 3, "5733870344714094e-12"_DD, true},
};

const hard_case<decimal128_t> d128_cases[] {
    {fn::sinh_, "1000000000000000000000000000000000e-40"_DL, "0e0"_DL, 0, 0, 1, "1000000000000001666666666666667501e-40"_DL, true},
    {fn::sinh_, "1000000000000000000000000000000000e-40"_DL, "0e0"_DL, 0, 0, 2, "1000000000000001666666666666667500e-40"_DL, true},
    {fn::sinh_, "1000000000000000000000000000000000e-40"_DL, "0e0"_DL, 0, 0, 4, "1000000000000001666666666666667500e-40"_DL, true},
    {fn::sinh_, "3000000000000000000000000000000000e-49"_DL, "0e0"_DL, 0, 0, 1, "3000000000000000000000000000000046e-49"_DL, true},
    {fn::sinh_, "3000000000000000000000000000000000e-49"_DL, "0e0"_DL, 0, 0, 2, "3000000000000000000000000000000045e-49"_DL, true},
    {fn::sinh_, "3000000000000000000000000000000000e-49"_DL, "0e0"_DL, 0, 0, 4, "3000000000000000000000000000000045e-49"_DL, true},
    {fn::sinh_, "1832012756492626236581499281557213e-34"_DL, "0e0"_DL, 0, 0, 1, "1842277852262446185144207584087379e-34"_DL, true},
    {fn::sinh_, "1832012756492626236581499281557213e-34"_DL, "0e0"_DL, 0, 0, 2, "1842277852262446185144207584087378e-34"_DL, true},
    {fn::sinh_, "1832012756492626236581499281557213e-34"_DL, "0e0"_DL, 0, 0, 4, "1842277852262446185144207584087378e-34"_DL, true},
    {fn::cosh_, "2314121822360046985684795279430829e-33"_DL, "0e0"_DL, 0, 0, 0, "5107444148594391504694706104514296e-33"_DL, true},
    {fn::cosh_, "2314121822360046985684795279430829e-33"_DL, "0e0"_DL, 0, 0, 3, "5107444148594391504694706104514296e-33"_DL, true},
};

} // namespace

int main()
{
    run<decimal32_t, decimal_fast32_t>(d32_cases);
    run<decimal64_t, decimal_fast64_t>(d64_cases);
    run<decimal128_t, decimal_fast128_t>(d128_cases);

    return boost::report_errors();
}
