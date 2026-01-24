#pragma once

#include <array>
#include <bit>
#include <cstdint>
#include <cstdlib>
#include <iostream>

/* The following macros are derived from GCC's longlong.h and are used for high-precision integer
 * operations */

#define W_TYPE_SIZE 64
typedef uint64_t UWtype;   // Single-word unsigned type
typedef uint32_t UHWtype;  // Half-word unsigned type

#define __BITS4 (W_TYPE_SIZE / 4)
#define __ll_B ((UWtype)1 << (W_TYPE_SIZE / 2))
#define __ll_lowpart(n) ((UWtype)(n) & (__ll_B - 1))
#define __ll_highpart(n) ((UWtype)((n) >> (W_TYPE_SIZE / 2)))

/* __udiv_qrnnd_c: Computes quotient and remainder of double-word by single-word division
   (q, r) = (n1,n0) / d */
/* Define this unconditionally, so it can be used for debugging.  */
#if !defined(__udiv_qrnnd_c)
#define __udiv_qrnnd_c(q, r, n1, n0, d)                                         \
    do {                                                                        \
        UWtype __d1, __d0, __q1, __q0;                                          \
        UWtype __r1, __r0, __m;                                                 \
        __d1 = __ll_highpart(d);                                                \
        __d0 = __ll_lowpart(d);                                                 \
                                                                                \
        __r1 = (n1) % __d1;                                                     \
        __q1 = (n1) / __d1;                                                     \
        __m = (UWtype)__q1 * __d0;                                              \
        __r1 = __r1 * __ll_B | __ll_highpart(n0);                               \
        if (__r1 < __m) {                                                       \
            __q1--, __r1 += (d);                                                \
            if (__r1 >= (d)) /* i.e. we didn't get carry when adding to __r1 */ \
                if (__r1 < __m)                                                 \
                    __q1--, __r1 += (d);                                        \
        }                                                                       \
        __r1 -= __m;                                                            \
                                                                                \
        __r0 = __r1 % __d1;                                                     \
        __q0 = __r1 / __d1;                                                     \
        __m = (UWtype)__q0 * __d0;                                              \
        __r0 = __r0 * __ll_B | __ll_lowpart(n0);                                \
        if (__r0 < __m) {                                                       \
            __q0--, __r0 += (d);                                                \
            if (__r0 >= (d))                                                    \
                if (__r0 < __m)                                                 \
                    __q0--, __r0 += (d);                                        \
        }                                                                       \
        __r0 -= __m;                                                            \
                                                                                \
        (q) = (UWtype)__q1 * __ll_B | __q0;                                     \
        (r) = __r0;                                                             \
    } while (0)
#endif

/* udiv_qrnnd: Computes quotient and remainder of double-word by single-word division
   (q, r) = (n1,n0) / d */
#if !defined(udiv_qrnnd)
#define udiv_qrnnd __udiv_qrnnd_c
#endif

/* umul_ppmm: Calculates the double-word product of two single-word operands
   (w1, w0) = u * v */
/* If we still don't have umul_ppmm, define it using plain C.  */
#if !defined(umul_ppmm)
#define umul_ppmm(w1, w0, u, v)                                            \
    do {                                                                   \
        UWtype __x0, __x1, __x2, __x3;                                     \
        UHWtype __ul, __vl, __uh, __vh;                                    \
                                                                           \
        __ul = __ll_lowpart(u);                                            \
        __uh = __ll_highpart(u);                                           \
        __vl = __ll_lowpart(v);                                            \
        __vh = __ll_highpart(v);                                           \
                                                                           \
        __x0 = (UWtype)__ul * __vl;                                        \
        __x1 = (UWtype)__ul * __vh;                                        \
        __x2 = (UWtype)__uh * __vl;                                        \
        __x3 = (UWtype)__uh * __vh;                                        \
                                                                           \
        __x1 += __ll_highpart(__x0); /* this can't give carry */           \
        __x1 += __x2;                /* but this indeed can */             \
        if (__x1 < __x2)             /* did we get it? */                  \
            __x3 += __ll_B;          /* yes, add it in the proper pos.  */ \
                                                                           \
        (w1) = __x3 + __ll_highpart(__x1);                                 \
        (w0) = __ll_lowpart(__x1) * __ll_B + __ll_lowpart(__x0);           \
    } while (0)
#endif

