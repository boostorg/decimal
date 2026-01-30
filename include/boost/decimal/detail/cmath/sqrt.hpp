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
// Per doc/decimal_float_and_sqrt_optimization.md (two-table scheme, 2^e vs 10^e):
// 1. Normalize x = sig × 10^e so that gx is in [1, 10).
// 2. Index from gx; eps = position of gx within bin. r0 = k0[index] − k1[index]×eps.
// 3. Initial value: z = gx × r (r = r0 or refined).
// 4. Remainder: rem = gx − z²; correction: q = rem × r / 2; z = z + q.
// 5. Optional: repeat remainder step or one Newton polish.
// 6. Rescale: z × 10^(e/2), ×√10 when e odd.
// ============================================================================

namespace sqrt_tables {

// Two-table scheme (SoftFloat-style, cf. softfloat_approxRecipSqrt_1k0s / _1k1s):
// k0 = base 1/sqrt at bin, k1 = slope per bin. r0 = k0 − k1×eps (eps = position within bin).
// Range [1, 10), 128 bins, scale 10^16.
static constexpr std::uint64_t approxRecipSqrt_1k0s[128] = {
    9828721869343220ULL, 9511012772444224ULL, 9222246683267760ULL, 8958280175293669ULL,
    8715755371245493ULL, 8491918278948404ULL, 8284485969255964ULL, 8091547798786778ULL,
    7911490822946380ULL, 7742942695933236ULL, 7584727410239635ULL, 7435830597442770ULL,
    7295372041400851ULL, 7162583696573524ULL, 7036791954136679ULL, 6917403218673871ULL,
    6803892089000997ULL, 6695791605098995ULL, 6592685147442634ULL, 6494199667745591ULL,
    6400000000000000ULL, 6309784053800792ULL, 6223278732675312ULL, 6140236451626690ULL,
    6060432152628562ULL, 5983660736054126ULL, 5909734841224942ULL, 5838482921350614ULL,
    5769747567796951ULL, 5703384046396153ULL, 5639259014801931ULL, 5577249395005999ULL,
    5517241379310345ULL, 5459129551479523ULL, 5402816107625394ULL, 5348210163719005ULL,
    5295227138571961ULL, 5243788202755697ULL, 5193819785289564ULL, 5145253131074652ULL,
    5098023903017329ULL, 5052071823605381ULL, 5007340351395246ULL, 4963776388461581ULL,
    4921330015366920ULL, 4879954250643350ULL, 4839604832151228ULL, 4800240018001500ULL,
    4761820405005901ULL, 4724308762859941ULL, 4687669882472381ULL, 4651870437036643ULL,
    4616878854598090ULL, 4582665201009638ULL, 4549201072289468ULL, 4516459495501094ULL,
    4484414837369684ULL, 4453042719931014ULL, 4422319942582257ULL, 4392224409968225ULL,
    4362735065193694ULL, 4333831827903092ULL, 4305495536813798ULL, 4277707896329388ULL,
    4250451426894840ULL, 4223709418787637ULL, 4197465889067236ULL, 4171705541430886ULL,
    4146413728746754ULL, 4121576418055830ULL, 4097180157852672ULL, 4073212047471648ULL,
    4049659708420430ULL, 4026511257516003ULL, 4003755281690787ULL, 3981380814347516ULL,
    3959377313151598ULL, 3937734639158822ULL, 3916443037184518ULL, 3895493117327868ULL,
    3874875837571862ULL, 3854582487385678ULL, 3834604672261930ULL, 3814934299126462ULL,
    3795563562563060ULL, 3776484931799864ULL, 3757691138408166ULL, 3739175164667992ULL,
    3720930232558140ULL, 3702949793331471ULL, 3685227517639030ULL, 3667757286169162ULL,
    3650533180770203ULL, 3633549476027478ULL, 3616800631267395ULL, 3600281282963276ULL,
    3583986237519272ULL, 3567910464410330ULL, 3552049089657631ULL, 3536397389620282ULL,
    3520950785085292ULL, 3505704835639068ULL, 3490655234304682ULL, 3475797802430250ULL,
    3461128484814591ULL, 3446643345057310ULL, 3432338561121153ULL, 3418210421095303ULL,
    3404255319148936ULL, 3390469751665032ULL, 3376850313545018ULL, 3363393694675400ULL,
    3350096676548056ULL, 3336956129026353ULL, 3323969007249717ULL, 3311132348669690ULL,
    3298443270210938ULL, 3285898965551035ULL, 3273496702513168ULL, 3261233820566302ULL,
    3249107728427581ULL, 3237115901762068ULL, 3225255880975204ULL, 3213525269093564ULL,
    3201921729729816ULL, 3190442985127910ULL, 3179086814284815ULL, 3167851051145280ULL
};

// k1[i] = slope for bin i (k0[i] - k0[i+1] in scale 10^16); k1[127] = k1[126].
static constexpr std::uint64_t approxRecipSqrt_1k1s[128] = {
    317709096898996ULL, 288766089176464ULL, 263966507974091ULL, 242524804048176ULL,
    223837092297089ULL, 207432309692440ULL, 192938170469186ULL, 180056975840398ULL,
    168548127013144ULL, 158215285693601ULL, 148896812796865ULL, 140458556041919ULL,
    132788344827327ULL, 125791742436845ULL, 119388735462808ULL, 113511129672874ULL,
    108100483902002ULL, 103106457656361ULL,  98485479697043ULL,  94199667745591ULL,
     90215946199208ULL,  86505321125480ULL,  83042281048622ULL,  79804298998128ULL,
     76771416574436ULL,  73925894829184ULL,  71251919874328ULL,  68735353553663ULL,
     66363521400798ULL,  64125031594222ULL,  62009619795932ULL,  60008015695654ULL,
     58111827830822ULL,  56313443854129ULL,  54605943906389ULL,  52983025147044ULL,
     51438935816264ULL,  49968417466133ULL,  48566654214912ULL,  47229228057323ULL,
     45952079411948ULL,  44731472210135ULL,  43563962933665ULL,  42446373094661ULL,
     41375764723570ULL,  40349418492122ULL,  39364814149728ULL,  38419612995599ULL,
     37511642145960ULL,  36638880387560ULL,  35799445435738ULL,  34991582438553ULL,
     34213653588452ULL,  33464128720170ULL,  32741576788374ULL,  32044658131410ULL,
     31372117438670ULL,  30722777348757ULL,  30095532614032ULL,  29489344774531ULL,
     28903237290602ULL,  28336291089294ULL,  27787640484410ULL,  27256469434548ULL,
     26742008107203ULL,  26243529720401ULL,  25760347636350ULL,  25291812684132ULL,
     24837310690924ULL,  24396260203158ULL,  23968110381024ULL,  23552339051218ULL,
     23148450904427ULL,  22755975825216ULL,  22374467343271ULL,  22003501195918ULL,
     21642673992776ULL,  21291601974304ULL,  20949919856650ULL,  20617279756006ULL,
     20293350186184ULL,  19977815123748ULL,  19670373135468ULL,  19370736563402ULL,
     19078630763196ULL,  18793793391698ULL,  18515973740174ULL,  18244932109852ULL,
     17980439226669ULL,  17722275692441ULL,  17470231469868ULL,  17224105398959ULL,
     16983704742725ULL,  16748844760083ULL,  16519348304119ULL,  16295045444004ULL,
     16075773108942ULL,  15861374752699ULL,  15651700037349ULL,  15446604534990ULL,
     15245949446224ULL,  15049601334386ULL,  14857431874432ULL,  14669317615659ULL,
     14485139757281ULL,  14304783936157ULL,  14128140025850ULL,  13955101946367ULL,
     13785567483904ULL,  13619438120014ULL,  13456618869618ULL,  13297018127344ULL,
     13140547521703ULL,  12987121776636ULL,  12836658580027ULL,  12689078458752ULL,
     12544304659903ULL,  12402263037867ULL,  12262881946866ULL,  12126092138721ULL,
     11991826665513ULL,  11860020786864ULL,  11730611881640ULL,  11603539363748ULL,
     11478744601906ULL,  11356170843095ULL,  11235763139535ULL,  11235763139535ULL
};

constexpr int table_size = 128;
constexpr int table_scale = 16;  // values scaled by 10^16

// Wrapper for table lookup (SoftFloat-style: one function for recip sqrt approx).
// Returns r0 = approxRecipSqrt_1k0s[index] - approxRecipSqrt_1k1s[index]*eps.
template <typename T>
constexpr auto approx_recip_sqrt_1(int index, T eps) noexcept -> T
{
    if (index < 0) { index = 0; }
    if (index >= table_size) { index = table_size - 1; }
    T k0_val{static_cast<std::uint64_t>(approxRecipSqrt_1k0s[index]), -table_scale};
    T k1_val{static_cast<std::uint64_t>(approxRecipSqrt_1k1s[index]), -table_scale};
    return k0_val - k1_val * eps;
}

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

