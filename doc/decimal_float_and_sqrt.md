# Decimal vs Binary Float: Representation, Bit Layout, and Sqrt Optimization

## 1. Numeric meaning

| | Binary (float/double) | Decimal (decimal32/64/128) |
|---|------------------------|----------------------------|
| **Formula** | \((-1)^s \times 1.m \times 2^e\) | \((-1)^s \times \mathit{sig} \times 10^e\) |
| **Significand** | Binary fraction \(m\) (bit sequence) | Decimal integer \(\mathit{sig}\) (e.g. 1–9999999) |
| **Radix** | 2 | 10 |
| **Single value** | Unique representation | Multiple representations (cohorts, e.g. 1e5 = 10e4) |
| **Hardware sqrt** | Yes | No |

```
Binary:   value = sign × (1.xxxx₂) × 2^exp
Decimal:  value = sign × (integer sig) × 10^exp
```

---

## 2. Bit layout

### Binary32 (float) — 32 bits

```
 31  30────22  21────────────────────0
 ┌───┬─────────┬──────────────────────┐
 │ s │  exp    │   mantissa (1.m)     │
 │ 1 │   8     │        23            │
 └───┴─────────┴──────────────────────┘
 sign  biased exp   binary fraction
```

### Decimal32 (BID) — 32 bits (Boost.Decimal internal `bits_`)

```
 31  30-29  28────21  20─────────────────0
 ┌───┬──────┬─────────┬──────────────────┐
 │ s │ comb │  exp    │  significand     │
 │ 1 │  2   │ 6 or 8  │  21 or 23        │
 └───┴──────┴─────────┴──────────────────┘
 sign  comb field  decimal biased exp  integer significand (stored in binary)
```

- **Combination field**: Encodes whether exponent uses 6 or 8 bits and significand 21 or 23 bits; semantics remain "decimal exponent + integer significand".

---

## 3. Examples: 0.1 and 100 — representation and bits

### 0.1: Why float/double cannot represent it exactly

0.1 = 1/10. In **binary**, 1/10 is a recurring fraction:

```
1/10 = 0.0001 1001 1001 1001 1001 …₂  (0011 repeats)
```

Binary floating-point has only a finite mantissa, so it must round; **float/double cannot represent 0.1 exactly**, only the nearest representable value.

| Type   | Stored value for 0.1 (actual)   | Hex (bits)              |
|--------|----------------------------------|-------------------------|
| float  | ≈ 0.10000000149011612            | `0x3DCCCCCD`            |
| double | ≈ 0.10000000000000000555…        | `0x3FB999999999999A`    |

Hence `0.1 + 0.2 == 0.3` can be false for float/double; decimal represents these exactly in base 10.

### 0.1 in decimal32 (exact)

0.1 = **1 × 10⁻¹** → sig=1, e=−1. decimal32 bias is 101, so stored exponent = 100.

- BID 32-bit: `bits_ = 0x00400001`
- Binary (8-bit groups):
  ```
  0000 0000  0100 0000  0000 0000  0000 0001
  └─ s=0      └─ exp=100 (0x64)    └─ sig=1
  ```
- Fields: `s=0` | 8-bit exp=100 | 23-bit sig=1 → **0x00400001** (exact 0.1)

### 100 in decimal32

100 = **1 × 10²** → sig=1, e=2. Stored exponent = 2 + 101 = 103.

- BID 32-bit: `bits_ = (103 << 23) | 1 = 0x00C00001`
- Binary sketch: `0_01100111_00000000000000000000001` (s=0, exp=103, sig=1)

---

## 4. Summary comparison

```
         Binary float              Decimal (BID)
         ─────────────              ─────────────
Storage  sign + exp + mantissa      sign + comb + exp + significand
Sig      binary fraction (1.m)      integer sig (binary encoding)
Exp      2^exp                      10^exp
sqrt     hardware or iterate on     software, sqrt of integer sig
         binary mantissa
```

---

## 5. Sqrt Implementation: SoftFloat-Inspired Design

### 5.1 Core Algorithm

Both SoftFloat (binary) and Boost.Decimal (decimal) use the same fundamental approach:

1. **Normalize** input to a fixed range
2. **Table lookup** for initial 1/√x approximation  
3. **Newton-Raphson iterations** to refine precision
4. **Final rounding check** to ensure z² ≤ x
5. **Rescale** result by exponent

The key difference is **radix**: binary uses 2^e, decimal uses 10^e.

### 5.2 SoftFloat's Key Innovation: 16-Entry Table + Built-in Refinement

SoftFloat uses only **16 table entries** but achieves ~32-bit precision through:

