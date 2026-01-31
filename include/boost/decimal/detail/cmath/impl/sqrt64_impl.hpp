// Copyright 2023 - 2024 Matt Borland
// Copyright 2023 - 2024 Christopher Kormanyos
// Copyright 2025 - 2026 Justin Zhu
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_DECIMAL_DETAIL_CMATH_IMPL_SQRT64_IMPL_HPP
#define BOOST_DECIMAL_DETAIL_CMATH_IMPL_SQRT64_IMPL_HPP

// ============================================================================
// decimal64 sqrt: SoftFloat f64_sqrt style with INTEGER remainder arithmetic
//
// Algorithm:
// 1. Normalize x to gx in [1, 10), extract integer coefficient
// 2. Table lookup with linear interpolation → r ≈ 1/sqrt(gx)
// 3. Compute z = gx * r as initial approximation
// 4. Two Newton corrections using INTEGER arithmetic:
//    - sig_z = integer coefficient of z (16 digits)
//    - rem = sig_x * scale - sig_z² (exact integer, needs uint128)
//    - correction based on rem
// 5. Final rounding check (integer)
// 6. Rescale by 10^(exp/2) and ×√10 if exp was odd
//
// Key improvement: remainder is computed as EXACT INTEGER, no FP rounding error
// Requires: 128-bit integer support (__uint128_t or equivalent)
// ============================================================================

#include <boost/decimal/detail/config.hpp>
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

// sqrt for decimal64 (16 decimal digits) with integer remainder arithmetic
template <typename T>
constexpr auto sqrt64_impl(T x, int exp10val) noexcept -> T
{
    constexpr int digits10 = std::numeric_limits<T>::digits10;
    static_assert(digits10 > 7 && digits10 <= 16, "sqrt64_impl is for decimal64 (16 digits)");

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
    constexpr T one{1};
    constexpr T ten{10};

    T idx_real = (gx - one) * ten;
    int index = static_cast<int>(idx_real);
    if (index < 0) index = 0;
    if (index >= sqrt_tables::table_size) index = sqrt_tables::table_size - 1;

    T eps = idx_real - T{index};
    T r = sqrt_tables::approx_recip_sqrt_1<T>(index, eps);

    // ---------- Initial z = gx * r ----------
    T z = gx * r;

    // ---------- Newton corrections with INTEGER arithmetic ----------
    // gx is in [1, 10), so gx * 10^15 gives 16-digit integer in [10^15, 10^16)
    // z is in [1, sqrt(10)) ≈ [1, 3.16), so z * 10^15 gives 16-digit integer
    
    constexpr std::uint64_t scale = 1000000000000000ULL;  // 10^15

#if defined(BOOST_DECIMAL_HAS_INT128)
    // sig_gx: integer representation of gx, scaled by 10^15
    std::uint64_t sig_gx = static_cast<std::uint64_t>(gx * T{scale});
    
    // sig_z: integer representation of z, scaled by 10^15
    std::uint64_t sig_z = static_cast<std::uint64_t>(z * T{scale});
    
    // First Newton correction
    {
        // z² as exact 128-bit integer (sig_z² is at most 32 digits)
        builtin_uint128_t z_squared = static_cast<builtin_uint128_t>(sig_z) * sig_z;
        
        // rem = gx - z² (scaled: sig_gx * 10^15 - sig_z²)
        // sig_gx scaled by 10^15, z_squared scaled by 10^30
        // To compare: sig_gx * 10^15 vs z_squared
        builtin_int128_t rem = static_cast<builtin_int128_t>(sig_gx) * static_cast<builtin_int128_t>(scale)
                            - static_cast<builtin_int128_t>(z_squared);
        
        // Newton correction: q = rem / (2 * sig_z)
        if (rem != 0 && sig_z > 0)
        {
            builtin_int128_t correction = rem / (2 * static_cast<builtin_int128_t>(sig_z));
            sig_z = static_cast<std::uint64_t>(static_cast<builtin_int128_t>(sig_z) + correction);
        }
    }
    
    // Second Newton correction
    {
        builtin_uint128_t z_squared = static_cast<builtin_uint128_t>(sig_z) * sig_z;
        builtin_int128_t rem = static_cast<builtin_int128_t>(sig_gx) * static_cast<builtin_int128_t>(scale)
                             - static_cast<builtin_int128_t>(z_squared);
        
        if (rem != 0 && sig_z > 0)
        {
            builtin_int128_t correction = rem / (2 * static_cast<builtin_int128_t>(sig_z));
            sig_z = static_cast<std::uint64_t>(static_cast<builtin_int128_t>(sig_z) + correction);
        }
    }
    
    // Final rounding check
    {
        builtin_uint128_t z_squared = static_cast<builtin_uint128_t>(sig_z) * sig_z;
        builtin_int128_t rem = static_cast<builtin_int128_t>(sig_gx) * static_cast<builtin_int128_t>(scale)
                             - static_cast<builtin_int128_t>(z_squared);
        
        // Ensure z² ≤ gx (z is a lower bound)
        if (rem < 0)
        {
            --sig_z;
        }
    }
    
    // Convert back to decimal type
    z = T{sig_z, -15};  // sig_z * 10^-15

#else
    // Fallback: use floating-point Newton iterations when no int128
    constexpr T half{5, -1};
    
    // First Newton correction
    {
        T rem = gx - z * z;
        T q = rem * r * half;
        z = z + q;
    }
    
    // Second Newton correction
    {
        T rem = gx - z * z;
        T q = rem * r * half;
        z = z + q;
    }
    
    // Final rounding check
    {
        T rem = gx - z * z;
        constexpr T zero{0};
        constexpr T tiny{1, -(digits10)};
        if (rem < zero)
        {
            z = z - tiny;
        }
    }
#endif

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

#endif // BOOST_DECIMAL_DETAIL_CMATH_IMPL_SQRT64_IMPL_HPP