    // ---------- Two-table lookup: r0 = k0 − k1×eps (doc); z = gx × r ----------
    // Index: gx in [1, 10) -> index = (gx - 1) / step; eps = (gx - x_lo) / step (position within bin)
    constexpr T one{1};
    constexpr T nine{9};
    constexpr T step{nine / T{sqrt_tables::table_size}};

    T normalized = (gx - one) / step;
    int index = static_cast<int>(normalized);

    T x_lo = one + T{index} * step;
    T eps = (gx - x_lo) / step;  // position within bin, in [0, 1)

    T r = sqrt_tables::approx_recip_sqrt_1<T>(index, eps);  // r0 = k0 − k1×eps (SoftFloat-style)

    T z = gx * r;

    // ---------- Remainder handling: 2^e → 10^e mapping (SoftFloat primitives.h) ----------
    // SoftFloat (2^e): rem = sig_a - z²; q = (rem>>2)×recip_sqrt>>32; z += q (fixed-point).
    // Decimal (10^e): rem = gx - z²; q = rem×r/2; z += q (same formula, scale 10 instead of 2).
    constexpr T half{5, -1};
    constexpr int rem_iters = (digits10 <= 7) ? 1 : (digits10 <= 16) ? 2 : 3;

    for (int i = 0; i < rem_iters; ++i)
    {
        T rem = gx - z * z;
        T q = rem * r * half;  // 10^e counterpart of (rem>>2)×r in SoftFloat
        z = z + q;
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