```c
// softfloat_approxRecipSqrt32_1 (simplified)
uint32_t softfloat_approxRecipSqrt32_1(unsigned int oddExpA, uint32_t a)
{
    // 1. Table lookup with 16-bit interpolation → ~16 bits
    int index = (a>>27 & 0xE) + oddExpA;  // 0-15
    uint16_t eps = (uint16_t)(a>>12);
    uint16_t r0 = k0s[index] - ((k1s[index] * eps) >> 20);
    
    // 2. Built-in Newton refinement → ~32 bits
    uint32_t ESqrR0 = r0 * r0;
    uint32_t sigma0 = ~((ESqrR0 * a) >> 23);  // σ = 1 - a*r0²
    uint32_t r = (r0 << 16) + ((r0 * sigma0) >> 25);
    uint32_t sqrSigma0 = (sigma0 * sigma0) >> 32;
    r += ((r/2 + r/8 - (r0<<14)) * sqrSigma0) >> 48;
    
    return r;  // 32-bit precision output
}
```

**All operations are exact integer arithmetic** — no floating-point rounding.

### 5.3 Decimal Adaptation

For decimal, we adapt this approach with **90-entry table** (step=0.1) covering [1,10):

| Aspect | SoftFloat (2^e) | Decimal (10^e) |
|--------|-----------------|----------------|
| Normalize range | [1, 2) or [2, 4) | [1, 10) |
| Table entries | 16 | 90 |
| Index calculation | `(a>>27 & 0xE) + oddExp` | `(gx - 1) * 10` |
| Interpolation | 16-bit eps, bit shifts | Decimal fraction |
| Odd exponent | ×√2 (shift) | ×√10 |

---

## 6. Integer Arithmetic Implementation

### 6.1 Why Integer Arithmetic Matters

SoftFloat's strength is that **remainder calculation is exact**:

```c
// SoftFloat: exact integer subtraction
uint64_t rem = sigA - (uint64_t)sig32Z * sig32Z;
```

Floating-point would introduce rounding errors in `z*z` and the subtraction.

### 6.2 Decimal32 Integer Implementation

For decimal32 (7 digits), we use `uint64` for z²:

```cpp
// Integer coefficient extraction
constexpr uint64_t scale = 1000000ULL;  // 10^6
uint32_t sig_gx = static_cast<uint32_t>(gx * T{scale});  // 7 digits
uint32_t sig_z  = static_cast<uint32_t>(z * T{scale});   // 7 digits

// Exact z² calculation (14 digits fits in uint64)
uint64_t z_squared = static_cast<uint64_t>(sig_z) * sig_z;

// Exact remainder (scaled by 10^12)
int64_t rem = static_cast<int64_t>(sig_gx) * scale - static_cast<int64_t>(z_squared);

// Newton correction: q = rem / (2 * sig_z)
int64_t correction = rem / (2 * static_cast<int64_t>(sig_z));
sig_z += correction;
```

### 6.3 Decimal64 Integer Implementation

For decimal64 (16 digits), we need `uint128` for z²:

```cpp
#if defined(__SIZEOF_INT128__)
using uint128_t = __uint128_t;
using int128_t = __int128_t;

constexpr uint64_t scale = 1000000000000000ULL;  // 10^15
uint64_t sig_gx = static_cast<uint64_t>(gx * T{scale});  // 16 digits
uint64_t sig_z  = static_cast<uint64_t>(z * T{scale});   // 16 digits

// Exact z² calculation (32 digits fits in uint128)
uint128_t z_squared = static_cast<uint128_t>(sig_z) * sig_z;

// Exact remainder (scaled by 10^30)
int128_t rem = static_cast<int128_t>(sig_gx) * scale - static_cast<int128_t>(z_squared);

// Newton correction
int128_t correction = rem / (2 * static_cast<int128_t>(sig_z));
sig_z += correction;
#endif
```

### 6.4 Decimal128 Limitation

For decimal128 (34 digits), z² requires 68 digits (uint256 with subtraction).
Current `u256.hpp` lacks subtraction operator, so we use floating-point Newton iterations with integer final rounding check.

| Type | Coefficient | z² Width | Integer Type | Status |
|------|-------------|----------|--------------|--------|
| decimal32 | 7 digits | 14 digits | uint64 | ✅ Full integer |
| decimal64 | 16 digits | 32 digits | uint128 | ✅ Full integer (with `__int128`) |
| decimal128 | 34 digits | 68 digits | uint256 | ⚠️ Floating-point Newton |

---

## 7. Implementation Files and Precision Analysis

### 7.1 File Structure

