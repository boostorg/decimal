// Copyright 2023 - 2024 Matt Borland
// Copyright 2023 - 2024 Christopher Kormanyos
// Copyright 2025 - 2026 Justin Zhu
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_DECIMAL_DETAIL_CMATH_IMPL_SQRT64_IMPL_HPP
#define BOOST_DECIMAL_DETAIL_CMATH_IMPL_SQRT64_IMPL_HPP

// ============================================================================
// decimal64 sqrt: SoftFloat f64_sqrt style (faithful translation, 2^e → 10^e)
//
// f64_sqrt.c flow:
// 1. sig32A = sigA >> 21
// 2. recipSqrt32 = softfloat_approxRecipSqrt32_1(expA, sig32A)
// 3. sig32Z = (sig32A * recipSqrt32) >> 32   ← initial 32-bit approx
// 4. rem = sigA - sig32Z²                    ← remainder (exact integer)
// 5. q = ((rem>>2) * recipSqrt32) >> 32      ← ONE correction (NO loop!)
// 6. sigZ = (sig32Z<<32 | 1<<5) + (q<<3)     ← assemble 64-bit result
// 7. if ((sigZ & 0x1FF) < 0x22) { ... }      ← final rounding check
//
// decimal64 (10^e) equivalent:
// - Same structure: initial z, one remainder correction, final rounding
// - rem = gx - z²
// - q = rem * r / 2
// - Final check: if rem < 0, adjust z
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

// sqrt for decimal64 (16 decimal digits, ~53 bits)
// Faithful to f64_sqrt: ONE remainder correction, no loop
template <typename T>
constexpr auto sqrt64_impl(T x, int exp10val) noexcept -> T
{
    constexpr int digits10 = std::numeric_limits<T>::digits10;
    static_assert(digits10 > 7 && digits10 <= 16, "sqrt64_impl is for decimal64 (16 digits)");

    // ---------- Normalize to [1, 10) ----------
    // f64: sigA in [2^52, 2^53), expA&1 determines shift
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
    // f64: recipSqrt32 = softfloat_approxRecipSqrt32_1(expA, sig32A)
    constexpr T one{1};
    constexpr T nine{9};
    constexpr T step{nine / T{sqrt_tables::table_size}};

    T normalized = (gx - one) / step;
    int index = static_cast<int>(normalized);

    T x_lo = one + T{index} * step;
    T eps = (gx - x_lo) / step;

    T r = sqrt_tables::approx_recip_sqrt_1<T>(index, eps);

    // ---------- Initial z = gx * r ----------
    // f64: sig32Z = (sig32A * recipSqrt32) >> 32
    T z = gx * r;

    // ---------- ONE remainder correction (f64-style, NO loop) ----------
    // f64: rem = sigA - sig32Z²
    //      q = ((rem>>2) * recipSqrt32) >> 32
    //      sigZ = (sig32Z<<32 | 1<<5) + (q<<3)
    constexpr T half{5, -1};
    {
        T rem = gx - z * z;
        T q = rem * r * half;
        z = z + q;
    }

    // ---------- Final rounding check (f64-style) ----------
    // f64: if ((sigZ & 0x1FF) < 0x22) {
    //        rem = (sigA<<52) - shiftedSigZ * shiftedSigZ;
    //        if (rem & 0x8000...) --sigZ;   ← rem < 0: decrease z
    //        else if (rem) sigZ |= 1;      ← rem > 0: add 1 ULP
    //      }
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
            z = z + tiny;  // f64: if (rem) sigZ |= 1
        }
    }

    // ---------- Rescale: sqrt(x) = z × 10^(e/2), ×√10 when e odd ----------
    // f64: return softfloat_roundPackToF64(0, expZ, sigZ)
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

#endif // BOOST_DECIMAL_DETAIL_CMATH_IMPL_SQRT64_IMPL_HPP