namespace math::fp {
/**
 * @brief Low-level fixed-point math utilities
 * Independent of any other math classes, operates solely on raw int64_t values
 */
class Primitives {
 public:
    /**
     * @brief 64-bit fixed-point square root implementation, based on ARM CMSIS-DSP(arm_sqrt_q31.c)
     *
     * @param in Input value, raw fixed-point value
     * @param fractionBits Number of fractional bits in the fixed-point format
     * @return Square root result as a raw fixed-point value
     */
    [[nodiscard]] static auto Fixed64Sqrt(int64_t in, int fractionBits) noexcept -> int64_t {
        // Define Q60QUARTER (corresponds to ARM's Q28QUARTER)
        constexpr int64_t Q60QUARTER = 0x2000000000000000LL;  // 0.25 in Q0.63 format

        // Initial approximation lookup table for 1/sqrt(x), used in 64-bit fixed-point square root
        // Format: Q3.60, generated based on ARM CMSIS-DSP table generation logic
        static constexpr std::array<int64_t, 32> sqrt_initial_lut_q63 = {
            0x2000000000000000LL,  // 1/sqrt(0.250000) = 0.250000
            0x1E2B7DDDFEFA6700LL,  // 1/sqrt(0.281250) = 0.235702
            0x1C9F25C5BFEDD900LL,  // 1/sqrt(0.312500) = 0.223607
            0x1B4A293C1D954F00LL,  // 1/sqrt(0.343750) = 0.213201
            0x1A20BD700C2C3F00LL,  // 1/sqrt(0.375000) = 0.204124
            0x191A556151761C00LL,  // 1/sqrt(0.406250) = 0.196116
            0x183091E6A7F7E600LL,  // 1/sqrt(0.437500) = 0.188982
            0x175E9746A0B09800LL,  // 1/sqrt(0.468750) = 0.182574
            0x16A09E667F3BCC00LL,  // 1/sqrt(0.500000) = 0.176777
            0x15F3AA673FA91000LL,  // 1/sqrt(0.531250) = 0.171499
            0x1555555555555500LL,  // 1/sqrt(0.562500) = 0.166667
            0x14C3ABE93BCF7400LL,  // 1/sqrt(0.593750) = 0.162221
            0x143D136248490F00LL,  // 1/sqrt(0.625000) = 0.158114
            0x13C03650E00E0300LL,  // 1/sqrt(0.656250) = 0.154303
            0x134BF63D15682600LL,  // 1/sqrt(0.687500) = 0.150756
            0x12DF60C5DF2C9E00LL,  // 1/sqrt(0.718750) = 0.147442
            0x1279A74590331D00LL,  // 1/sqrt(0.750000) = 0.144338
            0x121A1851FF630A00LL,  // 1/sqrt(0.781250) = 0.141421
            0x11C01AA03BE89600LL,  // 1/sqrt(0.812500) = 0.138675
            0x116B28F55D72D400LL,  // 1/sqrt(0.843750) = 0.136083
            0x111ACEE560242A00LL,  // 1/sqrt(0.875000) = 0.133631
            0x10CEA6317186DC00LL,  // 1/sqrt(0.906250) = 0.131306
            0x108654A2D4F6DA00LL,  // 1/sqrt(0.937500) = 0.129099
            0x10418A4806DE7D00LL,  // 1/sqrt(0.968750) = 0.127000
            0x1000000000000000LL,  // 1/sqrt(1.000000) = 0.125000
            0x0FC176441607CD00LL,  // 1/sqrt(1.031250) = 0.123091
            0x0F85B42469578E00LL,  // 1/sqrt(1.062500) = 0.121268
            0x0F4C866D6AAF6900LL,  // 1/sqrt(1.093750) = 0.119523
            0x0F15BEEEFF7D3380LL,  // 1/sqrt(1.125000) = 0.117851
            0x0EE133DF522AA480LL,  // 1/sqrt(1.156250) = 0.116248
            0x0EAEBF548A5C9B00LL,  // 1/sqrt(1.187500) = 0.114708
            0x0E7E3ED195490900LL   // 1/sqrt(1.218750) = 0.113228
        };

        int64_t number, var1, signBits1, temp;

        number = in;

        /* If the input is a positive number then compute the signBits. */
        if (number > 0) {
            // For Q(63-n).n format, square root operation converts it to Q((63-n)/2).n
            // Right shift by 1 converts Q(63-n).n to Q(62-n).n, Square root yields
            // Q((62-n)/2).(n/2), ensuring precision and preventing overflow
            if (fractionBits % 2 == 0) {
                number = number >> 1;
            }

            signBits1 = CountlZero(static_cast<uint64_t>(number)) - 1;

            /* Shift by the number of signBits1 */
            if ((signBits1 % 2) == 0) {
                number = number << signBits1;
            } else {
                number = number << (signBits1 - 1);
            }

            /* Start value for 1/sqrt(x) for the Newton iteration */
            // Use ARM-style index calculation - note this is (q+1-6)
            var1 = sqrt_initial_lut_q63[(number >> 58) - (Q60QUARTER >> 58)];

            /* 0.5 var1 * (3 - number * var1 * var1) */

            /* 1st iteration */
            temp = MulU64Shifted(var1, var1, 60);
            temp = MulU64Shifted(number, temp, 63);
            temp = 0x3000000000000000LL - temp;    // 3.0 in Q3.60
            var1 = MulU64Shifted(var1, temp, 61);  // Divide by 2 (0.5 * ...)

            /* 2nd iteration */
            temp = MulU64Shifted(var1, var1, 60);
            temp = MulU64Shifted(number, temp, 63);
            temp = 0x3000000000000000LL - temp;
            var1 = MulU64Shifted(var1, temp, 61);

            /* 3rd iteration */
            temp = MulU64Shifted(var1, var1, 60);
            temp = MulU64Shifted(number, temp, 63);
            temp = 0x3000000000000000LL - temp;
            var1 = MulU64Shifted(var1, temp, 61);

            /* Multiply the inverse square root with the original value */
            var1 = MulU64Shifted(number, var1, 60);

            // Q((63-n)/2).(n/2) -> Q(63-n).n
            int shift = (63 - fractionBits) / 2;

            /* Shift the output down accordingly */
            if ((signBits1 % 2) == 0) {
                shift += (signBits1 / 2);
            } else {
                shift += ((signBits1 - 1) / 2);
            }

            /* Shift */
            var1 = var1 >> shift;

            return var1;
        }
        /* If the number is a negative number then store zero as its square root value */
        else {
            return 0;
        }
    }

    // Lookup table for reciprocal square root approximation
    static constexpr std::array<uint16_t, 16> softfloat_approxRecipSqrt_1k0s = {0xB4C9,
                                                                                0xFFAB,
                                                                                0xAA7D,
                                                                                0xF11C,
                                                                                0xA1C5,
                                                                                0xE4C7,
                                                                                0x9A43,
                                                                                0xDA29,
                                                                                0x93B5,
                                                                                0xD0E5,
                                                                                0x8DED,
                                                                                0xC8B7,
                                                                                0x88C6,
                                                                                0xC16D,
                                                                                0x8424,
                                                                                0xBAE1};

    static constexpr std::array<uint16_t, 16> softfloat_approxRecipSqrt_1k1s = {0xA5A5,
                                                                                0xEA42,
                                                                                0x8C21,
                                                                                0xC62D,
                                                                                0x788F,
                                                                                0xAA7F,
                                                                                0x6928,
                                                                                0x94B6,
                                                                                0x5CC7,
                                                                                0x8335,
                                                                                0x52A6,
                                                                                0x74E2,
                                                                                0x4A3E,
                                                                                0x68FE,
                                                                                0x432B,
                                                                                0x5EFD};