```
include/boost/decimal/detail/cmath/
├── sqrt.hpp                    # Entry point, special cases, dispatch
└── impl/
    ├── sqrt_tables.hpp         # 90-entry k0/k1 tables (step=0.1)
    ├── sqrt32_impl.hpp         # decimal32: 1 Newton + integer rem
    ├── sqrt64_impl.hpp         # decimal64: 2 Newton + integer rem
    ├── sqrt128_impl.hpp        # decimal128: 4 Newton + FP rem
    └── approx_recip_sqrt.hpp   # Integer approximation functions
```

### 7.2 Table Design

**90 entries** covering [1, 10) with step = 0.1:

```cpp
// k0[i] = 10^16 / sqrt(1 + i * 0.1), at left edge of bin
// k1[i] = k0[i] - k0[i+1], slope for linear interpolation
static constexpr uint64_t approxRecipSqrt_1k0s[90] = {
    10000000000000000ULL,  // 1/sqrt(1.0) = 1.0
    9534625892455923ULL,   // 1/sqrt(1.1) ≈ 0.953
    9128709291752768ULL,   // 1/sqrt(1.2) ≈ 0.913
    ...
    3178208630818641ULL    // 1/sqrt(9.9) ≈ 0.318
};
```

Linear interpolation: `r = k0[i] - k1[i] * eps` gives ~10 bits initial precision.

### 7.3 Precision Chain

| Type | Table | After N1 | After N2 | After N3 | After N4 | Target |
|------|-------|----------|----------|----------|----------|--------|
| decimal32 | ~10 bits | ~20 bits | - | - | - | 23 bits ✅ |
| decimal64 | ~10 bits | ~20 bits | ~40 bits | - | - | 53 bits ✅ |
| decimal128 | ~10 bits | ~20 bits | ~40 bits | ~80 bits | ~160 bits | 113 bits ✅ |

### 7.4 Comparison with SoftFloat

| Aspect | SoftFloat | Boost.Decimal |
|--------|-----------|---------------|
| Table entries | 16 (with built-in refinement) | 90 (simple interpolation) |
| Remainder arithmetic | Integer (exact) | Integer for decimal32/64, FP for decimal128 |
| Newton iterations | 0/1/3 (f32/f64/f128) | 1/2/4 (decimal32/64/128) |
| Odd exponent adjustment | ×√2 (bit shift) | ×√10 (multiply constant) |

---

## 8. Algorithm Flow Summary

```
┌─────────────────────────────────────────────────────────────────────┐
│                         sqrt(x) Algorithm                          │
├─────────────────────────────────────────────────────────────────────┤
│  1. Handle special cases (NaN, 0, negative, infinity)              │
│                                                                     │
│  2. Normalize: x → gx ∈ [1, 10), track exponent e                  │
│                                                                     │
│  3. Table lookup:                                                   │
│     index = floor((gx - 1) * 10)                                   │
│     eps = (gx - 1) * 10 - index                                    │
│     r = k0[index] - k1[index] * eps    (~10 bits 1/√gx)           │
│                                                                     │
│  4. Initial approximation: z = gx * r                              │
│                                                                     │
│  5. Newton iterations (INTEGER arithmetic):                        │
│     sig_gx = integer(gx * scale)                                   │
│     sig_z = integer(z * scale)                                     │
│     z² = sig_z * sig_z                   (exact)                   │
│     rem = sig_gx * scale - z²            (exact)                   │
│     correction = rem / (2 * sig_z)                                 │
│     sig_z += correction                                            │
│     (repeat for required precision)                                │
│                                                                     │
│  6. Final rounding: if rem < 0, sig_z -= 1                         │
│                                                                     │
│  7. Rescale: result = z * 10^(e/2) * (√10 if e is odd)            │
└─────────────────────────────────────────────────────────────────────┘
```

---

## 9. Key Implementation Details

### 9.1 Index Calculation for [1, 10)

```cpp
// Perfect decimal alignment with 90-entry table
T idx_real = (gx - one) * ten;  // [0, 90)
int index = static_cast<int>(idx_real);
T eps = idx_real - T{index};    // [0, 1)
```

### 9.2 Integer Remainder Formula

For Newton-Raphson sqrt: z' = z + (x - z²)/(2z)

In integer form with scaling:
- sig_x scaled by S
- sig_z scaled by S
- z² scaled by S²
- rem = sig_x × S - sig_z² (scaled by S²)
- correction = rem / (2 × sig_z) (scaled by S)

### 9.3 Final Rounding

Ensure result satisfies z² ≤ x (z is a lower bound):

```cpp
// After last Newton iteration
z² = sig_z * sig_z;
rem = sig_gx * scale - z²;
if (rem < 0) {
    --sig_z;  // z was too large, decrease by 1 ULP
}
```

---

*References: IEEE 754-2019, SoftFloat 3e by John R. Hauser, Boost.Decimal implementation.*
