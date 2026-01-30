// Copyright 2023 - 2024 Matt Borland
// Copyright 2023 - 2024 Christopher Kormanyos
// Copyright 2025 - 2026 Justin Zhu
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_DECIMAL_DETAIL_CMATH_IMPL_SQRT128_IMPL_HPP
#define BOOST_DECIMAL_DETAIL_CMATH_IMPL_SQRT128_IMPL_HPP

// ============================================================================
// decimal128 sqrt: SoftFloat f128_sqrt style (faithful translation, 2^e → 10^e)
//
// f128_sqrt.c flow:
// 1. sig32A = sigA.v64 >> 17
// 2. recipSqrt32 = softfloat_approxRecipSqrt32_1(expA, sig32A)
// 3. sig32Z = (sig32A * recipSqrt32) >> 32   ← initial 32-bit approx
// 4. rem = sigA - sig32Z²                    ← first remainder
// 5. q = (rem>>2) * recipSqrt32 >> 32        ← first correction
// 6. for (;;) { if (rem >= 0) break; --q; }  ← adjust until rem >= 0
// 7. qs[1] = q; sig64Z = sig32Z<<32 + q<<3
// 8. (repeat similar for qs[0])              ← second correction + loop
// 9. q = ((rem>>2)*recipSqrt32>>32) + 2      ← third correction
// 10. Assemble sigZ from qs[2], qs[1], qs[0], q
// 11. Final rounding with complex rem check
//
// decimal128 (10^e) equivalent:
// - Same structure, rem = gx - z²
// - Correction: q = rem * r / 2 (10^e version of (rem>>2)*r)
// - Loop: while (rem < 0) { z -= tiny; recompute rem }
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

// sqrt for decimal128 (34 decimal digits, ~113 bits)
// Faithful to f128_sqrt: three corrections, each with adjustment loop
template <typename T>
constexpr auto sqrt128_impl(T x, int exp10val) noexcept -> T
{
    constexpr int digits10 = std::numeric_limits<T>::digits10;
    static_assert(digits10 > 16, "sqrt128_impl is for decimal128 (34 digits)");

    // ---------- Normalize to [1, 10) ----------
    // f128: sigA in [2^112, 2^113), expA&1 determines shift
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
    // f128: recipSqrt32 = softfloat_approxRecipSqrt32_1(expA, sig32A)
    constexpr T one{1};
    constexpr T nine{9};
    constexpr T step{nine / T{sqrt_tables::table_size}};

    T normalized = (gx - one) / step;
    int index = static_cast<int>(normalized);

    T x_lo = one + T{index} * step;
    T eps = (gx - x_lo) / step;

    T r = sqrt_tables::approx_recip_sqrt_1<T>(index, eps);

    // ---------- Initial z = gx * r ----------
    // f128: sig32Z = (sig32A * recipSqrt32) >> 32
    T z = gx * r;

    // ---------- First remainder and correction ----------
    // f128: rem = sigA - sig32Z²
    //       q = ((rem>>2) * recipSqrt32) >> 32
    //       for (;;) { if (!(rem.v64 & 0x8000...)) break; --q; sig64Z -= 8; }
    constexpr T half{5, -1};
    constexpr T zero{0};
    constexpr T tiny{1, -(digits10)};  // 1 ULP

    // f128: "Repeating this loop is a rare occurrence" — cap at 2 per block to avoid hang
    // (decimal rounding can leave rem < 0 without converging in rare cases)
    constexpr int max_rem_adjust = 2;

    // First correction (like qs[2] → qs[1] in f128)
    {
        T rem = gx - z * z;
        T q = rem * r * half;
        z = z + q;
        rem = gx - z * z;
        for (int n = 0; n < max_rem_adjust && rem < zero; ++n)
        {
            z = z - tiny;
            rem = gx - z * z;
        }
    }

    // ---------- Second correction ----------
    {
        T rem = gx - z * z;
        T q = rem * r * half;
        z = z + q;
        rem = gx - z * z;
        for (int n = 0; n < max_rem_adjust && rem < zero; ++n)
        {
            z = z - tiny;
            rem = gx - z * z;
        }
    }

    // ---------- Third correction ----------
    // f128: q = (((rem.v64>>2) * recipSqrt32) >> 32) + 2
    {
        T rem = gx - z * z;
        T q = rem * r * half;
        constexpr T nudge2{2, -(digits10)};  // 10^e equivalent of f128's +2
        z = z + q + nudge2;
        rem = gx - z * z;
        for (int n = 0; n < max_rem_adjust && rem < zero; ++n)
        {
            z = z - tiny;
            rem = gx - z * z;
        }
    }

    // ---------- Final rounding check ----------
    // f128: if ((q & 0xF) <= 2) { complex remainder check, adjust sigZ/sigZExtra }
    // decimal: ensure z² ≤ gx (round toward zero for sqrt)
    {
        T rem = gx - z * z;
        if (rem < zero)
        {
            z = z - tiny;
        }
    }

    // ---------- Rescale: sqrt(x) = z × 10^(e/2), ×√10 when e odd ----------
    // f128: return softfloat_roundPackToF128(0, expZ, sigZ.v64, sigZ.v0, sigZExtra)
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

#endif // BOOST_DECIMAL_DETAIL_CMATH_IMPL_SQRT128_IMPL_HPP
