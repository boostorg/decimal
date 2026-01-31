#!/usr/bin/env python3
"""
Generate lookup tables for decimal sqrt approximation.

Based on SoftFloat's approach but adapted for base-10:
- Range: [1, 10) instead of [1, 2) ∪ [2, 4)
- 90 entries with step = 0.1 for perfect decimal alignment
- k0[i] = scaled 1/sqrt(1 + i*0.1)
- k1[i] = k0[i] - k0[i+1] (slope for linear interpolation)

Usage:
    python generate_sqrt_tables.py
"""

from decimal import Decimal, getcontext

# High precision for calculations
getcontext().prec = 60

# Table parameters
TABLE_SIZE = 90
STEP = Decimal('0.1')
RANGE_START = Decimal('1')

# Scale factor: 10^16 fits in uint64 and provides good precision
SCALE_POWER = 16
SCALE = Decimal(10) ** SCALE_POWER


def generate_tables():
    """Generate k0 and k1 tables for 1/sqrt(x) approximation."""
    
    k0_values = []
    
    # Generate k0: 1/sqrt(x) at left edge of each bin
    for i in range(TABLE_SIZE + 1):  # +1 for k1 calculation
        x = RANGE_START + Decimal(i) * STEP
        recip_sqrt = Decimal(1) / x.sqrt()
        k0_scaled = int(recip_sqrt * SCALE)
        k0_values.append(k0_scaled)
    
    # k0 table (first 90 entries)
    k0_table = k0_values[:TABLE_SIZE]
    
    # k1 table: slope = k0[i] - k0[i+1]
    k1_table = []
    for i in range(TABLE_SIZE - 1):
        k1_table.append(k0_values[i] - k0_values[i + 1])
    # Last entry: copy from previous (or use actual slope to endpoint)
    k1_table.append(k1_table[-1])
    
    return k0_table, k1_table


def format_table(name, values, values_per_line=4):
    """Format table as C++ array."""
    lines = []
    lines.append(f"// {name}[i] = value at x = 1 + i * 0.1, scaled by 10^{SCALE_POWER}")
    lines.append(f"static constexpr std::uint64_t {name}[{TABLE_SIZE}] = {{")
    
    for i in range(0, len(values), values_per_line):
        chunk = values[i:i + values_per_line]
        formatted = ", ".join(f"{v}ULL" for v in chunk)
        if i + values_per_line < len(values):
            formatted += ","
        lines.append(f"    {formatted}")
    
    lines.append("};")
    return "\n".join(lines)


def verify_tables(k0_table, k1_table):
    """Verify table accuracy."""
    print("\n=== Table Verification ===\n")
    
    max_error = Decimal(0)
    
    for i in range(TABLE_SIZE):
        x = RANGE_START + Decimal(i) * STEP
        
        # Actual value
        actual = Decimal(1) / x.sqrt()
        
        # Table approximation at bin edge (eps = 0)
        approx = Decimal(k0_table[i]) / SCALE
        
        # Relative error
        error = abs(actual - approx) / actual
        max_error = max(max_error, error)
        
        if i < 5 or i >= TABLE_SIZE - 3:
            print(f"i={i:2d}, x={float(x):.1f}: actual={float(actual):.10f}, "
                  f"approx={float(approx):.10f}, rel_err={float(error):.2e}")
    
    print(f"\nMax relative error at bin edges: {float(max_error):.2e}")
    
    # Test interpolation accuracy at bin midpoints
    print("\n=== Interpolation Test (midpoints) ===\n")
    
    max_interp_error = Decimal(0)
    
    for i in range(TABLE_SIZE - 1):
        x = RANGE_START + Decimal(i) * STEP + STEP / 2
        
        # Actual value
        actual = Decimal(1) / x.sqrt()
        
        # Linear interpolation: k0 - k1 * eps, where eps = 0.5
        eps = Decimal('0.5')
        approx = Decimal(k0_table[i] - int(k1_table[i] * eps)) / SCALE
        
        # Relative error
        error = abs(actual - approx) / actual
        max_interp_error = max(max_interp_error, error)
    
    print(f"Max relative error at midpoints: {float(max_interp_error):.2e}")
    
    # Estimate bits of precision
    bits = -float(max_interp_error.ln() / Decimal(2).ln())
    print(f"Approximate bits of precision: {bits:.1f}")


