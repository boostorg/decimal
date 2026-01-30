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

## 5. Sqrt strategy: SoftFloat-style, split by precision

- **Binary**: Significand is a binary fraction; hardware `sqrt` or bit-level approximation + Newton on \(1.m \times 2^e\).
- **Decimal**: \(x = \mathit{sig} \times 10^e\) → \(\sqrt{x} = \sqrt{\mathit{sig}} \times 10^{e/2}\) (multiply by \(\sqrt{10}\) when e is odd). The core is **approximating sqrt of the normalized significand** in [1, 10), with **decimal precision** (e.g. 7 digits for decimal32).
- **Normalization**: Before table lookup or correction, normalize to a fixed range: gx ∈ [1, 10).

SoftFloat implements sqrt in **three separate files**: `f32_sqrt.c`, `f64_sqrt.c`, `f128_sqrt.c`. The **number of remainder corrections** increases with precision: f32 uses none (nudge only), f64 uses one, f128 uses three (each with an adjustment loop). Boost.Decimal mirrors this with **three implementation headers** and a single aggregate `sqrt.hpp`.

---

## 6. Two-table scheme and 2^e → 10^e mapping

The algorithm is the same; only the radix differs: **binary 2^e** vs **decimal 10^e**.

### Two-table scheme: (k0, k1) per bin, r = k0 − k1·eps

SoftFloat stores **two coefficients per bin**:

- **k0** (table `approxRecipSqrt_1k0s`): 1/√x at the **left edge** of the bin (intercept).
- **k1** (table `approxRecipSqrt_1k1s`): slope for that bin.

Within each bin, **eps** is the fractional position. Then:

```
r0 = k0 - k1 * eps   (with appropriate scale)
```

So **recip_sqrt** is a **piecewise linear** approximation to 1/√x. The same formula is used in decimal with gx ∈ [1, 10), step = 9/128, and decimal scaling.

### Flow: SoftFloat (2^e) vs Decimal (10^e)

| Step | SoftFloat (binary, 2^e) | Decimal (10^e) |
|------|--------------------------|----------------|
| **Normalize** | sig_a in [2^23, 2^24) or similar; exp_a odd/even | gx ∈ [1, 10); e_odd = e mod 2 |
| **Index** | index from sig_a bits + exp_a_odd | index = (gx−1)/step (and e_odd if needed) |
| **eps** | position in bin (bit extract) | eps = (gx − x_lo)/step |
| **r0** | r0 = k0[index] − (k1[index] × eps) >> 20 | r0 = k0[index] − k1[index] × eps (decimal scale) |
| **Initial z** | z = sig_a × r | z = gx × r |
| **Remainder** | rem = sig_a − z² | rem = gx − z² |
| **Correction** | q = (rem >> 2) × r; z = z + q | q = rem × r / 2; z = z + q |
| **Rescale** | result = z × 2^(exp/2); ×√2 if exp odd | result = z × 10^(e/2); ×√10 if e odd |

Mathematically, the correction is the Newton variant:

$$\sqrt{x} \approx z + \frac{x - z^2}{2\sqrt{x}} = z + \frac{\text{rem}}{2} \cdot \frac{1}{\sqrt{x}}$$

**Important**: Initial value must be **z = gx × recip_sqrt** (from the 1/√x table). Do **not** use linear interpolation of a √x table for z; on [1, 10), √x is concave, so that would systematically overestimate.

### Why decimal can do the same

In BID format, the decimal significand is an integer. Remainder and scaling are exact; the only difference from SoftFloat is radix and scaling (bit shift vs power-of-10).

| Operation | Binary (2^e) | Decimal (10^e) |
|-----------|--------------|----------------|
| **rem = x − z²** | Integer subtraction | Integer subtraction (align powers of 10) |
| **q** | (rem>>2) × r | rem × r / 2 |
| **Scaling** | >> n, << n | ×10^n, /10^n |

---

## 7. SoftFloat f32 / f64 / f128 flows and decimal mapping

SoftFloat splits sqrt into **f32_sqrt.c**, **f64_sqrt.c**, and **f128_sqrt.c**. The main difference is **how many remainder corrections** are applied: higher precision → more corrections.

### 7.1 f32_sqrt (23-bit significand ≈ decimal32’s 7 decimal digits)

```
1. recipSqrt32 = softfloat_approxRecipSqrt32_1(expA, sigA)
2. sigZ = (sigA * recipSqrt32) >> 32
3. if (expA) sigZ >>= 1
4. sigZ += 2                                    // nudge; NO remainder loop
5. Final rounding: if ((sigZ & 0x3F) < 2) { negRem = (sigZ>>2)²; adjust ±1 }
```

**Key**: **Zero** explicit remainder correction loops; only nudge (+2) and final rounding.

### 7.2 f64_sqrt (52-bit significand ≈ decimal64’s 16 decimal digits)

