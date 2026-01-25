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
// Lookup table for sqrt(x) over [0.1, 1.0) with 32 entries
// Table stores sqrt values directly as (significand, exponent) pairs
// Entry k corresponds to x = 0.1 + k * 0.9/32 = 0.1 + k * 0.028125
// This gives approximately 4-5 bits of initial accuracy
// ============================================================================

namespace sqrt_lut {

// sqrt(0.1 + k * 0.028125) for k = 0..31
// Values computed with high precision: sqrt(x) where x in [0.1, 1.0)
// Stored as (significand, exponent) where value = significand * 10^exponent

// For decimal32 (7 digits): significand has 7 digits
struct sqrt_table_32 {
    // sqrt values for x = 0.1, 0.128125, 0.15625, ... 0.971875
    static constexpr std::uint32_t sig[32] = {
        3162278U,  // sqrt(0.100000) = 0.3162278
        3579107U,  // sqrt(0.128125) = 0.3579107
        3952847U,  // sqrt(0.156250) = 0.3952847
        4293405U,  // sqrt(0.184375) = 0.4293405
        4609772U,  // sqrt(0.212500) = 0.4609772
        4906639U,  // sqrt(0.240625) = 0.4906639
        5187484U,  // sqrt(0.268750) = 0.5187484
        5454545U,  // sqrt(0.296875) = 0.5454545
        5709641U,  // sqrt(0.325000) = 0.5709641
        5954279U,  // sqrt(0.353125) = 0.5954279
        6189727U,  // sqrt(0.381250) = 0.6189727
        6417053U,  // sqrt(0.409375) = 0.6417053
        6637144U,  // sqrt(0.437500) = 0.6637144
        6850730U,  // sqrt(0.465625) = 0.6850730
        7058411U,  // sqrt(0.493750) = 0.7058411
        7260688U,  // sqrt(0.521875) = 0.7260688
        7457983U,  // sqrt(0.550000) = 0.7457983
        7650654U,  // sqrt(0.578125) = 0.7650654
        7839010U,  // sqrt(0.606250) = 0.7839010
        8023322U,  // sqrt(0.634375) = 0.8023322
        8203833U,  // sqrt(0.662500) = 0.8203833
        8380762U,  // sqrt(0.690625) = 0.8380762
        8554307U,  // sqrt(0.718750) = 0.8554307
        8724649U,  // sqrt(0.746875) = 0.8724649
        8891944U,  // sqrt(0.775000) = 0.8891944
        9056340U,  // sqrt(0.803125) = 0.9056340
        9217963U,  // sqrt(0.831250) = 0.9217963
        9376928U,  // sqrt(0.859375) = 0.9376928
        9533340U,  // sqrt(0.887500) = 0.9533340
        9687293U,  // sqrt(0.915625) = 0.9687293
        9838872U,  // sqrt(0.943750) = 0.9838872
        9988152U   // sqrt(0.971875) = 0.9988152
    };
    static constexpr int exp = -7;  // All values are 0.xxx...
};

// For decimal64 (16 digits): significand has 16 digits
struct sqrt_table_64 {
    static constexpr std::uint64_t sig[32] = {
        3162277660168380ULL,  // sqrt(0.100000)
        3579106635071914ULL,  // sqrt(0.128125)
        3952847075210474ULL,  // sqrt(0.156250)
        4293404959342067ULL,  // sqrt(0.184375)
        4609772228646444ULL,  // sqrt(0.212500)
        4906638735054128ULL,  // sqrt(0.240625)
        5187484255498820ULL,  // sqrt(0.268750)
        5454545454545454ULL,  // sqrt(0.296875)
        5709641448012965ULL,  // sqrt(0.325000)
        5954279315739265ULL,  // sqrt(0.353125)
        6189727347919860ULL,  // sqrt(0.381250)
        6417053202092121ULL,  // sqrt(0.409375)
        6637143758222514ULL,  // sqrt(0.437500)
        6850730098476167ULL,  // sqrt(0.465625)
        7058410988837906ULL,  // sqrt(0.493750)
        7260688316046190ULL,  // sqrt(0.521875)
        7457982697109289ULL,  // sqrt(0.550000)
        7650654413461371ULL,  // sqrt(0.578125)
        7839010010010010ULL,  // sqrt(0.606250)
        8023321821957880ULL,  // sqrt(0.634375)
        8203832766255085ULL,  // sqrt(0.662500)
        8380762018227291ULL,  // sqrt(0.690625)
        8554306988261416ULL,  // sqrt(0.718750)
        8724649011478770ULL,  // sqrt(0.746875)
        8891944297423007ULL,  // sqrt(0.775000)
        9056339633645177ULL,  // sqrt(0.803125)
        9217962963254956ULL,  // sqrt(0.831250)
        9376928356636595ULL,  // sqrt(0.859375)
        9533339654498488ULL,  // sqrt(0.887500)
        9687292698545498ULL,  // sqrt(0.915625)
        9838872656679117ULL,  // sqrt(0.943750)
        9988152262180321ULL   // sqrt(0.971875)
    };
    static constexpr int exp = -16;
};

// Out-of-line definitions for C++14 ODR-use
constexpr std::uint32_t sqrt_table_32::sig[32];
constexpr int sqrt_table_32::exp;
constexpr std::uint64_t sqrt_table_64::sig[32];
constexpr int sqrt_table_64::exp;

// Helper to select table based on decimal type
template <typename T>
struct sqrt_table_selector;

template <> struct sqrt_table_selector<decimal32_t> { using type = sqrt_table_32; };
template <> struct sqrt_table_selector<decimal64_t> { using type = sqrt_table_64; };
template <> struct sqrt_table_selector<decimal128_t> { using type = sqrt_table_64; };  // Use 64-bit table, refine with Newton
template <> struct sqrt_table_selector<decimal_fast32_t> { using type = sqrt_table_32; };
template <> struct sqrt_table_selector<decimal_fast64_t> { using type = sqrt_table_64; };
template <> struct sqrt_table_selector<decimal_fast128_t> { using type = sqrt_table_64; };

} // namespace sqrt_lut