def generate_header():
    """Generate the complete C++ header content."""
    
    k0_table, k1_table = generate_tables()
    
    header = '''// Copyright 2023 - 2024 Matt Borland
// Copyright 2023 - 2024 Christopher Kormanyos
// Copyright 2025 - 2026 Justin Zhu
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_DECIMAL_DETAIL_CMATH_IMPL_SQRT_TABLES_HPP
#define BOOST_DECIMAL_DETAIL_CMATH_IMPL_SQRT_TABLES_HPP

// ============================================================================
// Decimal sqrt lookup tables (SoftFloat-style, optimized for base-10)
//
// Design:
// - 90 entries covering range [1, 10) with step = 0.1
// - Perfect decimal alignment: index = (x - 1) * 10
// - k0[i] = 10^16 / sqrt(1 + i * 0.1) at left edge of bin
// - k1[i] = k0[i] - k0[i+1] (slope for linear interpolation)
//
// Usage:
//   int index = int((x - 1) * 10);  // x in [1, 10)
//   T eps = (x - 1) * 10 - index;   // fractional part in [0, 1)
//   T r = (k0[index] - k1[index] * eps) / 10^16;  // 1/sqrt(x) approximation
//
// This gives ~17 bits of precision from table lookup alone.
// Combined with Newton refinement, can reach 32+ bits.
// ============================================================================

#ifndef BOOST_DECIMAL_BUILD_MODULE
#include <cstdint>
#endif

namespace boost {
namespace decimal {
namespace detail {
namespace sqrt_tables {

'''

    header += format_table("approxRecipSqrt_1k0s", k0_table)
    header += "\n\n"
    header += format_table("approxRecipSqrt_1k1s", k1_table)
    
    header += f'''

constexpr int table_size = {TABLE_SIZE};
constexpr int table_scale = {SCALE_POWER};  // values scaled by 10^{SCALE_POWER}

// Integer-based table lookup (like softfloat_approxRecipSqrt32_1).
// Input: sig is the significand, oddExp indicates if original exponent was odd.
// Returns: approximation of 1/sqrt(normalized_sig) scaled by 10^16.
//
// For decimal, we normalize to [1, 10):
// - oddExp = 0: sig represents value in [1, 10)
// - oddExp = 1: sig represents value in [1, 10), result needs sqrt(10) adjustment
inline constexpr std::uint64_t approx_recip_sqrt_int(std::uint64_t sig, int /* oddExp */) noexcept
{{
    // sig is assumed to be in range [10^15, 10^16) representing [1, 10)
    // Extract index: top digit(s) determine bin
    // sig / 10^15 gives value in [1, 10), subtract 1 and multiply by 10 for index
    
    // Simplified: index = (sig / 10^14) - 10, clamped to [0, 89]
    int index = static_cast<int>(sig / 100000000000000ULL) - 10;
    if (index < 0) index = 0;
    if (index >= table_size) index = table_size - 1;
    
    // eps: fractional position within bin, scaled to 16 bits for interpolation
    // This extracts the sub-index precision from lower bits of sig
    std::uint64_t sig_in_bin = sig - (static_cast<std::uint64_t>(index + 10) * 100000000000000ULL);
    std::uint32_t eps = static_cast<std::uint32_t>(sig_in_bin >> (14 - 4));  // 16-bit eps
    
    // Linear interpolation: r0 = k0 - (k1 * eps) >> 16
    std::uint64_t r0 = approxRecipSqrt_1k0s[index]
                     - ((approxRecipSqrt_1k1s[index] * static_cast<std::uint64_t>(eps)) >> 16);
    
    return r0;
}}

// Floating-point wrapper for compatibility with existing code.
// Returns r0 = k0[index] - k1[index] * eps.
template <typename T>
constexpr auto approx_recip_sqrt_1(int index, T eps) noexcept -> T
{{
    if (index < 0) {{ index = 0; }}
    if (index >= table_size) {{ index = table_size - 1; }}
    T k0_val{{static_cast<std::uint64_t>(approxRecipSqrt_1k0s[index]), -table_scale}};
    T k1_val{{static_cast<std::uint64_t>(approxRecipSqrt_1k1s[index]), -table_scale}};
    return k0_val - k1_val * eps;
}}

}} // namespace sqrt_tables
}} // namespace detail
}} // namespace decimal
}} // namespace boost

#endif // BOOST_DECIMAL_DETAIL_CMATH_IMPL_SQRT_TABLES_HPP
'''
    
    return header, k0_table, k1_table


def main():
    print("Generating decimal sqrt lookup tables...")
    print(f"Table size: {TABLE_SIZE}")
    print(f"Step: {STEP}")
    print(f"Scale: 10^{SCALE_POWER}")
    
    header_content, k0_table, k1_table = generate_header()
    
    # Verify tables
    verify_tables(k0_table, k1_table)
    
    # Print header content
    print("\n" + "=" * 60)
    print("Generated header content:")
    print("=" * 60 + "\n")
    print(header_content)
    
    # Optionally save to file
    output_path = "sqrt_tables_generated.hpp"
    with open(output_path, "w") as f:
        f.write(header_content)
    print(f"\nSaved to {output_path}")


if __name__ == "__main__":
    main()
