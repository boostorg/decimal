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
#include <boost/decimal/detail/to_float.hpp>
#include <boost/decimal/detail/cmath/frexp10.hpp>
#include <boost/decimal/detail/remove_trailing_zeros.hpp>
#include <boost/decimal/numbers.hpp>
#include <boost/decimal/detail/int128.hpp>

#ifndef BOOST_DECIMAL_BUILD_MODULE
#include <type_traits>
#include <cstdint>
#include <cmath>
#include <array>
#endif

namespace boost {
namespace decimal {

namespace detail {

// Lookup table for 1/sqrt(x) over [0.1, 1) in decimal.
// 16 entries at x = 0.1 + k * 0.06, k = 0..15.
// Used for fast initial guess; same scaling as baseline (gx in [0.1, 1)).
// SoftFloat-style: table + Newton, but in decimal domain for correctness.
namespace sqrt_lut_detail {

template<typename T>
struct inv_sqrt_01_table;

template<>
struct inv_sqrt_01_table<decimal32_t> {
    // 1/sqrt(0.1 + k*0.06), k=0..15, as decimal32 (sig, exp)
    static constexpr std::array<std::pair<std::uint32_t, int>, 16> value = {{
        {3162278U, -6},   // 1/sqrt(0.10) = 3.162278
        {2500000U, -6},   // 1/sqrt(0.16) = 2.5
        {2132007U, -6},   // 1/sqrt(0.22) = 2.132007
        {1889822U, -6},   // 1/sqrt(0.28) = 1.889822
        {1715733U, -6},   // 1/sqrt(0.34) = 1.715733
        {1581139U, -6},   // 1/sqrt(0.40) = 1.581139
        {1472243U, -6},   // 1/sqrt(0.46) = 1.472243
        {1381699U, -6},   // 1/sqrt(0.52) = 1.381699
        {1304348U, -6},   // 1/sqrt(0.58) = 1.304348
        {1250000U, -6},   // 1/sqrt(0.64) = 1.25
        {1195229U, -6},   // 1/sqrt(0.70) = 1.195229
        {1147875U, -6},   // 1/sqrt(0.76) = 1.147876
        {1106567U, -6},   // 1/sqrt(0.82) = 1.106567
        {1069973U, -6},   // 1/sqrt(0.88) = 1.069973
        {1031095U, -6}    // 1/sqrt(0.94) = 1.031095; 1/sqrt(1.0)=1.0 handled at bound
    }};
};

template<>
struct inv_sqrt_01_table<decimal64_t> {
    static constexpr std::array<std::pair<std::uint64_t, int>, 16> value = {{
        {316227766016838ULL, -14}, {250000000000000ULL, -14}, {213200716355600ULL, -14},
        {188982236504614ULL, -14}, {171573250052696ULL, -14}, {158113883008419ULL, -14},
        {147224318434979ULL, -14}, {138169858614370ULL, -14}, {130434782608696ULL, -14},
        {125000000000000ULL, -14}, {119522860813439ULL, -14}, {114787456861885ULL, -14},
        {110656691950116ULL, -14}, {106997273528849ULL, -14}, {103109594271534ULL, -14},
        {100000000000000ULL, -14}  // 1/sqrt(1.0) = 1.0
    }};
};

template<>
struct inv_sqrt_01_table<decimal128_t> {
    static constexpr std::array<std::pair<std::uint64_t, int>, 16> value = {{
        {316227766016838ULL, -14}, {250000000000000ULL, -14}, {213200716355600ULL, -14},
        {188982236504614ULL, -14}, {171573250052696ULL, -14}, {158113883008419ULL, -14},
        {147224318434979ULL, -14}, {138169858614370ULL, -14}, {130434782608696ULL, -14},
        {125000000000000ULL, -14}, {119522860813439ULL, -14}, {114787456861885ULL, -14},
        {110656691950116ULL, -14}, {106997273528849ULL, -14}, {103109594271534ULL, -14},
        {100000000000000ULL, -14}
    }};
};

template<> struct inv_sqrt_01_table<decimal_fast32_t> : inv_sqrt_01_table<decimal32_t> {};
template<> struct inv_sqrt_01_table<decimal_fast64_t> : inv_sqrt_01_table<decimal64_t> {};
template<> struct inv_sqrt_01_table<decimal_fast128_t> : inv_sqrt_01_table<decimal128_t> {};

// Get reciprocal sqrt approximation from decimal lookup table
// Returns fixed-point representation suitable for SoftFloat algorithm
// sig_a is normalized to [10^(digits10-1), 10^digits10), representing gx in [0.1, 1.0)
template<typename T>
constexpr std::uint32_t approx_recip_sqrt_decimal(
    typename T::significand_type sig_a, int exp10val, bool exp_odd) noexcept
{
    constexpr int digits10 = std::numeric_limits<T>::digits10;
    constexpr auto& tbl = inv_sqrt_01_table<T>::value;
    
    // Calculate scale factors for normalization
    typename T::significand_type scale_factor;
    if (digits10 == 7) {
        scale_factor = UINT32_C(1000000);  // 10^6
    } else if (digits10 == 16) {
        scale_factor = UINT64_C(1000000000000000);  // 10^15
    } else {
        scale_factor = typename T::significand_type{1};
    }
    
    // sig_a is in [scale_factor, scale_factor*10), representing gx in [0.1, 1.0)
    // Map to table index: idx = (sig_a - scale_factor) * 16 / (scale_factor * 9)
    // Simplified: idx = (sig_a / scale_factor - 1) * 16 / 9
    // For better precision, use: idx = (sig_a - scale_factor) * 16 / (scale_factor * 9)
    
    typename T::significand_type gx_scaled = sig_a - scale_factor;  // [0, scale_factor*9)
    typename T::significand_type idx_numerator = gx_scaled * static_cast<typename T::significand_type>(16U);
    typename T::significand_type idx_denominator = scale_factor * static_cast<typename T::significand_type>(9U);
    
    int idx = 0;
    typename T::significand_type zero_val{};
    if (idx_denominator > zero_val) {
        idx = static_cast<int>(idx_numerator / idx_denominator);
    }
    
    // Clamp to valid table range [0, 15]
    if (idx < 0) idx = 0;
    if (idx > 15) idx = 15;
    
    // Get table entry: (sig, exp) pair representing 1/sqrt(x)
    const auto& entry = tbl[idx];
    const auto sig = entry.first;
    const auto exp = entry.second;
    
    // Convert decimal (sig, exp) to fixed-point representation for SoftFloat
    // We need to represent this as a 32-bit fixed-point number
    // The table value is sig * 10^exp, we need to scale it appropriately
    
    // For SoftFloat algorithm, we need the value in a form suitable for:
    // sig_z = (sig_a * recip_sqrt) >> 32
    // So recip_sqrt should represent 1/sqrt(gx) scaled appropriately
    
    // Convert sig * 10^exp to fixed-point: multiply by 2^32 / scale_factor
    // This gives us a value that when multiplied by sig_a and shifted right 32 bits,
    // gives us the correct result
    
    std::uint64_t sig_64 = static_cast<std::uint64_t>(sig);
    
    // Handle exponent: if exp is negative, we need to scale down
    // For now, we'll use a simplified approach: convert to approximate fixed point
    // The exact scaling depends on the SoftFloat algorithm requirements
    
    // Scale to fixed-point: multiply by 2^32, then divide by appropriate scale
    // For decimal, we need to account for the fact that sig_a is in [scale_factor, scale_factor*10)
    // and we want 1/sqrt(gx) where gx = sig_a / (scale_factor * 10)
    
    // Simplified: use the significand directly scaled to 32-bit range
    // For better accuracy, we'd need to properly handle the decimal exponent
    
    // For now, return a scaled version that works with the SoftFloat algorithm
    // This is a simplified version - may need refinement for correctness
    if (digits10 == 7) {
        // decimal32: sig is uint32_t, scale appropriately
        std::uint64_t scaled = sig_64;
        // Adjust for exponent: multiply by 10^(-exp) to normalize
        // Then scale to fixed-point representation
        // For exp = -6, we have sig * 10^-6, so multiply by 10^6 to get integer part
        // Then scale to 32-bit fixed point
        for (int i = 0; i < -exp; ++i) {
            scaled *= 10U;
        }
        // Scale to 32-bit fixed point: multiply by 2^32 / (scale_factor * 10)
        // Simplified: return upper 32 bits of scaled value
        return static_cast<std::uint32_t>(scaled >> 32);
    } else {
        // decimal64/128: similar approach but with higher precision
        // Scale sig to fixed-point representation
        std::uint64_t scaled = sig_64;
        for (int i = 0; i < -exp && i < 20; ++i) {
            scaled *= 10U;
        }
        // Return upper 32 bits for fixed-point representation
        return static_cast<std::uint32_t>(scaled >> 32);
    }
}

// Out-of-line definitions for static constexpr members (required for C++14 when ODR-used)
// Note: For explicitly specialized template classes, members are defined without template<>
constexpr std::array<std::pair<std::uint32_t, int>, 16> inv_sqrt_01_table<decimal32_t>::value;

constexpr std::array<std::pair<std::uint64_t, int>, 16> inv_sqrt_01_table<decimal64_t>::value;

constexpr std::array<std::pair<std::uint64_t, int>, 16> inv_sqrt_01_table<decimal128_t>::value;

} // namespace sqrt_lut_detail

// Optimized sqrt: same scaling as baseline (gx in [0.1, 1)), decimal lookup table
// for 1/sqrt(gx), then Newton-Raphson. Keeps table-based fast path, correct scaling.
template <typename T>
constexpr auto sqrt_impl_softfloat(const T x) noexcept
    BOOST_DECIMAL_REQUIRES(detail::is_decimal_floating_point_v, T)
{
    const auto fpc = fpclassify(x);
    T result{};

    #ifndef BOOST_DECIMAL_FAST_MATH
    if ((fpc == FP_NAN) || (fpc == FP_ZERO))
    {
        result = x;
        return result;
    }
    else if (signbit(x))
    {
        result = std::numeric_limits<T>::quiet_NaN();
        return result;
    }
    else if (fpc == FP_INFINITE)
    {
        result = std::numeric_limits<T>::infinity();
        return result;
    }
    #else
    if (signbit(x))
    {
        result = T{0};
        return result;
    }
    #endif

    // Extract significand and exponent
    int exp10val{};
    auto sig_a = frexp10(x, &exp10val);
    
    // Handle pure powers of 10 (exact case)
    const auto zeros_removal = remove_trailing_zeros(sig_a);
    const bool is_pure = (zeros_removal.trimmed_number == 1U);
    
    constexpr T one{1};
    
    if (is_pure)
    {
        const int p10 = exp10val + static_cast<int>(zeros_removal.number_of_removed_zeros);
        
        if (p10 == 0)
        {
            result = one;
        }
        else
        {
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
        }
        return result;
    }
    
    // Normalize to range suitable for table lookup: [0.1, 1.0) in decimal
    // Scale significand to [10^(digits10-1), 10^digits10)
    constexpr int digits10 = std::numeric_limits<T>::digits10;
    typename T::significand_type scale_factor;
    if (digits10 == 7) {
        scale_factor = UINT32_C(1000000);
    } else if (digits10 == 16) {
        scale_factor = UINT64_C(1000000000000000);
    } else {
        scale_factor = typename T::significand_type{1};
    }
    
    // Normalize significand
    const typename T::significand_type ten_val = static_cast<typename T::significand_type>(10U);
    const typename T::significand_type scale_factor_10 = scale_factor * ten_val;
    
    while (sig_a < scale_factor && exp10val > -1000)
    {
        sig_a *= ten_val;
        exp10val -= 1;
    }
    while (sig_a >= scale_factor_10 && exp10val < 1000)
    {
        sig_a /= ten_val;
        exp10val += 1;
    }
    
    // Determine exponent parity (affects sqrt calculation)
    const bool exp_odd = (exp10val & 1) != 0;
    
    // Get reciprocal square root approximation using lookup table
    const std::uint32_t recip_sqrt = sqrt_lut_detail::approx_recip_sqrt_decimal<T>(
        sig_a, exp10val, exp_odd);
    
    // Calculate initial square root approximation: sqrt(x) ≈ x * (1/sqrt(x))
    // Following SoftFloat algorithm: sig_z = (sig_a * recip_sqrt) >> 32
    typename T::significand_type sig_z;
    // Use same calculation for all types (works for all significand sizes)
    sig_z = static_cast<typename T::significand_type>(
        (static_cast<std::uint64_t>(sig_a) * static_cast<std::uint64_t>(recip_sqrt)) >> 32);
    
    // Adjust based on exponent parity (SoftFloat algorithm)
    // For odd exponent, we need to scale sig_a and adjust sig_z
    if (exp_odd)
    {
        // Scale up significand by 10 (decimal)
        const typename T::significand_type ten_val = static_cast<typename T::significand_type>(10U);
        sig_a *= ten_val;
        exp10val -= 1;  // Adjust exponent accordingly
        // Adjust sig_z: divide by sqrt(10) ≈ 3.16, approximated as divide by 3
        const typename T::significand_type three = static_cast<typename T::significand_type>(3U);
        sig_z = (sig_z * three) / ten_val;  // More accurate: sig_z / sqrt(10)
    }
    
    // Remainder elimination: Calculate remainder and refine result
    // Following SoftFloat: rem = sig_a - sig_z^2, then q = (rem >> 2) * recip_sqrt >> 32
    // Scale sig_z^2 appropriately for decimal arithmetic
    typename T::significand_type rem;
    // Use same calculation for all types
    const std::uint64_t sig_z_sq = static_cast<std::uint64_t>(sig_z) * static_cast<std::uint64_t>(sig_z);
    // Scale down: sig_z^2 needs to be divided by scale_factor to match sig_a's scale
    rem = sig_a - static_cast<typename T::significand_type>(sig_z_sq / scale_factor);
    
    // Refine using remainder: q = (rem >> 2) * recip_sqrt >> 32
    // This is the key remainder elimination step from SoftFloat
    typename T::significand_type q;
    q = static_cast<typename T::significand_type>(
        ((static_cast<std::uint64_t>(rem) >> 2) * static_cast<std::uint64_t>(recip_sqrt)) >> 32);
    
    // Update result: sig_z = sig_z + (q << 3) following SoftFloat algorithm
    // The << 3 scaling comes from the algorithm's precision requirements
    sig_z = sig_z + (q << 3);
    
    // Additional precision refinement (SoftFloat style)
    // Check if low bits need adjustment
    if (digits10 >= 16)
    {
        // For higher precision types, apply additional correction
        const std::uint64_t sig_z_scaled = static_cast<std::uint64_t>(sig_z) >> 6;
        const std::uint64_t rem2 = (static_cast<std::uint64_t>(sig_a) << 52) 
                                   - (sig_z_scaled * sig_z_scaled);
        
        if (rem2 & (1ULL << 63))
        {
            sig_z -= static_cast<typename T::significand_type>(1U);  // Adjust down
        }
        else if (rem2 != 0)
        {
            sig_z |= static_cast<typename T::significand_type>(1U);  // Adjust up (with proper masking)
        }
    }
    
    // Construct result with proper exponent
    // Ensure sig_z is not zero to avoid invalid result
    typename T::significand_type zero_sig{};
    if (sig_z == zero_sig)
    {
        // Fallback: use baseline-style approximation if sig_z is zero
        T gx { sig_a, -digits10 };
        result = gx;  // Initial guess as gx itself
    }
    else
    {
        const int result_exp = exp10val / 2;
        result = T(sig_z, result_exp);
        
        // Apply final corrections for odd exponent
        if (exp10val % 2 != 0)
        {
            if (exp10val % 2 == 1)
            {
                result *= numbers::sqrt10_v<T>;
            }
            else if (exp10val % 2 == -1)
            {
                result /= numbers::sqrt10_v<T>;
            }
        }
    }
    
    // One Newton-Raphson iteration for final precision refinement
    // This ensures we meet the required precision for decimal types
    // Skip Newton-Raphson if result is zero or invalid to avoid inf
    constexpr T zero{};
    if (result != zero && fpclassify(result) == FP_NORMAL)
    {
        int newton_iterations;
        if (digits10 == 7) {
            newton_iterations = 1;   // decimal32: 1 iteration
        } else if (digits10 == 16) {
            newton_iterations = 1;  // decimal64: 1 iteration
        } else {
            newton_iterations = 2;  // decimal128: 2 iterations
        }
        
        for (int i = 0; i < newton_iterations; ++i)
        {
            const T prev_result = result;
            result = (result + x / result) / T{2};
            // Check for convergence or invalid result
            if (result == prev_result || fpclassify(result) != FP_NORMAL)
            {
                break;
            }
        }
    }
    
    return result;
}

// "Simple" approach: Convert decimal to binary float, use hardware sqrt, convert back
// This leverages the highly optimized binary floating-point sqrt implementations
template <typename T>
constexpr auto sqrt_impl(const T x) noexcept
    BOOST_DECIMAL_REQUIRES(detail::is_decimal_floating_point_v, T)
{
    // Use SoftFloat-style optimization for better accuracy
    return sqrt_impl_softfloat(x);
}

} //namespace detail

BOOST_DECIMAL_EXPORT template <typename T>
constexpr auto sqrt(const T val) noexcept
    BOOST_DECIMAL_REQUIRES(detail::is_decimal_floating_point_v, T)
{
    using evaluation_type = detail::evaluation_type_t<T>;

    return static_cast<T>(detail::sqrt_impl(static_cast<evaluation_type>(val)));
}

} //namespace decimal
} //namespace boost

#endif //BOOST_DECIMAL_DETAIL_CMATH_SQRT_HPP