    /**
     * @brief Precise reciprocal square root approximation, directly referenced from SoftFloat
     * library
     */
    [[nodiscard]] static constexpr auto softfloat_approxRecipSqrt32_1(uint32_t oddExpA,
                                                                      uint32_t a) noexcept
        -> uint32_t {
        // Calculate lookup table index
        int index = ((a >> 27) & 0xE) + oddExpA;

        // Calculate correction term
        uint16_t eps = static_cast<uint16_t>(a >> 12);

        // Get initial approximation and apply correction
        uint16_t r0 =
            softfloat_approxRecipSqrt_1k0s[index]
            - static_cast<uint16_t>((static_cast<uint32_t>(softfloat_approxRecipSqrt_1k1s[index])
                                     * static_cast<uint32_t>(eps))
                                    >> 20);

        // Calculate square correction term
        uint32_t ESqrR0 = static_cast<uint32_t>(r0) * r0;

        // Adjust based on parity
        if (oddExpA == 0) {
            ESqrR0 <<= 1;
        }

        // Calculate sigma0 correction term
        uint32_t sigma0 = ~static_cast<uint32_t>((static_cast<uint64_t>(ESqrR0) * a) >> 23);

        // Calculate more precise approximation r
        uint32_t r = (static_cast<uint32_t>(r0) << 16)
                     + (static_cast<uint32_t>((static_cast<uint64_t>(r0) * sigma0) >> 25));

        // Apply additional precision correction
        r += static_cast<uint32_t>(
            (static_cast<uint64_t>((((r >> 1) + (r >> 3)) - (static_cast<uint32_t>(r0) << 14)))
             * static_cast<uint64_t>((static_cast<uint64_t>(sigma0) * sigma0) >> 32))
            >> 48);

        // Ensure the highest bit is 1
        if ((r & 0x80000000) == 0) {
            r = 0x80000000;
        }

        return r;
    }

    /**
     * @brief Optimized Q-format square root calculation, strictly following SoftFloat library
     * implementation
     *
     * This implementation is based on the SoftFloat library's square root algorithm,
     * adapted for fixed-point arithmetic. It uses table-based initial approximation
     * followed by Newton-Raphson iterations for refinement.
     *
     * References:
     * - SoftFloat library by John R. Hauser
     * - Jean-Michel Muller et al. "Elementary Functions: Algorithms and Implementation"
     * - Cody and Waite "Software Manual for the Elementary Functions"
     *
     * @param a Input value in Q(63-fraction_bits).fraction_bits format
     * @param fraction_bits Number of fractional bits [0,63]
     * @return Square root result, maintaining the original Q format
     */
    [[nodiscard]] static auto Fixed64SqrtFast(int64_t a, int fraction_bits) noexcept -> int64_t {
        // Handle zero and negative inputs
        if (a <= 0) [[unlikely]]
            return 0;  // Return 0 for negative inputs

        // 1. Find the position of the most significant bit
        const uint64_t u_a = static_cast<uint64_t>(a);
        const int msb = 63 - CountlZero(u_a);

        // Calculate IEEE-754 double precision biased exponent
        const int exp_a =
            msb - fraction_bits + 1023;   // 1023 is the exponent bias for double precision
        const int exp_a_odd = exp_a & 1;  // Exponent parity affects square root calculation

        // 2. Construct normalized significand in IEEE-754 format
        const int align_shift =
            msb - 52;  // 52 is the number of significand bits in double precision

        uint64_t sig_a;
        if (align_shift >= 0) {
            // Right shift to align, add implicit leading 1
            sig_a = 0x0010000000000000ULL | ((u_a >> align_shift) & 0x000FFFFFFFFFFFFF);
        } else {
            // Left shift to align, add implicit leading 1
            sig_a = 0x0010000000000000ULL | ((u_a << (-align_shift)) & 0x000FFFFFFFFFFFFF);
        }

        // 3. Extract top 32 bits from sig_a for initial approximation
        const uint32_t sig32_a = static_cast<uint32_t>(sig_a >> 21);

        // Normalization to position 31 (used only for denormalization)
        const int norm_shift = msb - 31;

        // 4. Get reciprocal square root approximation
        const uint32_t recip_sqrt32 = softfloat_approxRecipSqrt32_1(exp_a_odd, sig32_a);

        // 5. Calculate initial square root approximation
        uint32_t sig32_z =
            static_cast<uint32_t>((static_cast<uint64_t>(sig32_a) * recip_sqrt32) >> 32);

        // 6. Adjust sig_a and sig32_z based on exponent parity
        if (exp_a_odd) {
            sig_a <<= 8;
            sig32_z >>= 1;
        } else {
            sig_a <<= 9;
        }

        // 7. Calculate remainder and precision adjustment
        uint64_t rem = (sig_a - (static_cast<uint64_t>(sig32_z) * sig32_z));

        const uint32_t q =
            static_cast<uint32_t>((static_cast<uint64_t>(rem >> 2) * recip_sqrt32) >> 32);

        uint64_t sig_z = ((static_cast<uint64_t>(sig32_z) << 32) | (1ULL << 5))
                         + (static_cast<uint64_t>(q) << 3);

        // 8. Additional precision correction
        if ((sig_z & 0x1FF) < 0x22) {
            sig_z &= ~0x3FULL;
            const uint64_t shifted_sig_z = sig_z >> 6;
            const uint64_t rem2 = (sig_a << 52) - shifted_sig_z * shifted_sig_z;

            if (rem2 & (1ULL << 63)) {
                sig_z -= 1;
            } else if (rem2) {
                sig_z |= 1;
            }
        }

        // 9. Denormalization calculation
        const int denorm_shift = norm_shift >> 1;

        // 10. Adjust final shift based on parity
        const int final_shift = denorm_shift - 31 + exp_a_odd + ((fraction_bits - 31) >> 1);

        uint64_t result;
        if (final_shift >= 0) {
            // Left shift doesn't require rounding as no precision is lost
            result = (sig_z << final_shift);
        } else {
            // Apply SoftFloat style rounding for right shift
            int abs_shift = -final_shift;

            // Extract round bits (all bits that will be shifted out)
            uint64_t round_mask = (1ULL << abs_shift) - 1ULL;
            uint64_t round_bits = sig_z & round_mask;

            // Half-way point for rounding
            uint64_t half_point = 1ULL << (abs_shift - 1);

            // Perform right shift with rounding
            result = (sig_z >> abs_shift)
                     + ((round_bits > half_point
                         || (round_bits == half_point && ((sig_z >> abs_shift) & 1ULL)))
                            ? 1ULL
                            : 0ULL);
        }

        return static_cast<int64_t>(result);
    }

