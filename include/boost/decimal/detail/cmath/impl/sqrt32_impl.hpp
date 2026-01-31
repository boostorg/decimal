// Copyright 2023 - 2024 Matt Borland
// Copyright 2023 - 2024 Christopher Kormanyos
// Copyright 2025 - 2026 Justin Zhu
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_DECIMAL_DETAIL_CMATH_IMPL_SQRT32_IMPL_HPP
#define BOOST_DECIMAL_DETAIL_CMATH_IMPL_SQRT32_IMPL_HPP

// ============================================================================
// decimal32 sqrt: SoftFloat f32_sqrt style with INTEGER remainder arithmetic
//
// Algorithm:
// 1. Normalize x to gx in [1, 10), extract integer coefficient
// 2. Table lookup with linear interpolation → r ≈ 1/sqrt(gx)
// 3. Compute z = gx * r as initial approximation
// 4. Newton correction using INTEGER arithmetic:
//    - sig_z = integer coefficient of z (7 digits)
//    - rem = sig_x * scale - sig_z² (exact integer subtraction)
//    - correction based on rem sign
// 5. Final rounding check (integer)
// 6. Rescale by 10^(exp/2) and ×√10 if exp was odd
//
// Key improvement: remainder is computed as EXACT INTEGER, no FP rounding error
// ============================================================================

#include <boost/decimal/detail/cmath/impl/sqrt_tables.hpp>
#include <boost/decimal/detail/cmath/frexp10.hpp>
#include <boost/decimal/detail/remove_trailing_zeros.hpp>
#include <boost/decimal/numbers.hpp>

#ifndef BOOST_DECIMAL_BUILD_MODULE
#include <limits>
#include <cstdint>
#endif

namespace boost {
namespace decimal {
namespace detail {

// sqrt for decimal32 (7 decimal digits) with integer remainder arithmetic
template <typename T>
constexpr auto sqrt32_impl(T x, int exp10val) noexcept -> T
{
    constexpr int digits10 = std::numeric_limits<T>::digits10;
    static_assert(digits10 <= 7, "sqrt32_impl is for decimal32 (7 digits)");

    // ---------- Normalize to [1, 10) ----------
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
    // 90-entry table with step = 0.1, perfect decimal alignment
    constexpr T one{1};
    constexpr T ten{10};

    // index = floor((gx - 1) * 10)
    T idx_real = (gx - one) * ten;
    int index = static_cast<int>(idx_real);
    if (index < 0) index = 0;
    if (index >= sqrt_tables::table_size) index = sqrt_tables::table_size - 1;

    // eps = (gx - 1) * 10 - index, in [0, 1)
    T eps = idx_real - T{index};

    // r = 1/sqrt(gx) approximation from table
    T r = sqrt_tables::approx_recip_sqrt_1<T>(index, eps);

    // ---------- Initial z = gx * r ----------
    T z = gx * r;

    // ---------- Newton correction with INTEGER arithmetic ----------
    // Extract integer coefficients for exact remainder calculation
    // gx is in [1, 10), so gx * 10^6 gives 7-digit integer in [10^6, 10^7)
    // z is in [1, sqrt(10)) ≈ [1, 3.16), so z * 10^6 gives 7-digit integer
    
    constexpr std::uint64_t scale = 1000000ULL;  // 10^6
    constexpr std::uint64_t scale_sq = scale * scale;  // 10^12
    
    // sig_gx: integer representation of gx, scaled by 10^6
    std::uint32_t sig_gx = static_cast<std::uint32_t>(gx * T{scale});
    
    // sig_z: integer representation of z, scaled by 10^6
    std::uint32_t sig_z = static_cast<std::uint32_t>(z * T{scale});
    
    // Compute z² as exact integer (sig_z² is at most 14 digits, fits in uint64)
    std::uint64_t z_squared = static_cast<std::uint64_t>(sig_z) * sig_z;
    
    // rem = gx - z² (scaled: sig_gx * 10^6 - sig_z²)
    // sig_gx is scaled by 10^6, z_squared is scaled by 10^12
    // To compare: sig_gx * 10^6 vs z_squared
    std::int64_t rem = static_cast<std::int64_t>(sig_gx) * static_cast<std::int64_t>(scale) 
                     - static_cast<std::int64_t>(z_squared);
    
    // Newton correction: q = rem * r / 2
    // Since r ≈ 1/sqrt(gx) ≈ 1/z, and rem is scaled by 10^12
    // correction to sig_z (scaled by 10^6) = rem / (2 * sig_z)
    if (rem != 0 && sig_z > 0)
    {
        std::int64_t correction = rem / (2 * static_cast<std::int64_t>(sig_z));
        sig_z = static_cast<std::uint32_t>(static_cast<std::int64_t>(sig_z) + correction);
        
        // Recompute remainder after correction
        z_squared = static_cast<std::uint64_t>(sig_z) * sig_z;
        rem = static_cast<std::int64_t>(sig_gx) * static_cast<std::int64_t>(scale) 
            - static_cast<std::int64_t>(z_squared);
    }

    // ---------- Final rounding check (integer) ----------
    // Ensure z² ≤ gx (z is a lower bound)
    // If rem < 0, z is too large, decrease by 1 ULP
    if (rem < 0)
    {
        --sig_z;
    }
    
    // Convert back to decimal type
    z = T{sig_z, -6};  // sig_z * 10^-6

    // ---------- Rescale: sqrt(x) = z × 10^(e/2), ×√10 when e odd ----------
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