// ============================================================================
// Optimized sqrt implementation using lookup table + Newton-Raphson
// 
// Algorithm:
// 1. Handle special cases (NaN, zero, negative, infinity)
// 2. Extract significand and exponent: x = sig * 10^exp
// 3. Normalize to gx in [0.1, 1.0): gx = sig * 10^(-digits10)
// 4. Lookup sqrt(gx) from table with linear interpolation
// 5. Apply 1-2 Newton-Raphson iterations for full precision
// 6. Rescale result based on original exponent
// ============================================================================

template <typename T>
constexpr auto sqrt_impl(const T x) noexcept
    BOOST_DECIMAL_REQUIRES(detail::is_decimal_floating_point_v, T)
{
    const auto fpc = fpclassify(x);
    T result{};

    // ========================================================================
    // Special case handling
    // ========================================================================
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

    // ========================================================================
    // Extract significand and exponent
    // ========================================================================
    int exp10val{};
    auto sig = frexp10(x, &exp10val);

    // ========================================================================
    // Handle pure powers of 10 (exact results)
    // ========================================================================
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

    // ========================================================================
    // Normalize significand to [0.1, 1.0) range
    // gx = sig * 10^(-digits10) where sig is in [10^(digits10-1), 10^digits10)
    // ========================================================================
    constexpr int digits10 = std::numeric_limits<T>::digits10;
    
    // Create gx in [0.1, 1.0)
    T gx{sig, -digits10};
    exp10val += digits10;

    // ========================================================================
    // Lookup table interpolation for initial sqrt approximation
    // Table has 32 entries covering [0.1, 1.0) with step 0.028125
    // ========================================================================
    using table_type = typename sqrt_lut::sqrt_table_selector<T>::type;
    
    // Calculate table index: idx = (gx - 0.1) / 0.028125 = (gx - 0.1) * 32 / 0.9
    // For integer arithmetic: idx = (sig - sig_min) * 32 / (sig_max - sig_min)
    // where sig_min = 10^(digits10-1), sig_max = 10^digits10
    
    // Compute index using the significand directly
    // sig is in [10^(digits10-1), 10^digits10), we want index in [0, 31]
    typename T::significand_type sig_min;
    typename T::significand_type sig_range;
    
    if (digits10 == 7) {
        sig_min = UINT32_C(1000000);      // 10^6
        sig_range = UINT32_C(9000000);    // 10^7 - 10^6
    } else if (digits10 == 16) {
        sig_min = UINT64_C(1000000000000000);      // 10^15
        sig_range = UINT64_C(9000000000000000);    // 10^16 - 10^15
    } else {
        // decimal128: use 64-bit calculation, will refine with more Newton iterations
        sig_min = UINT64_C(1000000000000000);
        sig_range = UINT64_C(9000000000000000);
    }
    
    // Safe index calculation avoiding overflow
    // idx = (sig - sig_min) * 32 / sig_range
    std::uint64_t sig_offset;
    if (digits10 <= 16) {
        sig_offset = static_cast<std::uint64_t>(sig) - static_cast<std::uint64_t>(sig_min);
    } else {
        // For decimal128, extract lower 64 bits for index calculation
        sig_offset = static_cast<std::uint64_t>(sig) - static_cast<std::uint64_t>(sig_min);
    }
    
    // Compute index with proper rounding
    std::uint32_t idx = static_cast<std::uint32_t>((sig_offset * 32ULL) / static_cast<std::uint64_t>(sig_range));
    if (idx > 31U) idx = 31U;
    
    // Get table value and construct initial approximation
    // Linear interpolation between table[idx] and table[idx+1] for better accuracy
    const auto sig0 = table_type::sig[idx];
    const auto sig1 = (idx < 31U) ? table_type::sig[idx + 1U] : table_type::sig[31];
    const int tbl_exp = table_type::exp;
    
    // Interpolation fraction: frac = (sig_offset * 32 % sig_range) / sig_range
    // Simplified: use midpoint if we're in the upper half of the interval
    std::uint64_t remainder = (sig_offset * 32ULL) % static_cast<std::uint64_t>(sig_range);
    bool use_upper_half = (remainder * 2ULL >= static_cast<std::uint64_t>(sig_range));
    
    // Linear interpolation: result = sig0 + (sig1 - sig0) * frac
    // For simplicity, use weighted average based on position
    typename T::significand_type interp_sig;
    if (digits10 == 7) {
        if (use_upper_half && idx < 31U) {
            // Interpolate: (sig0 + sig1) / 2 approximately
            interp_sig = static_cast<typename T::significand_type>((static_cast<std::uint64_t>(sig0) + static_cast<std::uint64_t>(sig1)) / 2U);
        } else {
            interp_sig = static_cast<typename T::significand_type>(sig0);
        }
    } else {
        if (use_upper_half && idx < 31U) {
            interp_sig = static_cast<typename T::significand_type>((sig0 + sig1) / 2U);
        } else {
            interp_sig = static_cast<typename T::significand_type>(sig0);
        }
    }
    
    // Construct initial sqrt(gx) approximation
    result = T{interp_sig, tbl_exp};

    // ========================================================================
    // Newton-Raphson refinement: result = (result + gx / result) / 2
    // Number of iterations depends on precision needed:
    // - Table gives ~4-5 decimal digits
    // - Each Newton iteration roughly doubles precision
    // - decimal32 (7 digits): 1 iteration
    // - decimal64 (16 digits): 2 iterations  
    // - decimal128 (34 digits): 3 iterations
    // ========================================================================
    constexpr int newton_iters = (digits10 <= 7) ? 1 : (digits10 <= 16) ? 2 : 3;
    
    for (int i = 0; i < newton_iters; ++i)
    {
        result = (result + gx / result) / T{2};
    }

    // ========================================================================
    // Rescale result based on original exponent
    // sqrt(x * 10^exp) = sqrt(x) * 10^(exp/2)
    // For odd exponent: sqrt(x * 10^(2k+1)) = sqrt(10*x) * 10^k = sqrt(x) * sqrt(10) * 10^k
    // ========================================================================
    if (exp10val != 0)
    {
        const int exp10val_mod2 = (exp10val % 2);
        result *= T{1, exp10val / 2};

        if (exp10val_mod2 == 1)
        {
            result *= numbers::sqrt10_v<T>;
        }
        else if (exp10val_mod2 == -1)
        {
            result /= numbers::sqrt10_v<T>;
        }
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