    /**
     * @brief Calculate the binary width of an integer (position of the highest bit + 1)
     * @param x Input value
     * @return Binary width, returns 0 if x is 0
     */
    [[nodiscard]] static constexpr auto BitWidth(uint64_t x) noexcept -> int {
#if defined(__GNUC__) || defined(__clang__)
        return x == 0 ? 0 : 64 - __builtin_clzll(x);
#elif defined(__cpp_lib_bitops) && __cpp_lib_bitops >= 201806L
        // Use C++20 standard library implementation (available in VS2019 16.8+, _MSC_VER >= 1928)
        return std::bit_width(x);
#else
        // Efficient constexpr implementation for MSVC
        if (x == 0)
            return 0;

        // Find position of highest bit using binary search approach
        int n = 0;
        if (x & 0xFFFFFFFF00000000ULL) {
            n += 32;
            x >>= 32;
        }
        if (x & 0x00000000FFFF0000ULL) {
            n += 16;
            x >>= 16;
        }
        if (x & 0x000000000000FF00ULL) {
            n += 8;
            x >>= 8;
        }
        if (x & 0x00000000000000F0ULL) {
            n += 4;
            x >>= 4;
        }
        if (x & 0x000000000000000CULL) {
            n += 2;
            x >>= 2;
        }
        if (x & 0x0000000000000002ULL) {
            n += 1;
        }

        return n + 1;  // Add 1 because we want width (position + 1)
#endif
    }

    /**
     * @brief Count leading zeros
     * @param x Input value
     * @return Number of leading zeros, returns 64 if x is 0
     */
    [[nodiscard]] static constexpr auto CountlZero(uint64_t x) noexcept -> int {
#if defined(__GNUC__) || defined(__clang__)
        return __builtin_clzll(x);
#elif defined(__cpp_lib_bitops) && __cpp_lib_bitops >= 201806L
        // Use C++20 standard library implementation (available in VS2019 16.8+, _MSC_VER >= 1928)
        return std::countl_zero(x);
#else
        // Efficient constexpr implementation for MSVC
        if (x == 0)
            return 64;

        // Accelerated implementation using binary search approach
        int result = 0;
        // Check 32 bits at a time
        if ((x & 0xFFFFFFFF00000000ULL) == 0) {
            result += 32;
            x <<= 32;
        }
        // Check 8 bits at a time
        if ((x & 0xFF00000000000000ULL) == 0) {
            result += 8;
            x <<= 8;
        }
        // Check 4 bits at a time
        if ((x & 0xF000000000000000ULL) == 0) {
            result += 4;
            x <<= 4;
        }
        // Check bit by bit for the final count
        if ((x & 0x8000000000000000ULL) == 0) {
            result += 1;
            x <<= 1;
        }
        if ((x & 0x8000000000000000ULL) == 0) {
            result += 1;
            x <<= 1;
        }
        if ((x & 0x8000000000000000ULL) == 0) {
            result += 1;
            x <<= 1;
        }
        if ((x & 0x8000000000000000ULL) == 0) {
            result += 1;
        }

        return result;
#endif
    }

    /**
     * @brief Count trailing zeros
     * @param x Input value
     * @return Number of trailing zeros, returns 64 if x is 0
     */
    [[nodiscard]] static constexpr auto CountrZero(uint64_t x) noexcept -> int {
#if defined(__GNUC__) || defined(__clang__)
        return __builtin_ctzll(x);
#elif defined(__cpp_lib_bitops) && __cpp_lib_bitops >= 201806L
        // Use C++20 standard library implementation (available in VS2019 16.8+, _MSC_VER >= 1928)
        return std::countr_zero(x);
#else
        // Efficient constexpr implementation for MSVC
        if (x == 0)
            return 64;

        // Use binary search approach to find first set bit
        int n = 0;
        if ((x & 0x00000000FFFFFFFFULL) == 0) {
            n += 32;
            x >>= 32;
        }
        if ((x & 0x000000000000FFFFULL) == 0) {
            n += 16;
            x >>= 16;
        }
        if ((x & 0x00000000000000FFULL) == 0) {
            n += 8;
            x >>= 8;
        }
        if ((x & 0x000000000000000FULL) == 0) {
            n += 4;
            x >>= 4;
        }
        if ((x & 0x0000000000000003ULL) == 0) {
            n += 2;
            x >>= 2;
        }
        if ((x & 0x0000000000000001ULL) == 0) {
            n += 1;
        }

        return n;
#endif
    }