```
1. sig32A = sigA >> 21
2. recipSqrt32 = softfloat_approxRecipSqrt32_1(expA, sig32A)
3. sig32Z = (sig32A * recipSqrt32) >> 32
4. rem = sigA - sig32Z²
5. q = ((rem >> 2) * recipSqrt32) >> 32         // ONE correction, NO loop
6. sigZ = (sig32Z << 32 | 1<<5) + (q << 3)
7. Final rounding: if ((sigZ & 0x1FF) < 0x22) { rem2; if rem<0 --sigZ else if rem sigZ|=1 }
```

**Key**: **One** remainder correction; no adjustment loop.

### 7.3 f128_sqrt (112-bit significand ≈ decimal128’s 34 decimal digits)

```
1. sig32A = sigA.v64 >> 17
2. recipSqrt32 = softfloat_approxRecipSqrt32_1(expA, sig32A)
3. sig32Z = (sig32A * recipSqrt32) >> 32
4. rem = sigA - sig32Z²; qs[2] = sig32Z
5. First correction: q = (rem>>2)*recipSqrt32>>32; sig64Z = sig32Z<<32 + q<<3
   Loop: while (rem < 0) { --q; sig64Z -= 8; recompute rem }
6. Second correction: q = (rem>>2)*recipSqrt32>>32; loop while (rem < 0) { --q }
7. Third correction: q = ((rem>>2)*recipSqrt32>>32) + 2    // note the +2
8. Assemble sigZ from qs[2], qs[1], qs[0], q
9. Final rounding: complex multi-word remainder check
```

**Key**: **Three** remainder corrections, each with an adjustment loop (decrement q while rem < 0); third q includes +2.

### 7.4 Comparison

| | f32 | f64 | f128 |
|---|-----|-----|------|
| Significand bits | 23 | 52 | 112 |
| Initial approx | 32-bit recipSqrt | 32-bit recipSqrt | 32-bit recipSqrt |
| Remainder corrections | **0** (nudge only) | **1** | **3** (with loops) |
| Final rounding | 6-bit mask | 9-bit mask | Multi-word rem |

### 7.5 Decimal implementation layout

The implementation follows the same structure as SoftFloat, in header-only form:

| File | Corresponds to | Flow |
|------|----------------|------|
| `impl/sqrt_tables.hpp` | SoftFloat internals (k0/k1 tables) | Shared k0/k1 tables + `approx_recip_sqrt_1()` |
| `impl/sqrt32_impl.hpp` | f32_sqrt.c | z = gx×r → nudge → rounding (rem<0 → z−=1ULP, rem>0 → z+=1ULP) |
| `impl/sqrt64_impl.hpp` | f64_sqrt.c | z = gx×r → **one** q = rem×r/2, z+=q → rounding (rem<0 → z−=1ULP, rem>0 → z+=1ULP) |
| `impl/sqrt128_impl.hpp` | f128_sqrt.c | z = gx×r → **three** corrections (q = rem×r/2; third adds +2); each with while(rem<0) z−=1ULP → final rounding |
| `sqrt.hpp` | — | Special cases, fast path, tag dispatch to sqrt32/64/128_impl |

| Decimal type | Decimal digits | ~Binary bits | SoftFloat analogue | Remainder corrections |
|--------------|----------------|--------------|--------------------|------------------------|
| decimal32 | 7 | ~23 | f32 | 0 (nudge only) |
| decimal64 | 16 | ~53 | f64 | 1 |
| decimal128 | 34 | ~113 | f128 | 3 (with adjustment loops) |

### 7.6 Mapping verification (2^e → 10^e faithfulness)

The following mismatches were found and corrected so that the implementation strictly follows SoftFloat’s flow with only 2^e → 10^e mapping:

| Issue | Location | SoftFloat behavior | Fix |
|-------|----------|--------------------|-----|
| f128 third q missing +2 | sqrt128_impl.hpp | `q = ((rem>>2)*recipSqrt32>>32) + 2` | Third correction: `z = z + q + nudge2` (nudge2 = 2×10^−digits10) |
| f64 rounding only for rem<0 | sqrt64_impl.hpp | `if (rem<0) --sigZ; else if (rem) sigZ \|= 1` | Add `else if (rem > 0) z += tiny` |
| f32 rounding only for rem<0 | sqrt32_impl.hpp | f32 uses negRem=(sigZ/4)² for two branches | 10^e mapping: rem<0 → z−=tiny, rem>0 → z+=tiny |
| Comment mentioned “Newton” | sqrt.hpp | f128 has no Newton step | Comment changed to “three corrections, each with adjustment loop” |

**Conclusion**: Only 2^e → 10^e scaling and operation mapping is applied; no extra steps (e.g. Newton) are added. Rounding matches f32/f64 rem direction; f128’s +2 in the third q is included.

---

*References: IEEE 754-2019, Boost.Decimal `conversions.adoc`, `decimal32_t.hpp` comments, SoftFloat library by John R. Hauser.*
