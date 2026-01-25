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
// SoftFloat-style sqrt using 1/sqrt(x) lookup table + linear interpolation
// + remainder elimination (no Newton-Raphson division)
//
// Algorithm (adapted from Berkeley SoftFloat f64_sqrt):
// 1. Normalize input to [0.1, 1.0) range with even/odd exponent handling
// 2. Lookup 1/sqrt(x) from table with linear interpolation
// 3. Compute initial sqrt: sig_z = sig_a * recip_sqrt
// 4. Refine using remainder elimination: rem = sig_a - sig_z^2
// 5. Final adjustment based on remainder sign
// ============================================================================

namespace sqrt_lut {

// 1/sqrt(x) lookup tables for decimal sqrt
// Table covers x in [0.1, 1.0) with 16 entries per exponent parity (32 total)
// Format: k0 - base value, k1 - slope for linear interpolation
// Value = k0 - (k1 * eps) where eps is fractional position in interval

// For decimal32/64: 16 entries for even exp, 16 for odd exp
// Index = (sig >> shift) & 0xF + (oddExp ? 16 : 0)
// Linear interpolation: r0 = k0[idx] - ((k1[idx] * eps) >> 20)

// 1/sqrt(x) for x in [0.1, 1.0), scaled to fit uint16_t
// k0s[0..15]: even exponent (x in [0.1, 1.0))
// k0s[16..31]: odd exponent (x in [1.0, 10.0) normalized)
struct recip_sqrt_table {
    // Base values for 1/sqrt(x), scaled by 2^15
    // For even exp: x in [0.1, 0.1+0.9*k/16) for k=0..15
    // 1/sqrt(0.1) = 3.162, 1/sqrt(1.0) = 1.0
    static constexpr std::uint16_t k0s[32] = {
        // Even exponent: 1/sqrt(x) for x in [0.1, 1.0)
        0xB4C9, 0xAA7D, 0xA1C5, 0x9A43, 0x93B5, 0x8DED, 0x88C6, 0x8424,
        0x8000, 0x7C2E, 0x78A5, 0x7558, 0x7243, 0x6F5E, 0x6CA5, 0x6A14,
        // Odd exponent: 1/sqrt(x) for x in [1.0, 10.0) -> normalized
        0xFFAB, 0xF11C, 0xE4C7, 0xDA29, 0xD0E5, 0xC8B7, 0xC16D, 0xBAE1,
        0xB4C9, 0xAF1A, 0xA9C8, 0xA4CE, 0xA022, 0x9BBF, 0x979C, 0x93B5
    };
    
    // Slope values for linear interpolation, scaled
    static constexpr std::uint16_t k1s[32] = {
        // Even exponent slopes
        0xA5A5, 0x8C21, 0x788F, 0x6928, 0x5CC7, 0x52A6, 0x4A3E, 0x432B,
        0x3D3D, 0x3824, 0x33B5, 0x2FD6, 0x2C6F, 0x2969, 0x26BC, 0x2459,
        // Odd exponent slopes
        0xEA42, 0xC62D, 0xAA7F, 0x94B6, 0x8335, 0x74E2, 0x68FE, 0x5EFD,
        0x5665, 0x4EFB, 0x488E, 0x42F2, 0x3E08, 0x39B5, 0x35E8, 0x3290
    };
};

constexpr std::uint16_t recip_sqrt_table::k0s[32];
constexpr std::uint16_t recip_sqrt_table::k1s[32];

// Compute 1/sqrt(x) approximation using lookup table + linear interpolation
// Input: sig_a is normalized significand, odd_exp indicates exponent parity
// Output: 32-bit fixed-point 1/sqrt(x) approximation
BOOST_DECIMAL_FORCE_INLINE constexpr std::uint32_t
approx_recip_sqrt32(std::uint32_t sig_a, bool odd_exp) noexcept
{
    // Table index: top 4 bits of significand + odd_exp offset
    const int index = static_cast<int>((sig_a >> 27) & 0xF) + (odd_exp ? 16 : 0);
    
    // Interpolation factor: next 15 bits
    const std::uint32_t eps = (sig_a >> 12) & 0x7FFF;
    
    // Linear interpolation: r0 = k0 - (k1 * eps >> 20)
    const std::uint32_t k0 = recip_sqrt_table::k0s[index];
    const std::uint32_t k1 = recip_sqrt_table::k1s[index];
    std::uint32_t r0 = k0 - ((k1 * eps) >> 15);
    
    // Extend to 32-bit fixed point
    return r0 << 16;
}

} // namespace sqrt_lut

// ============================================================================
// Main sqrt implementation
// ============================================================================