    /**
     * @brief Count the number of 1 bits in binary representation
     * @param x Input value
     * @return Number of 1 bits
     */
    [[nodiscard]] static constexpr auto Popcount(uint64_t x) noexcept -> int {
#if defined(__GNUC__) || defined(__clang__)
        return __builtin_popcountll(x);
#elif defined(__cpp_lib_bitops) && __cpp_lib_bitops >= 201806L
        // Use C++20 standard library implementation (available in VS2019 16.8+, _MSC_VER >= 1928)
        return std::popcount(x);
#else
        // Efficient constexpr implementation for MSVC using SWAR algorithm
        x = x - ((x >> 1) & 0x5555555555555555ULL);
        x = (x & 0x3333333333333333ULL) + ((x >> 2) & 0x3333333333333333ULL);
        x = (x + (x >> 4)) & 0x0F0F0F0F0F0F0F0FULL;
        x = x + (x >> 8);
        x = x + (x >> 16);
        x = x + (x >> 32);
        return static_cast<int>(x & 0x7F);
#endif
    }

    /**
     * @brief Shift a 128-bit value right with proper rounding for small distances (<64)
     *
     * @param hi High 64 bits (signed)
     * @param lo Low 64 bits (unsigned)
     * @param dist Shift distance (must be less than 64)
     * @return int64_t Result with proper rounding
     *
     * @note This is optimized for the common case where dist = 63-P
     */
    static constexpr auto ShortShiftRightRound64(int64_t hi, uint64_t lo, uint8_t dist) noexcept
        -> int64_t {
        if (dist >= 64) {
            return hi < 0 ? -1 : 0;  // For large shifts, return sign extension
        }

        // Use GCC-style sign handling
        int c1 = 0;  // Result sign flag

        // Handle sign of the high part (determines sign of entire 128-bit value)
        uint64_t hi_abs = static_cast<uint64_t>(hi);
        uint64_t lo_abs = lo;

        if (hi < 0) {
            c1 = ~c1;  // Flip result sign

            // Get absolute value of 128-bit number
            hi_abs = ~hi_abs;
            lo_abs = ~lo_abs;

            // Add 1 (two's complement)
            lo_abs += 1;
            if (lo_abs == 0) {
                hi_abs += 1;  // Handle carry
            }
        }

        // Main value comes from high part shifted right
        uint64_t result = hi_abs >> dist;

        // Get bits that shift from high word to low word
        uint64_t hi_to_lo_bits = hi_abs << (64 - dist);

        // Bits from low part that contribute after shift
        uint64_t lo_contribution = lo_abs >> dist;

        // Combine high bits shifted down and low bits shifted in
        result |= hi_to_lo_bits | lo_contribution;

        // Handle rounding using round-to-nearest, ties to even
        if (dist > 0) {
            // Create mask for all bits that will be shifted out
            uint64_t round_mask = (1ULL << dist) - 1ULL;
            uint64_t round_bits = lo_abs & round_mask;

            // Half-way point for rounding
            uint64_t half_point = 1ULL << (dist - 1);

            // Round up if round_bits > half_point or if exactly at half_point and result is odd
            if (round_bits > half_point || (round_bits == half_point && (result & 1ULL))) {
                result += 1;
            }
        }

        // Apply sign
        if (c1)
            return -static_cast<int64_t>(result);
        else
            return static_cast<int64_t>(result);
    }

    /**
     * @brief Multiply two 64-bit unsigned integers, returning a 64-bit unsigned result
     *
     * Implementation based on the umul_ppmm macro from GCC's libgcc longlong.h,
     * splitting 64-bit operands into 32-bit parts for multiplication to avoid
     * the performance overhead of direct 128-bit multiplication and using
     * conditional carry detection for optimization.
     *
     * @param a First 64-bit unsigned integer
     * @param b Second 64-bit unsigned integer
     * @param shift Right shift amount, range 1-63
     * @return Equivalent to (a*b)>>shift, but avoids intermediate value overflow
     */
    static constexpr auto MulU64Shifted(uint64_t a, uint64_t b, int shift) noexcept -> uint64_t {
        uint64_t high, low;

        // Use umul_ppmm macro to directly calculate 128-bit result
        umul_ppmm(high, low, a, b);

        // Apply shift
        return (low >> shift) | (high << (64 - shift));
    }

    /**
     * @brief Signed 64-bit fixed-point multiplication, returns 64-bit result (using LLVM-style bit
     * operations for sign handling)
     *
     * Performs multiplication of two fixed-point numbers, then right-shifts by the specified number
     * of fraction bits. Uses bit operations for efficient sign handling, with optimal performance
     * for fractionBits=16.
     *
     * @param a First operand
     * @param b Second operand
     * @param fractionBits Number of fraction bits
     * @return 64-bit result, preserving specified fraction bits
     */
    [[nodiscard]] static constexpr auto Fixed64MulBitStyle(int64_t a,
                                                           int64_t b,
                                                           int fractionBits) noexcept -> int64_t {
        // Extract sign bits
        int64_t s_a = a >> 63;  // s_a = a < 0 ? -1 : 0
        int64_t s_b = b >> 63;  // s_b = b < 0 ? -1 : 0

        // Convert signed numbers to unsigned
        uint64_t a_abs = (static_cast<uint64_t>(a ^ s_a)) - s_a;  // If s_a is -1, invert and add 1
        uint64_t b_abs = (static_cast<uint64_t>(b ^ s_b)) - s_b;  // If s_b is -1, invert and add 1

        // Calculate result sign: XOR operation
        int64_t s_result = s_a ^ s_b;  // Result sign

        // Call unsigned multiplication
        uint64_t result = MulU64Shifted(a_abs, b_abs, fractionBits);

        // Apply sign: If s_result is -1, invert and add 1
        return (static_cast<int64_t>(result ^ s_result)) - s_result;
    }

