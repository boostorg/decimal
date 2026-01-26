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
// Decimal-native sqrt using lookup table + remainder elimination
//
// Algorithm:
// 1. Normalize input to [0.1, 1.0) or [1.0, 10.0) range based on exponent parity
// 2. Lookup sqrt(x) and 1/sqrt(x) from table with linear interpolation
// 3. Refine using remainder elimination (multiplication only, no division):
//    rem = gx - z^2
//    z = z + rem * recip_sqrt / 2
// 4. Rescale result based on original exponent
//
// Key optimizations over baseline:
// - Higher precision initial approximation (4-5 digits vs 2 digits)
// - Fewer iterations needed (1-3 vs 3-5)
// - Uses multiplication instead of division in refinement loop
// ============================================================================

namespace sqrt_tables {

// Lookup tables for decimal sqrt
// 64 entries for each exponent parity (even/odd), 128 total
// Covers x in [0.1, 1.0) for even exponent, [1.0, 10.0) for odd exponent
// Values scaled by 10^16 for precision

// Even exponent table: x in [0.1, 1.0)
// x_i = 0.1 + 0.9 * i / 64, i = 0..63
static constexpr std::uint64_t sqrt_even[64] = {
      3162277660168379ULL,  3377314021526574ULL,  3579455265819088ULL,  3770775782249589ULL,
      3952847075210474ULL,  4126893504804794ULL,  4293891009329417ULL,  4454632420301365ULL,
      4609772228646443ULL,  4759858191164942ULL,  4905354217587145ULL,  5046657309546587ULL,
      5184110338331930ULL,  5318011846545661ULL,  5448623679425841ULL,  5576177005798865ULL,
      5700877125495689ULL,  5822907349426057ULL,  5942432162002356ULL,  6059599821770411ULL,
      6174544517614234ULL,  6287388169979645ULL,  6398241946034863ULL,  6507207542410185ULL,
      6614378277661476ULL,  6719840027857806ULL,  6823672031978090ULL,  6925947588597534ULL,
      7026734661277598ULL,  7126096406869612ULL,  7224091638399944ULL,  7320775232173161ULL,
      7416198487095662ULL,  7510409442899900ULL,  7603453162872774ULL,  7695371985810692ULL,
      7786205751198718ULL,  7875992001011682ULL,  7964766161036995ULL,  8052561704203203ULL,
      8139410298049853ULL,  8225341938181050ULL,  8310385069297330ULL,  8394566695190407ULL,
      8477912478906585ULL,  8560446834131966ULL,  8642193008721802ULL,  8723173161183950ULL,
      8803408430829504ULL,  8882919002219934ULL,  8961724164467460ULL,  9039842365882272ULL,
      9117291264405234ULL,  9194087774216646ULL,  9270248108869578ULL,  9345787821259372ULL,
      9420721840708385ULL,  9495064507416471ULL,  9568829604502318ULL,  9642030387838445ULL,
      9714679613862723ULL,  9786789565531691ULL,  9858372076565176ULL,  9929438554117750ULL
};

static constexpr std::uint64_t recip_sqrt_even[64] = {
     31622776601683793ULL, 29609328407904210ULL, 27937211830783128ULL, 26519741765271834ULL,
     25298221281347034ULL, 24231301312615306ULL, 23288900389583278ULL, 22448541330652549ULL,
     21693045781865617ULL, 21009029257555609ULL, 20385887657505021ULL, 19815096184722797ULL,
     19289712886816484ULL, 18804019788890738ULL, 18353258709644941ULL, 17933433586488813ULL,
     17541160386140584ULL, 17173551629643673ULL, 16828126476466850ULL, 16502739940140694ULL,
     16195526603578320ULL, 15904855449750882ULL, 15629293303291270ULL, 15367575007905972ULL,
     15118578920369089ULL, 14881306636086490ULL, 14654866108946234ULL, 14438457513688670ULL,
     14231361339296401ULL, 14032928308912467ULL, 13842570804119654ULL, 13659755535250213ULL,
     13483997249264841ULL, 13314853305972123ULL, 13151918984428583ULL, 12994823406118319ULL,
     12843225981358710ULL, 12696813301379034ULL, 12555296411486889ULL, 12418408411301325ULL,
     12285902336679023ULL, 12157549285071298ULL, 12033136751923736ULL, 11912467150602795ULL,
     11795356492391770ULL, 11681633206491382ULL, 11571137082807434ULL, 11463718322705807ULL,
     11359236684941296ULL, 11257560715684669ULL, 11158567053033413ULL, 11062139797637962ULL,
     10968169942141635ULL, 10876554853047418ULL, 10787197799411873ULL, 10700007523445434ULL,
     10614897848685505ULL, 10531787321917749ULL, 10450598885463283ULL, 10371259576834630ULL,
     10293700253099574ULL, 10217855337586105ULL, 10143662586819474ULL, 10071062875808811ULL
};

// Odd exponent table: x in [1.0, 10.0)
// x_i = 1.0 + 9.0 * i / 64, i = 0..63
static constexpr std::uint64_t sqrt_odd[64] = {
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

static constexpr std::uint64_t recip_sqrt_odd[64] = {
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

// Table parameters
constexpr int table_size = 64;
constexpr int table_scale = 16;  // Values are scaled by 10^16

} // namespace sqrt_tables

// ============================================================================
// Main sqrt implementation
// ============================================================================

template <typename T>
constexpr auto sqrt_impl(T x) noexcept
    BOOST_DECIMAL_REQUIRES(detail::is_decimal_floating_point_v, T)
{
    const auto fpc = fpclassify(x);

    // ========== Special case handling ==========
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

    // ========== Extract significand and exponent ==========
    int exp10val{};
    auto sig = frexp10(x, &exp10val);

    // Handle pure powers of 10 (exact results)
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

    // ========== Normalize to [0.1, 1.0) or [1.0, 10.0) ==========
    constexpr int digits10 = std::numeric_limits<T>::digits10;

    // Create gx in [0.1, 1.0)
    T gx{sig, -digits10};
    exp10val += digits10;

    // Determine exponent parity
    // For odd exponent, multiply gx by 10 to get [1.0, 10.0) range
    const bool odd_exp = (exp10val & 1) != 0;
    if (odd_exp)
    {
        gx *= T{10};
        exp10val -= 1;
    }

    // ========== Table lookup with linear interpolation ==========
    // Select appropriate table based on exponent parity
    const auto* sqrt_table = odd_exp ? sqrt_tables::sqrt_odd : sqrt_tables::sqrt_even;
    const auto* recip_table = odd_exp ? sqrt_tables::recip_sqrt_odd : sqrt_tables::recip_sqrt_even;

    // Calculate table index
    // Even: gx in [0.1, 1.0), index = (gx - 0.1) / 0.9 * 64
    // Odd:  gx in [1.0, 10.0), index = (gx - 1.0) / 9.0 * 64
    T base = odd_exp ? T{1} : T{1, -1};
    T range = odd_exp ? T{9} : T{9, -1};
    T step = range / T{sqrt_tables::table_size};

    // Compute normalized position [0, 64)
    T normalized = (gx - base) / step;

    // Convert to integer index, clamped to valid range
    int index = static_cast<int>(normalized);
    if (index < 0) index = 0;
    if (index >= sqrt_tables::table_size - 1) index = sqrt_tables::table_size - 2;

    // Compute interpolation fraction [0, 1)
    T x_i = base + T{index} * step;
    T frac = (gx - x_i) / step;

    // Get table values and interpolate
    // sqrt(gx) approximation
    T z0{sqrt_table[index], -sqrt_tables::table_scale};
    T z1{sqrt_table[index + 1], -sqrt_tables::table_scale};
    T z = z0 + (z1 - z0) * frac;

    // 1/sqrt(gx) approximation
    T r0{recip_table[index], -sqrt_tables::table_scale};
    T r1{recip_table[index + 1], -sqrt_tables::table_scale};
    T r = r0 + (r1 - r0) * frac;

    // ========== Remainder elimination refinement ==========
    // Each iteration approximately doubles precision
    // Initial: ~4-5 digits -> 8-10 -> 16-20 -> 32-40
    // Uses multiplication only (no division in the loop)

    constexpr int rem_iters = (digits10 <= 7) ? 1 : (digits10 <= 16) ? 2 : 3;
    constexpr T half{5, -1};  // 0.5

    for (int i = 0; i < rem_iters; ++i)
    {
        T z_sq = z * z;
        T rem = gx - z_sq;
        // correction = rem / (2 * z) ≈ rem * r / 2
        T correction = rem * r * half;
        z = z + correction;
    }

    // ========== Final Newton-Raphson polish (optional) ==========
    // One final iteration using the standard formula for maximum precision
    // This uses division but ensures we get the last few digits right
    if (digits10 > 16)
    {
        z = (z + gx / z) * half;
    }

    // ========== Rescale result ==========
    // sqrt(x * 10^exp) = sqrt(x) * 10^(exp/2)
    if (exp10val != 0)
    {
        z *= T{1, exp10val / 2};
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
