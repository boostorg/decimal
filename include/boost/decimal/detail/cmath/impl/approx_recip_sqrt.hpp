// Copyright 2025 - 2026 Justin Zhu
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_DECIMAL_DETAIL_CMATH_IMPL_APPROX_RECIP_SQRT_HPP
#define BOOST_DECIMAL_DETAIL_CMATH_IMPL_APPROX_RECIP_SQRT_HPP

// ============================================================================
// Integer-based 1/sqrt(x) approximation with Newton refinement
//
// Based on SoftFloat's softfloat_approxRecipSqrt32_1 but adapted for base-10.
//
// Algorithm:
// 1. Table lookup + linear interpolation → r0 (~10 bits)
// 2. Compute sigma = 1 - x * r0² (error term)
// 3. Newton refinement: r = r0 * (1 + σ/2 + 3σ²/8)
//
// This gives ~20 bits of precision for sqrt(x) after one Newton step.
// ============================================================================

#include <boost/decimal/detail/cmath/impl/sqrt_tables.hpp>

#ifndef BOOST_DECIMAL_BUILD_MODULE
#include <cstdint>
#endif

namespace boost {
namespace decimal {
namespace detail {

// ============================================================================
// approx_recip_sqrt32: for decimal32 (7 digits)
//
// Input:
//   sig: significand in [10^6, 10^7) representing value in [1, 10)
//   oddExp: 1 if original exponent was odd, 0 otherwise
//
// Output:
//   Approximation of 1/sqrt(sig/10^6) scaled by 10^7
//   (~20 bits of precision after Newton refinement)
// ============================================================================
inline constexpr std::uint32_t approx_recip_sqrt32(std::uint32_t sig, unsigned int /* oddExp */) noexcept
{
    // sig is in [10^6, 10^7) representing x in [1, 10)
    // We want 1/sqrt(x) where x = sig / 10^6

    // Step 1: Table lookup
    // index = (sig/10^5) - 10, clamped to [0, 89]
    // (sig/10^5 gives first two digits, e.g., 12 for sig=1.2*10^6)
    int index = static_cast<int>(sig / 100000U) - 10;
    if (index < 0) index = 0;
    if (index >= sqrt_tables::table_size) index = sqrt_tables::table_size - 1;

    // eps: position within bin, 16-bit scaled
    // sig_in_bin = sig - (index + 10) * 10^5
    // eps = sig_in_bin / 10^5 * 65536 ≈ sig_in_bin * 65536 / 100000
    std::uint32_t base_sig = static_cast<std::uint32_t>(index + 10) * 100000U;
    std::uint32_t sig_in_bin = sig - base_sig;
    std::uint32_t eps16 = (sig_in_bin * 65536U) / 100000U;  // 16-bit eps

    // r0 from table (scaled by 10^16)
    std::uint64_t r0_scaled = sqrt_tables::approxRecipSqrt_1k0s[index]
        - ((sqrt_tables::approxRecipSqrt_1k1s[index] * static_cast<std::uint64_t>(eps16)) >> 16);

    // Convert to working scale: r0 ≈ 10^16 / sqrt(x)
    // We want final result scaled by 10^7, so:
    // r0_work = r0_scaled / 10^9 ≈ 10^7 / sqrt(x)
    std::uint32_t r0 = static_cast<std::uint32_t>(r0_scaled / 1000000000ULL);

    // Step 2: Newton refinement
    // sigma = 1 - x * r0² where x = sig/10^6, r0 is scaled by 10^7
    // x * r0² = (sig/10^6) * (r0/10^7)² = sig * r0² / 10^20
    // We need to compute this carefully to avoid overflow

    // r0² scaled by 10^14
    std::uint64_t r0_sq = static_cast<std::uint64_t>(r0) * r0;  // scaled by 10^14

    // sig * r0² / 10^14 gives (sig/10^6) * r0² / 10^8 = x * r0² / 10^8
    // Should be ≈ 10^6 (since r0 ≈ 10^7/sqrt(x), r0² ≈ 10^14/x, x*r0² ≈ 10^14)
    std::uint64_t x_r0_sq = (static_cast<std::uint64_t>(sig) * r0_sq) / 100000000000000ULL;

    // sigma = 1 - x*r0²/10^6 (sigma scaled by 10^6 for working precision)
    // x_r0_sq is already ≈ 10^6 when r0 is accurate
    std::int32_t sigma = 1000000 - static_cast<std::int32_t>(x_r0_sq);

    // r = r0 * (1 + sigma/2) where sigma is scaled by 10^6
    // r = r0 + r0 * sigma / (2 * 10^6)
    std::int32_t correction = static_cast<std::int32_t>((static_cast<std::int64_t>(r0) * sigma) / 2000000);
    std::uint32_t r = static_cast<std::uint32_t>(static_cast<std::int32_t>(r0) + correction);

    // Clamp to valid range
    if (r < 3162278U) r = 3162278U;  // 10^7 / sqrt(10) ≈ 3162278
    if (r > 10000000U) r = 10000000U;  // 10^7 / sqrt(1) = 10^7

    return r;
}

// ============================================================================
// approx_recip_sqrt64: for decimal64 (16 digits)
//
// Input:
//   sig: significand in [10^15, 10^16) representing value in [1, 10)
//   oddExp: 1 if original exponent was odd, 0 otherwise
//
// Output:
//   Approximation of 1/sqrt(sig/10^15) scaled by 10^16
//   (~32 bits of precision after Newton refinement)
// ============================================================================
inline constexpr std::uint64_t approx_recip_sqrt64(std::uint64_t sig, unsigned int /* oddExp */) noexcept
{
    // sig is in [10^15, 10^16) representing x in [1, 10)

    // Step 1: Table lookup
    // index = (sig/10^14) - 10, clamped to [0, 89]
    int index = static_cast<int>(sig / 100000000000000ULL) - 10;
    if (index < 0) index = 0;
    if (index >= sqrt_tables::table_size) index = sqrt_tables::table_size - 1;

    // eps: position within bin, 16-bit scaled
    std::uint64_t base_sig = static_cast<std::uint64_t>(index + 10) * 100000000000000ULL;
    std::uint64_t sig_in_bin = sig - base_sig;
    // sig_in_bin is in [0, 10^14), scale to 16 bits
    std::uint32_t eps16 = static_cast<std::uint32_t>((sig_in_bin * 65536ULL) / 100000000000000ULL);

    // r0 from table (scaled by 10^16)
    std::uint64_t r0 = sqrt_tables::approxRecipSqrt_1k0s[index]
        - ((sqrt_tables::approxRecipSqrt_1k1s[index] * static_cast<std::uint64_t>(eps16)) >> 16);

    // Step 2: Newton refinement (first iteration)
    // sigma = 1 - x * r0² where x = sig/10^15, r0 is scaled by 10^16
    // x * r0² = (sig/10^15) * (r0/10^16)² = sig * r0² / 10^47
    //
    // To avoid overflow, we compute in steps:
    // r0² (needs uint128 or split computation)

    // For 64-bit only: use approximation
    // r0_hi = r0 / 10^8, r0_lo = r0 % 10^8
    // r0² ≈ r0_hi² * 10^16 + 2 * r0_hi * r0_lo * 10^8 + r0_lo²

    std::uint64_t r0_hi = r0 / 100000000ULL;
    std::uint64_t r0_lo = r0 % 100000000ULL;

    // r0² / 10^16 (keeping just the high part for sigma calculation)
    std::uint64_t r0_sq_hi = r0_hi * r0_hi + (2 * r0_hi * r0_lo) / 100000000ULL;

    // x * r0² / 10^31 = (sig/10^15) * (r0²/10^16)
    // = sig * r0_sq_hi / 10^15
    std::uint64_t x_r0_sq = (sig / 1000000000ULL) * (r0_sq_hi / 1000000ULL);
    // This gives us x*r0²/10^31 approximately

    // We expect x*r0² ≈ 10^32 (since r0 ≈ 10^16/sqrt(x))
    // sigma = (10^32 - x*r0²) / 10^32, scaled for working precision

    // Simplified: compute the correction directly
    // r_new = r0 + r0 * (1 - x*r0²) / 2 = r0 * (3 - x*r0²) / 2

    // For numerical stability, we use the formula:
    // sigma_scaled = 10^15 - (sig * r0_sq_hi / 10^17)
    std::uint64_t prod = (sig / 100ULL) * (r0_sq_hi / 100000000ULL);  // approximate
    std::int64_t sigma_scaled = 1000000000000000LL - static_cast<std::int64_t>(prod);

    // correction = r0 * sigma_scaled / (2 * 10^15)
    std::int64_t correction = (static_cast<std::int64_t>(r0 / 1000ULL) * (sigma_scaled / 1000LL)) / 2000000000LL;
    
    std::uint64_t r = static_cast<std::uint64_t>(static_cast<std::int64_t>(r0) + correction * 1000);

    // Second Newton iteration for more precision
    // Recompute with updated r
    r0_hi = r / 100000000ULL;
    r0_lo = r % 100000000ULL;
    r0_sq_hi = r0_hi * r0_hi + (2 * r0_hi * r0_lo) / 100000000ULL;
    prod = (sig / 100ULL) * (r0_sq_hi / 100000000ULL);
    sigma_scaled = 1000000000000000LL - static_cast<std::int64_t>(prod);
    correction = (static_cast<std::int64_t>(r / 1000ULL) * (sigma_scaled / 1000LL)) / 2000000000LL;
    r = static_cast<std::uint64_t>(static_cast<std::int64_t>(r) + correction * 1000);

    // Clamp to valid range
    if (r < 3162277660168379ULL) r = 3162277660168379ULL;  // 10^16 / sqrt(10)
    if (r > 10000000000000000ULL) r = 10000000000000000ULL;

    return r;
}

} // namespace detail
} // namespace decimal
} // namespace boost

#endif // BOOST_DECIMAL_DETAIL_CMATH_IMPL_APPROX_RECIP_SQRT_HPP