    /**
     * @brief Signed 64-bit fixed-point multiplication, returns 64-bit result (using GCC-style sign
     * handling)
     *
     * Performs multiplication of two fixed-point numbers, then right-shifts by the specified number
     * of fraction bits. Uses GCC-style conditional handling for sign, and returns correct signed
     * result.
     *
     * @param a First operand
     * @param b Second operand
     * @param fractionBits Number of fraction bits
     * @return 64-bit result, preserving specified fraction bits
     */
    [[nodiscard]] static constexpr auto Fixed64Mul(int64_t a, int64_t b, int fractionBits) noexcept
        -> int64_t {
        // Use GCC-style sign handling
        int c1 = 0;  // Result sign flag

        // Handle sign of the first operand
        uint64_t a_abs = static_cast<uint64_t>(a);
        if (a < 0) {
            c1 = ~c1;            // Flip result sign
            a_abs = ~a_abs + 1;  // Get absolute value
        }

        // Handle sign of the second operand
        uint64_t b_abs = static_cast<uint64_t>(b);
        if (b < 0) {
            c1 = ~c1;            // Flip result sign
            b_abs = ~b_abs + 1;  // Get absolute value
        }

        // Call unsigned multiplication
        uint64_t result = MulU64Shifted(a_abs, b_abs, fractionBits);

        // Apply sign
        if (c1)
            return -static_cast<int64_t>(result);
        else
            return static_cast<int64_t>(result);
    }

    /**
     * @brief Divide 128-bit number by 64-bit number, returning 64-bit quotient
     *
     * Uses the udiv_qrnnd macro from GCC's longlong.h.
     * Normalizes dividend and divisor, and decomposes into single-word parts for calculation.
     * Referenced the __udivmoddi4 function in libgcc2.c.
     *
     * @param n1 High 64 bits of dividend
     * @param n0 Low 64 bits of dividend
     * @param d0 64-bit divisor, cannot be 0
     * @return 64-bit quotient
     */
    [[nodiscard]] static constexpr auto DivU128ToU64(uint64_t n1, uint64_t n0, uint64_t d0) noexcept
        -> uint64_t {
        // If d0 <= n1, overflow
        if (d0 <= n1) {
            return UINT64_MAX;
        }

        uint64_t q0;
        int bm = CountlZero(d0);

        if (bm != 0) {
            // Normalize
            d0 = d0 << bm;
            n1 = (n1 << bm) | (n0 >> (W_TYPE_SIZE - bm));
            n0 = n0 << bm;
        }

        udiv_qrnnd(q0, n0, n1, n0, d0);

        return q0;
    }

    /**
     * @brief Signed 64-bit fixed-point division, returns 64-bit quotient (using LLVM-style bit
     * operations for sign handling)
     *
     * Left-shifts dividend by specified number of fraction bits, then performs division,
     * Uses bit operations for efficient sign handling, with optimal performance for
     * fractionBits=16.
     *
     * @param n Dividend
     * @param d Divisor
     * @param fractionBits Number of fraction bits
     * @return 64-bit quotient, preserving specified fraction bits
     */
    [[nodiscard]] static constexpr auto Fixed64DivBitStyle(int64_t n,
                                                           int64_t d,
                                                           int fractionBits) noexcept -> int64_t {
        // Extract sign bits
        int64_t s_n = n >> 63;  // s_n = n < 0 ? -1 : 0
        int64_t s_d = d >> 63;  // s_d = d < 0 ? -1 : 0

        // Convert signed numbers to unsigned
        uint64_t n_abs = (static_cast<uint64_t>(n ^ s_n)) - s_n;  // If s_n is -1, invert and add 1
        uint64_t d_abs = (static_cast<uint64_t>(d ^ s_d)) - s_d;  // If s_d is -1, invert and add 1

        // Prepare 128-bit dividend: n_abs << fractionBits
        uint64_t n_lo = n_abs << fractionBits;
        uint64_t n_hi = n_abs >> (64 - fractionBits);

        // Calculate result sign: XOR operation
        int64_t s_result = s_n ^ s_d;  // Result sign

        // Call unsigned division
        uint64_t result_abs = DivU128ToU64(n_hi, n_lo, d_abs);

        // Apply sign: If s_result is -1, invert and add 1
        return (static_cast<int64_t>(result_abs ^ s_result)) - s_result;
    }

    /**
     * @brief Signed 64-bit fixed-point division, returns 64-bit quotient (using GCC-style sign
     * handling)
     *
     * Left-shifts dividend by specified number of fraction bits, then performs division,
     * Uses GCC-style conditional handling for sign, and returns correct signed result.
     *
     * @param n Dividend
     * @param d Divisor
     * @param fractionBits Number of fraction bits
     * @return 64-bit quotient, preserving specified fraction bits
     */
    [[nodiscard]] static constexpr auto Fixed64Div(int64_t n, int64_t d, int fractionBits) noexcept
        -> int64_t {
        // Use GCC-style sign handling
        int c1 = 0;  // Result sign flag

        // Handle sign of the dividend
        uint64_t n_abs = static_cast<uint64_t>(n);
        if (n < 0) {
            c1 = ~c1;            // Flip result sign
            n_abs = ~n_abs + 1;  // Get absolute value
        }

        // Handle sign of the divisor
        uint64_t d_abs = static_cast<uint64_t>(d);
        if (d < 0) {
            c1 = ~c1;            // Flip result sign
            d_abs = ~d_abs + 1;  // Get absolute value
        }

        // Prepare 128-bit dividend: n_abs << fractionBits
        uint64_t n_lo = n_abs << fractionBits;
        uint64_t n_hi = n_abs >> (64 - fractionBits);

        // Call unsigned division
        uint64_t result_abs = DivU128ToU64(n_hi, n_lo, d_abs);

        // Apply sign
        if (c1)
            return -static_cast<int64_t>(result_abs);
        else
            return static_cast<int64_t>(result_abs);
    }