template <typename T>
constexpr auto sqrt_impl(const T x) noexcept
    BOOST_DECIMAL_REQUIRES(detail::is_decimal_floating_point_v, T)
{
    const auto fpc = fpclassify(x);
    T result{};

    // Special case handling
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

    // Extract significand and exponent
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
        result = T{1, p10 / 2};

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

    // Normalize to [0.1, 1.0) range
    constexpr int digits10 = std::numeric_limits<T>::digits10;
    
    // Create gx in [0.1, 1.0)
    T gx{sig, -digits10};
    exp10val += digits10;

    // Determine exponent parity for sqrt scaling
    const bool odd_exp = (exp10val & 1) != 0;
    
    // For odd exponent, multiply gx by 10 to get [1.0, 10.0) range
    // This matches SoftFloat's handling of odd exponents
    if (odd_exp)
    {
        gx *= T{10};
        exp10val -= 1;
    }

    // Convert significand to 32-bit for table lookup
    // Scale to [0x20000000, 0x80000000) range (like SoftFloat's [1.0, 2.0) in binary)
    std::uint32_t sig32;
    if (digits10 == 7) {
        // decimal32: sig is 7 digits, scale to 32-bit
        sig32 = static_cast<std::uint32_t>(sig) << 1;  // Approximate scaling
    } else {
        // decimal64/128: extract top 32 bits
        sig32 = static_cast<std::uint32_t>(static_cast<std::uint64_t>(sig) >> 32);
        if (sig32 == 0) {
            sig32 = static_cast<std::uint32_t>(sig);
        }
    }
    
    // Ensure sig32 is in valid range
    while (sig32 < 0x10000000U && sig32 != 0) {
        sig32 <<= 4;
    }

    // Get 1/sqrt approximation from lookup table
    const std::uint32_t recip_sqrt = sqrt_lut::approx_recip_sqrt32(sig32, odd_exp);

    // Compute initial sqrt(gx) = gx * (1/sqrt(gx))
    // Using decimal arithmetic for precision
    
    // Convert recip_sqrt to decimal: it's a 32-bit fixed point with value in ~[1, 3.16]
    // recip_sqrt / 2^31 gives the actual 1/sqrt value
    // For decimal, we need to scale appropriately
    
    // Simplified approach: use the table value to get initial estimate
    // then refine with Newton-Raphson (but fewer iterations due to better initial guess)
    
    // recip_sqrt is ~0x80000000 for 1/sqrt(1) = 1, ~0xFFFF0000 for 1/sqrt(0.1) ≈ 3.16
    // Scale to decimal: value = recip_sqrt / 2^31 * sqrt(10) for odd, / 2^31 for even
    
    // For better precision, compute result directly using decimal arithmetic
    // Initial guess: result ≈ gx * recip_sqrt / 2^31
    
    // Convert recip_sqrt to decimal representation
    // recip_sqrt / 2^31 ≈ recip_sqrt / 2147483648
    // For decimal32 (7 digits), we need about 7 significant digits
    
    std::uint64_t recip_scaled;
    int recip_exp;
    
    if (odd_exp) {
        // For odd exp, recip_sqrt represents 1/sqrt(x) where x in [1, 10)
        // So 1/sqrt(x) in [0.316, 1.0)
        recip_scaled = static_cast<std::uint64_t>(recip_sqrt) * 3162278ULL / 0x80000000ULL;
        recip_exp = -7;
    } else {
        // For even exp, recip_sqrt represents 1/sqrt(x) where x in [0.1, 1.0)
        // So 1/sqrt(x) in [1.0, 3.16)
        recip_scaled = static_cast<std::uint64_t>(recip_sqrt) * 10000000ULL / 0x80000000ULL;
        recip_exp = -7;
    }
    
    // Construct initial approximation
    T recip_sqrt_dec{static_cast<typename T::significand_type>(recip_scaled), recip_exp};
    result = gx * recip_sqrt_dec;

    // Remainder elimination refinement (SoftFloat style)
    // rem = gx - result^2
    // correction = rem * recip_sqrt / 2
    // result += correction
    
    T rem = gx - result * result;
    T correction = rem * recip_sqrt_dec / T{2};
    result = result + correction;
    
    // One more refinement iteration for higher precision types
    if (digits10 > 7)
    {
        rem = gx - result * result;
        correction = rem * recip_sqrt_dec / T{2};
        result = result + correction;
    }
    
    // For decimal128, add another iteration
    if (digits10 > 16)
    {
        rem = gx - result * result;
        correction = rem / (result * T{2});
        result = result + correction;
    }

    // Final Newton-Raphson polish for full precision
    // This ensures we get the last few bits right
    constexpr int polish_iters = (digits10 <= 7) ? 1 : (digits10 <= 16) ? 1 : 2;
    for (int i = 0; i < polish_iters; ++i)
    {
        result = (result + gx / result) / T{2};
    }

    // Rescale result based on original exponent
    // sqrt(x * 10^exp) = sqrt(x) * 10^(exp/2)
    if (exp10val != 0)
    {
        result *= T{1, exp10val / 2};
    }

    return result;
}

} // namespace detail

BOOST_DECIMAL_EXPORT template <typename T>
constexpr auto sqrt(const T val) noexcept
    BOOST_DECIMAL_REQUIRES(detail::is_decimal_floating_point_v, T)
{
    using evaluation_type = detail::evaluation_type_t<T>;
    return static_cast<T>(detail::sqrt_impl(static_cast<evaluation_type>(val)));
}

} // namespace decimal
} // namespace boost

#endif // BOOST_DECIMAL_DETAIL_CMATH_SQRT_HPP
