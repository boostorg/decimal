// Copyright 2023 - 2024 Matt Borland
// Copyright 2023 - 2024 Christopher Kormanyos
// Copyright 2025 - 2026 Justin Zhu
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_DECIMAL_DETAIL_CMATH_SQRT_HPP
#define BOOST_DECIMAL_DETAIL_CMATH_SQRT_HPP

#include <boost/decimal/fwd.hpp>
#include <boost/decimal/detail/type_traits.hpp>
#include <boost/decimal/detail/concepts.hpp>
#include <boost/decimal/detail/config.hpp>
#include <boost/decimal/detail/cmath/frexp10.hpp>
#include <boost/decimal/detail/remove_trailing_zeros.hpp>
#include <boost/decimal/numbers.hpp>

#ifndef BOOST_DECIMAL_BUILD_MODULE
#include <type_traits>
#include <cstdint>
#include <limits>
#endif

namespace boost {
namespace decimal {

namespace detail {

// ============================================================================
// Decimal sqrt: SoftFloat-style remainder elimination
//
// Per doc/decimal_float_and_sqrt_optimization.md:
// 1. Normalize x = sig × 10^e so that gx is in [1, 10) (same table for all).
// 2. Table lookup: recip_sqrt ≈ 1/sqrt(gx), sqrt ≈ sqrt(gx) with interpolation.
// 3. Initial value: z = table_sqrt (or z ≈ gx × recip_sqrt).
// 4. Remainder: rem = gx − z² (exact in decimal).
// 5. Correction: q = rem × recip_sqrt / 2; z = z + q (same recip_sqrt as table).
// 6. Optional: repeat remainder step or one Newton polish for high precision.
// 7. Rescale: result = z × 10^(e/2), multiply by sqrt(10) when e is odd.
// ============================================================================

namespace sqrt_tables {

// Single table for range [1, 10): sqrt and 1/sqrt at 64 sample points.
// x_i = 1 + 9*i/64, i = 0..63. Values scaled by 10^16.
static constexpr std::uint64_t sqrt_table[64] = {
    10000000000000000ULL, 10680004681646913ULL, 11319231422671770ULL, 11924240017711820ULL,
    12500000000000000ULL, 13050383136138187ULL, 13578475614000269ULL, 14086784586980806ULL,
    14577379737113251ULL, 15051993223490369ULL, 15512092057488570ULL, 15958931668504630ULL,
    16393596310755001ULL, 16817030058842137ULL, 17230060940112777ULL, 17633419974582355ULL,
    18027756377319946ULL, 18413649828320294ULL, 18791620472966135ULL, 19162137145944864ULL,
    19525624189766635ULL, 19882467150733582ULL, 20233017570298306ULL, 20577597041442909ULL,
    20916500663351888ULL, 21250000000000000ULL, 21578345627040085ULL, 21901769334919039ULL,
    22220486043288972ULL, 22534695471649933ULL, 22844583603121331ULL, 23150323971815167ULL,
    23452078799117147ULL, 23750000000000000ULL, 24044230077089180ULL, 24334902917414731ULL,
    24622144504490261ULL, 24906073556464093ULL, 25186802099512355ULL, 25464435984329203ULL,
    25739075352467500ULL, 26010815058356014ULL, 26279745052035797ULL, 26545950726994126ULL,
    26809513236909020ULL, 27070509784634644ULL, 27329013886344307ULL, 27585095613392388ULL,
    27838821814150109ULL, 28090256317805289ULL, 28339460121886584ULL, 28586491565073178ULL,
    28831406486676989ULL, 29074258374032518ULL, 29315098498896434ULL, 29553976043842222ULL,
    29790938219532462ULL, 30026030373660784ULL, 30259296092275510ULL, 30490777294126169ULL,
    30720514318611268ULL, 30948546007849867ULL, 31174909783349814ULL, 31399641717701175ULL
};

static constexpr std::uint64_t recip_sqrt_table[64] = {
    10000000000000000ULL,  9363291775690445ULL,  8834522085987723ULL,  8386278693775346ULL,
     8000000000000000ULL,  7662610281769211ULL,  7364596943186586ULL,  7098852075328910ULL,
     6859943405700353ULL,  6643638388299197ULL,  6446583712203042ULL,  6266083599903658ULL,
     6099942813304186ULL,  5946353169977330ULL,  5803810000880093ULL,  5671049640066687ULL,
     5547001962252291ULL,  5430753866417045ULL,  5321520841901914ULL,  5218624584427537ULL,
     5121475197315838ULL,  5029556907695451ULL,  4942416505721723ULL,  4859653913846296ULL,
     4780914437337574ULL,  4705882352941176ULL,  4634275570907937ULL,  4565841164282796ULL,
     4500351603704095ULL,  4437601569801832ULL,  4377405241316662ULL,  4319593977248311ULL,
     4264014327112208ULL,  4210526315789473ULL,  4159001959280290ULL,  4109323975500112ULL,
     4061384660534476ULL,  4015084905827964ULL,  3970333335883721ULL,  3927045549390527ULL,
     3885143449429056ULL,  3844554650657701ULL,  3805211953235952ULL,  3767052874784088ULL,
     3730019232961255ULL,  3694056772316881ULL,  3659114829970785ULL,  3625146035435550ULL,
     3592106040535498ULL,  3559953275919878ULL,  3528648731129847ULL,  3498155755573008ULL,
     3468439878096479ULL,  3439468643138782ULL,  3411211461689766ULL,  3383639475502508ULL,
     3356725433186756ULL,  3330443576974506ULL,  3304769539088110ULL,  3279680246763151ULL,
     3255153835084637ULL,  3231169566888077ULL,  3207707759058501ULL,  3184749714632131ULL
};

constexpr int table_size = 64;
constexpr int table_scale = 16;  // values scaled by 10^16

} // namespace sqrt_tables

// ============================================================================
// sqrt_impl
// ============================================================================

template <typename T>
constexpr auto sqrt_impl(T x) noexcept
    BOOST_DECIMAL_REQUIRES(detail::is_decimal_floating_point_v, T)
{
    const auto fpc = fpclassify(x);

    // ---------- Special cases ----------
    #ifndef BOOST_DECIMAL_FAST_MATH
    if ((fpc == FP_NAN) || (fpc == FP_ZERO))
    {
        return x;
    }
    if (signbit(x))
    {
        return std::numeric_limits<T>::quiet_NaN();
    }
    if (fpc == FP_INFINITE)
    {
        return std::numeric_limits<T>::infinity();
    }
    #else
    if (signbit(x))
    {
        return T{0};
    }
    #endif

    // ---------- Extract significand and exponent (x = sig × 10^e) ----------
    int exp10val{};
    auto sig = frexp10(x, &exp10val);

    const auto zeros_removal = remove_trailing_zeros(sig);
    const bool is_pure = (zeros_removal.trimmed_number == 1U);

    if (is_pure)
    {
        const int p10 = exp10val + static_cast<int>(zeros_removal.number_of_removed_zeros);

        if (p10 == 0)
        {
            return T{1};
        }

        const int p10_mod2 = (p10 % 2);
        T result = T{1, p10 / 2};

        if (p10_mod2 == 1)
        {
            result *= numbers::sqrt10_v<T>;
        }
        else if (p10_mod2 == -1)
        {
            result /= numbers::sqrt10_v<T>;
        }
        return result;
    }

    // ---------- Normalize to [1, 10) (doc: single range for table) ----------
    constexpr int digits10 = std::numeric_limits<T>::digits10;

    // gx = sig × 10^(-(digits10-1)) so gx in (0, 10]; then ensure gx in [1, 10)
    T gx{sig, -(digits10 - 1)};
    exp10val += (digits10 - 1);

    while (gx < T{1})
    {
        gx *= T{10};
        --exp10val;
    }
    // gx in [1, 10) now

    // ---------- Table lookup with linear interpolation ----------
    // Index: gx in [1, 10) -> index = (gx - 1) / 9 * 64
    constexpr T one{1};
    constexpr T nine{9};
    constexpr T step{nine / T{sqrt_tables::table_size}};

    T normalized = (gx - one) / step;
    int index = static_cast<int>(normalized);
    if (index < 0) { index = 0; }
    if (index >= sqrt_tables::table_size - 1) { index = sqrt_tables::table_size - 2; }

    T x_lo = one + T{index} * step;
    T frac = (gx - x_lo) / step;

    T z0{sqrt_tables::sqrt_table[index], -sqrt_tables::table_scale};
    T z1{sqrt_tables::sqrt_table[index + 1], -sqrt_tables::table_scale};
    T z = z0 + (z1 - z0) * frac;

    T r0{sqrt_tables::recip_sqrt_table[index], -sqrt_tables::table_scale};
    T r1{sqrt_tables::recip_sqrt_table[index + 1], -sqrt_tables::table_scale};
    T r = r0 + (r1 - r0) * frac;

    // ---------- Remainder elimination (doc: rem = gx - z², correction = rem × r / 2) ----------
    constexpr T half{5, -1};
    constexpr int rem_iters = (digits10 <= 7) ? 1 : (digits10 <= 16) ? 2 : 3;

    for (int i = 0; i < rem_iters; ++i)
    {
        T rem = gx - z * z;
        T correction = rem * r * half;
        z = z + correction;
    }

    // ---------- Optional Newton polish for high precision ----------
    if (digits10 > 16)
    {
        z = (z + gx / z) * half;
    }

    // ---------- Rescale: sqrt(x) = z × 10^(e/2), multiply by sqrt(10) when e odd ----------
    // C++ / truncates toward zero; for negative exp we need floor(exp10val/2), e.g. (-3)/2 = -1 but we need -2
    const int half_exp = (exp10val >= 0) ? (exp10val / 2) : ((exp10val - 1) / 2);
    if (half_exp != 0)
    {
        z *= T{1, half_exp};
    }
    if ((exp10val & 1) != 0)
    {
        z *= numbers::sqrt10_v<T>;
    }

    return z;
}

} // namespace detail

BOOST_DECIMAL_EXPORT template <typename T>
constexpr auto sqrt(T val) noexcept
    BOOST_DECIMAL_REQUIRES(detail::is_decimal_floating_point_v, T)
{
    using evaluation_type = detail::evaluation_type_t<T>;
    return static_cast<T>(detail::sqrt_impl(static_cast<evaluation_type>(val)));
}

} // namespace decimal
} // namespace boost

#endif // BOOST_DECIMAL_DETAIL_CMATH_SQRT_HPP