    /**
     * @brief Divide 128-bit number by 64-bit number, returning quotient
     * @param numhi High 64 bits of 128-bit dividend
     * @param numlo Low 64 bits of 128-bit dividend
     * @param den 64-bit divisor
     * @return 64-bit quotient
     * @note Algorithm referenced from libdivide library
     * (https://github.com/ridiculousfish/libdivide) Uses based on normalization and binary
     * approximation without hardware divider
     */
    [[nodiscard]] static constexpr auto Divide128Div64To64(uint64_t numhi,
                                                           uint64_t numlo,
                                                           uint64_t den) noexcept -> uint64_t {
        // Overflow check
        if (numhi >= den) {
            return ~0ull;
        }

        // Calculate normalization shift factor
        int shift = CountlZero(den);

        // Normalize divisor, ensuring highest bit is 1
        den <<= shift;

        // Normalize dividend
        uint64_t n_hi = numhi << shift;
        if (shift > 0) {
            n_hi |= (numlo >> (64 - shift));
        }
        uint64_t n_lo = numlo << shift;

        // Extract two 32-bit digits from divisor and low two 32-bit digits from dividend
        uint32_t den1 = static_cast<uint32_t>(den >> 32);
        uint32_t den0 = static_cast<uint32_t>(den & 0xFFFFFFFFu);
        uint32_t num1 = static_cast<uint32_t>(n_lo >> 32);
        uint32_t num0 = static_cast<uint32_t>(n_lo & 0xFFFFFFFFu);

        // Calculate quotient high 32 bits
        uint64_t qhat = n_hi / den1;
        uint64_t rhat = n_hi % den1;
        uint64_t c1 = qhat * den0;
        uint64_t c2 = rhat * (1ULL << 32) + num1;

        if (c1 > c2) {
            qhat -= (c1 - c2 > den) ? 2 : 1;
        }

        uint32_t q1 = static_cast<uint32_t>(qhat);

        // Calculate partial remainder
        uint64_t rem = n_hi * (1ULL << 32) + num1 - q1 * den;

        // Calculate quotient low 32 bits
        qhat = rem / den1;
        rhat = rem % den1;
        c1 = qhat * den0;
        c2 = rhat * (1ULL << 32) + num0;

        if (c1 > c2) {
            qhat -= (c1 - c2 > den) ? 2 : 1;
        }

        uint32_t q0 = static_cast<uint32_t>(qhat);

        // Return final quotient
        return (static_cast<uint64_t>(q1) << 32) | q0;
    }

    /**
     * @brief Convert float to fixed-point representation
     * @param f Input float
     * @param fraction_bits Number of fraction bits in fixed-point format
     * @return Converted fixed-point raw value
     */
    static constexpr auto F32ToFixed64(float f, int fraction_bits) noexcept -> int64_t {
        uint32_t bits = std::bit_cast<uint32_t>(f);

        // Extract sign bit
        bool negative = (bits >> 31) != 0;

        // Extract exponent and mantissa
        int exponent = static_cast<int>((bits >> 23) & 0xFF) - 127;
        uint32_t mantissa = bits & 0x7FFFFF;

        // Handle special cases
        if (exponent == -127 && mantissa == 0) {  // Zero
            return 0;
        }
        if (exponent == 128) {  // NaN or infinity
            return negative ? INT64_MIN : INT64_MAX;
        }

        // Handle subnormal numbers
        if (exponent == -127) {  // Subnormal number
            exponent = -126;
            // Subnormal numbers do not have an implied leading 1
        } else {
            // Add implied leading 1 (for normal numbers)
            mantissa |= 0x800000;
        }

        // Extend mantissa to 64 bits
        uint64_t extMantissa = static_cast<uint64_t>(mantissa);

        // Calculate scaling factor
        int scaleFactor = exponent - 23 + fraction_bits;

        // Adjust mantissa based on scaling factor
        int64_t result;
        if (scaleFactor >= 0) {
            // Prevent overflow
            if (scaleFactor > 63) {
                result = INT64_MAX;
            } else {
                result = static_cast<int64_t>(extMantissa << scaleFactor);
            }
        } else {
            // Round to nearest integer (round-to-nearest)
            if (scaleFactor <= -64) {
                result = 0;
            } else {
                uint64_t roundBit = 1ULL << (-scaleFactor - 1);
                result = static_cast<int64_t>((extMantissa + roundBit) >> -scaleFactor);
            }
        }

        // Apply sign
        return negative ? -result : result;
    }

    /**
     * @brief Convert double to fixed-point representation
     * @param d Input double
     * @param fraction_bits Number of fraction bits in fixed-point format
     * @return Converted fixed-point raw value
     */
    static constexpr auto F64ToFixed64(double d, int fraction_bits) noexcept -> int64_t {
        uint64_t bits = std::bit_cast<uint64_t>(d);

        // Extract sign bit
        bool negative = (bits >> 63) != 0;

        // Extract exponent and mantissa
        int exponent = static_cast<int>((bits >> 52) & 0x7FF) - 1023;
        uint64_t mantissa = bits & 0xFFFFFFFFFFFFF;

        // Handle special cases
        if (exponent == -1023 && mantissa == 0) {  // Zero
            return 0;
        }
        if (exponent == 1024) {  // NaN or infinity
            return negative ? INT64_MIN : INT64_MAX;
        }

        // Handle subnormal numbers
        if (exponent == -1023) {  // Subnormal number
            exponent = -1022;
            // Subnormal numbers do not have an implied leading 1
        } else {
            // Add implied leading 1 (for normal numbers)
            mantissa |= 0x10000000000000;
        }

        // Calculate scaling factor
        int scaleFactor = exponent - 52 + fraction_bits;

        // Adjust mantissa based on scaling factor
        int64_t result;
        if (scaleFactor >= 0) {
            // Prevent overflow
            if (scaleFactor > 63) {
                result = INT64_MAX;
            } else {
                result = static_cast<int64_t>(mantissa << scaleFactor);
            }
        } else {
            // Round to nearest integer (round-to-nearest)
            if (scaleFactor <= -64) {
                result = 0;
            } else {
                uint64_t roundBit = 1ULL << (-scaleFactor - 1);
                result = static_cast<int64_t>((mantissa + roundBit) >> -scaleFactor);
            }
        }

        // Apply sign
        return negative ? -result : result;
    }

