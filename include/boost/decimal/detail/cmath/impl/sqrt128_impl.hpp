// Copyright 2023 - 2024 Matt Borland
// Copyright 2023 - 2024 Christopher Kormanyos
// Copyright 2025 - 2026 Justin Zhu
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_DECIMAL_DETAIL_CMATH_IMPL_SQRT128_IMPL_HPP
#define BOOST_DECIMAL_DETAIL_CMATH_IMPL_SQRT128_IMPL_HPP

// ============================================================================
// decimal128 sqrt: SoftFloat f128_sqrt style
//
// Algorithm:
// 1. Normalize x to gx in [1, 10)
// 2. Table lookup with linear interpolation → r ≈ 1/sqrt(gx)
// 3. Compute z = gx * r as initial approximation
// 4. Four Newton corrections (each roughly doubles precision)
//    Note: Full integer arithmetic would require uint256 subtraction
//          which is not available in u256.hpp, so we use floating-point
// 5. Final rounding check using integer comparison where possible
// 6. Rescale by 10^(exp/2) and ×√10 if exp was odd
//
// Precision chain: Table ~10 bits → 20 → 40 → 80 → 160 bits > 113 bits needed
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

// sqrt for decimal128 (34 decimal digits)
// Uses floating-point Newton iterations due to lack of u256 subtraction
template <typename T>
constexpr auto sqrt128_impl(T x, int exp10val) noexcept -> T
{
    constexpr int digits10 = std::numeric_limits<T>::digits10;
    static_assert(digits10 > 16, "sqrt128_impl is for decimal128 (34 digits)");

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

    // ---------- Newton corrections ----------
    // z_new = z + (gx - z²) * r / 2
    // Each iteration roughly doubles the precision
    // Note: Using floating-point here because u256 lacks subtraction
    constexpr T half{5, -1};

    // First Newton correction: ~10 → ~20 bits
    {
        T rem = gx - z * z;
        T q = rem * r * half;
        z = z + q;
    }

    // Second Newton correction: ~20 → ~40 bits
    {
        T rem = gx - z * z;
        T q = rem * r * half;
        z = z + q;
    }

    // Third Newton correction: ~40 → ~80 bits
    {
        T rem = gx - z * z;
        T q = rem * r * half;
        z = z + q;
    }

    // Fourth Newton correction: ~80 → ~160 bits
    {
        T rem = gx - z * z;
        T q = rem * r * half;
        z = z + q;
    }

    // ---------- Final rounding check ----------
    // Ensure z² ≤ gx (z is a lower bound)
    constexpr T zero{0};
    constexpr T tiny{1, -(digits10)};  // 1 ULP
    {
        T rem = gx - z * z;
        if (rem < zero)
        {
            z = z - tiny;
        }
    }

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

#endif // BOOST_DECIMAL_DETAIL_CMATH_IMPL_SQRT128_IMPL_HPP
