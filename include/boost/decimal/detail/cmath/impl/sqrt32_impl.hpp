// Copyright 2023 - 2024 Matt Borland
// Copyright 2023 - 2024 Christopher Kormanyos
// Copyright 2025 - 2026 Justin Zhu
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_DECIMAL_DETAIL_CMATH_IMPL_SQRT32_IMPL_HPP
#define BOOST_DECIMAL_DETAIL_CMATH_IMPL_SQRT32_IMPL_HPP

// ============================================================================
// decimal32 sqrt: SoftFloat f32_sqrt style (faithful translation, 2^e → 10^e)
//
// f32_sqrt.c flow:
// 1. sigZ = (sigA * softfloat_approxRecipSqrt32_1(expA, sigA)) >> 32
// 2. if (expA) sigZ >>= 1
// 3. sigZ += 2                               ← nudge (NO remainder loop!)
// 4. if ((sigZ & 0x3F) < 2) {                ← final rounding check
//      shiftedSigZ = sigZ >> 2;
//      negRem = shiftedSigZ * shiftedSigZ;
//      if (negRem & 0x80000000) sigZ |= 1;
//      else if (negRem) --sigZ;
//    }
// 5. return softfloat_roundPackToF32(0, expZ, sigZ)
//
// decimal32 (10^e) equivalent:
// - Same structure: initial z, nudge, final rounding check
// - NO remainder correction loop at all
// ============================================================================

#include <boost/decimal/detail/cmath/impl/sqrt_tables.hpp>
#include <boost/decimal/detail/cmath/frexp10.hpp>
#include <boost/decimal/detail/remove_trailing_zeros.hpp>
#include <boost/decimal/numbers.hpp>

#ifndef BOOST_DECIMAL_BUILD_MODULE
#include <limits>
#endif

namespace boost {
namespace decimal {
namespace detail {

// sqrt for decimal32 (7 decimal digits, ~23 bits)
// Faithful to f32_sqrt: nudge only, no remainder correction loop
template <typename T>
constexpr auto sqrt32_impl(T x, int exp10val) noexcept -> T
{
    constexpr int digits10 = std::numeric_limits<T>::digits10;
    static_assert(digits10 <= 7, "sqrt32_impl is for decimal32 (7 digits)");

    // ---------- Normalize to [1, 10) ----------
    // f32: sigA in [2^23, 2^24), expA&1 determines shift
    // decimal: gx in [1, 10)
    T gx{x};

    while (gx >= T{10})
    {
        gx /= T{10};
        ++exp10val;
    }
    while (gx < T{1})
    {
        gx *= T{10};
        --exp10val;
    }

    // ---------- Table lookup: r = k0 - k1*eps ----------
    // f32: recipSqrt32 = softfloat_approxRecipSqrt32_1(expA, sigA)
    constexpr T one{1};
    constexpr T nine{9};
    constexpr T step{nine / T{sqrt_tables::table_size}};

    T normalized = (gx - one) / step;
    int index = static_cast<int>(normalized);

    T x_lo = one + T{index} * step;
    T eps = (gx - x_lo) / step;

    T r = sqrt_tables::approx_recip_sqrt_1<T>(index, eps);

    // ---------- Initial z = gx * r ----------
    // f32: sigZ = (sigA * recipSqrt32) >> 32
    T z = gx * r;

    // ---------- Nudge (f32-style, NO remainder loop) ----------
    // f32: sigZ += 2
    // decimal: add small positive offset to ensure lower bound
    constexpr T nudge{2, -(digits10)};  // 2e-7 for decimal32
    z = z + nudge;

    // ---------- Final rounding check (f32-style) ----------
    // f32: if ((sigZ & 0x3F) < 2) {
    //        negRem = (sigZ>>2)²;  (different scale)
    //        if (negRem & 0x80000000) sigZ |= 1;   ← round up
    //        else if (negRem) --sigZ;              ← round down
    //      }
    // decimal (2^e→10^e): rem = gx - z²; rem<0 → z too large → z-=tiny; rem>0 → z+=tiny
    constexpr T zero{0};
    constexpr T tiny{1, -(digits10)};  // 1 ULP
    {
        T rem = gx - z * z;
        if (rem < zero)
        {
            z = z - tiny;
        }
        else if (rem > zero)
        {
            z = z + tiny;  // f32 sigZ|=1 branch: round up when remainder positive
        }
    }

    // ---------- Rescale: sqrt(x) = z × 10^(e/2), ×√10 when e odd ----------
    // f32: return softfloat_roundPackToF32(0, expZ, sigZ)
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
} // namespace decimal
} // namespace boost

#endif // BOOST_DECIMAL_DETAIL_CMATH_IMPL_SQRT32_IMPL_HPP