    /**
     * @brief Convert fixed-point to float
     * @param value Fixed-point value
     * @param fraction_bits Number of fraction bits in fixed-point format
     * @return Converted float
     * @note This implementation uses simple binary rounding, not the IEEE 754 standard banker's
     * rounding (round-to-even) Difference appears at binary rounding bit is 1 and subsequent bits
     * are 1000... (i.e., binary 0.5) Since float has only 23-bit mantissa precision, extra bits
     * need to be rounded, which may produce a difference of about 2^-23 Simple rounding is chosen
     * for performance, with negligible impact on calculation
     */
    static constexpr auto Fixed64ToF32(int64_t value, int fraction_bits) noexcept -> float {
        // Handle special cases
        if (value == 0) {
            return 0.0f;
        }

        // Extract sign bit
        bool negative = value < 0;
        // Special handling for INT64_MIN
        uint64_t abs_value = negative ? (value == INT64_MIN ? static_cast<uint64_t>(INT64_MAX) + 1
                                                            : static_cast<uint64_t>(-value))
                                      : value;

        // Find highest significant bit
        int msb = Primitives::BitWidth(abs_value) - 1;

        // Calculate exponent
        int exponent = msb - fraction_bits + 127;

        // Handle overflow and underflow
        if (exponent >= 255) {
            return negative ? -std::numeric_limits<float>::infinity()
                            : std::numeric_limits<float>::infinity();
        }
        if (exponent <= 0) {
            // Handle subnormal or underflow to 0
            if (exponent < -23) {
                return 0.0f;
            }
            // Subnormal number handling
            uint32_t mantissa = static_cast<uint32_t>(abs_value >> (fraction_bits - 1 - msb));
            mantissa = mantissa >> 1;  // Remove implied bit
            uint32_t bits = (negative ? 0x80000000u : 0u) | mantissa;
            return std::bit_cast<float>(bits);
        }

        // Extract mantissa (23 bits)
        uint32_t mantissa;
        if (msb >= 23) {
            // Ensure shift value is non-negative
            int shift = msb - 23;
            if (shift > 0) {
                uint64_t roundBit = 1ULL << (shift - 1);
                uint64_t rounded_value = abs_value + roundBit;

                // Check if rounding causes carry (value becomes next power of 2)
                if (rounded_value >> msb > 1) {
                    // Carry occurs, need to adjust exponent
                    exponent += 1;
                    mantissa = 0;  // When value is exactly power of 2, mantissa is 0
                } else {
                    mantissa = static_cast<uint32_t>(rounded_value >> shift);
                }
            } else {
                mantissa = static_cast<uint32_t>(abs_value);
            }
        } else {
            mantissa = static_cast<uint32_t>(abs_value << (23 - msb));
        }
        mantissa &= 0x7FFFFF;  // Keep 23 bits

        // Assemble IEEE 754 float
        uint32_t bits =
            (negative ? 0x80000000u : 0u) | (static_cast<uint32_t>(exponent) << 23) | mantissa;

        return std::bit_cast<float>(bits);
    }

    /**
     * @brief Convert fixed-point to double
     * @param value Fixed-point value
     * @param fraction_bits Number of fraction bits in fixed-point format
     * @return Converted double
     * @note This implementation uses simple binary rounding, not the IEEE 754 standard banker's
     * rounding (round-to-even) Difference appears at binary rounding bit is 1 and subsequent bits
     * are 1000... (i.e., binary 0.5) Since double has only 52-bit mantissa precision, extra bits
     * need to be rounded, which may produce a difference of about 2^-52 Simple rounding is chosen
     * for performance, with negligible impact on calculation
     */
    static constexpr auto Fixed64ToF64(int64_t value, int fraction_bits) noexcept -> double {
        // Handle special cases
        if (value == 0) {
            return 0.0;
        }

        // Extract sign bit
        bool negative = value < 0;
        // Special handling for INT64_MIN
        uint64_t abs_value = negative ? (value == INT64_MIN ? static_cast<uint64_t>(INT64_MAX) + 1
                                                            : static_cast<uint64_t>(-value))
                                      : value;

        // Find highest significant bit
        int msb = Primitives::BitWidth(abs_value) - 1;

        // Calculate exponent
        int exponent = msb - fraction_bits + 1023;

        // Handle overflow and underflow
        if (exponent >= 2047) {
            return negative ? -std::numeric_limits<double>::infinity()
                            : std::numeric_limits<double>::infinity();
        }
        if (exponent <= 0) {
            // Handle subnormal or underflow to 0
            if (exponent < -52) {
                return 0.0;
            }
            // Subnormal number handling
            uint64_t mantissa = abs_value >> (fraction_bits - 1 - msb);
            mantissa = mantissa >> 1;  // Remove implied bit
            uint64_t bits = (negative ? 0x8000000000000000ULL : 0ULL) | mantissa;
            return std::bit_cast<double>(bits);
        }

        // Extract mantissa (52 bits)
        uint64_t mantissa;
        if (msb >= 52) {
            // Ensure shift value is non-negative
            int shift = msb - 52;
            if (shift > 0) {
                uint64_t roundBit = 1ULL << (shift - 1);
                uint64_t rounded_value = abs_value + roundBit;

                // Check if rounding causes carry (value becomes next power of 2)
                if (rounded_value >> msb > 1) {
                    // Carry occurs, need to adjust exponent
                    exponent += 1;
                    mantissa = 0;  // When value is exactly power of 2, mantissa is 0
                } else {
                    mantissa = rounded_value >> shift;
                }
            } else {
                mantissa = abs_value;
            }
        } else {
            mantissa = abs_value << (52 - msb);
        }
        mantissa &= 0xFFFFFFFFFFFFFULL;  // Keep 52 bits

        // Assemble IEEE 754 double
        uint64_t bits = (negative ? 0x8000000000000000ULL : 0ULL)
                        | (static_cast<uint64_t>(exponent) << 52) | mantissa;

        return std::bit_cast<double>(bits);
    }
};
}  // namespace math::fp
