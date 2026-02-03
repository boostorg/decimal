// Copyright 2023 - 2024 Matt Borland
// Copyright 2023 - 2024 Christopher Kormanyos
// Copyright 2025 - 2026 Justin Zhu
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_DECIMAL_DETAIL_CMATH_IMPL_SQRT64_IMPL_HPP
#define BOOST_DECIMAL_DETAIL_CMATH_IMPL_SQRT64_IMPL_HPP

// ============================================================================
// decimal64 sqrt: SoftFloat f64_sqrt style with INTEGER arithmetic throughout
//
// Algorithm:
// 1. Normalize x to gx in [1, 10), get sig_gx = gx * 10^15 as integer
// 2. Call approx_recip_sqrt64 → r_scaled ≈ 10^16 / sqrt(gx) (integer, ~48 bits)
// 3. Compute sig_z = sig_gx * r_scaled / 10^16 ≈ sqrt(gx) * 10^15
// 4. Newton corrections using exact integer remainder (needs 128-bit)
// 5. Final rounding check (integer)
// 6. Rescale by 10^(exp/2) and ×√10 if exp was odd
//
// Key improvement: ALL arithmetic is integer, no floating-point until final result
// Requires: 128-bit integer support (__uint128_t or equivalent) for full precision
// ============================================================================

#include <boost/decimal/detail/config.hpp>
#include <boost/decimal/detail/cmath/impl/approx_recip_sqrt.hpp>
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

// sqrt for decimal64 (16 decimal digits) with pure integer arithmetic
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

    // ---------- Convert to integer representation ----------
    // gx in [1, 10), sig_gx = gx * 10^15 in [10^15, 10^16)
    constexpr std::uint64_t scale15 = 1000000000000000ULL;   // 10^15
    constexpr std::uint64_t scale16 = 10000000000000000ULL;  // 10^16
    
    std::uint64_t sig_gx = static_cast<std::uint64_t>(gx * T{scale15});
    
    // ---------- Get 1/sqrt approximation using integer function ----------
    // r_scaled = approx_recip_sqrt64(sig_gx) ≈ 10^16 / sqrt(gx)
    // This has ~48 bits precision after internal Newton iterations
    std::uint64_t r_scaled = approx_recip_sqrt64(sig_gx, static_cast<unsigned int>(exp10val & 1));

#if defined(BOOST_DECIMAL_HAS_INT128)
    // ---------- Compute initial z = sqrt(gx) using 128-bit ----------
    // sig_z = sig_gx * r_scaled / 10^16
    //       = (gx * 10^15) * (10^16 / sqrt(gx)) / 10^16
    //       = gx * 10^15 / sqrt(gx) = sqrt(gx) * 10^15
    builtin_uint128_t product = static_cast<builtin_uint128_t>(sig_gx) * r_scaled;
    std::uint64_t sig_z = static_cast<std::uint64_t>(product / scale16);
    
    // ---------- Newton correction with exact integer remainder ----------
    // rem = sig_gx * 10^15 - sig_z² (exact 128-bit integer)
    {
        builtin_uint128_t z_squared = static_cast<builtin_uint128_t>(sig_z) * sig_z;
        builtin_int128_t rem = static_cast<builtin_int128_t>(sig_gx) * static_cast<builtin_int128_t>(scale15)
                             - static_cast<builtin_int128_t>(z_squared);
        
        if (rem != 0 && sig_z > 0)
        {
            builtin_int128_t correction = rem / (2 * static_cast<builtin_int128_t>(sig_z));
            sig_z = static_cast<std::uint64_t>(static_cast<builtin_int128_t>(sig_z) + correction);
        }
    }
    
    // Second Newton correction for full precision
    {
        builtin_uint128_t z_squared = static_cast<builtin_uint128_t>(sig_z) * sig_z;
        builtin_int128_t rem = static_cast<builtin_int128_t>(sig_gx) * static_cast<builtin_int128_t>(scale15)
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
        builtin_int128_t rem = static_cast<builtin_int128_t>(sig_gx) * static_cast<builtin_int128_t>(scale15)
                             - static_cast<builtin_int128_t>(z_squared);
        
        if (rem < 0)
        {
            --sig_z;
        }
    }
    
    // Convert back to decimal type
    T z{sig_z, -15};  // sig_z * 10^-15

#else
    // ---------- Fallback without 128-bit: use decimal type Newton iterations ----------
    // Initial approximation: z = gx * r where r = r_scaled * 10^-16 ≈ 1/sqrt(gx)
    // z ≈ gx / sqrt(gx) = sqrt(gx)
    T r{r_scaled, -16};  // r ≈ 1/sqrt(gx)
    T z = gx * r;        // z ≈ sqrt(gx)
    
    constexpr T half{5, -1};
    
    // Newton iterations: z_new = z + (gx - z²) * r / 2
    for (int i = 0; i < 3; ++i)
    {
        T rem = gx - z * z;
        T q = rem * r * half;
        z = z + q;
    }
    
    // Final rounding check
    {
        T rem = gx - z * z;
        constexpr T zero{0};
        constexpr T tiny{1, -digits10};
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
