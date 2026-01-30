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

## 5. Implications for sqrt optimization

- **Binary**: Significand is a binary fraction; can use hardware `sqrt` or bit-level approximation + Newton on \(1.m \times 2^e\).
- **Decimal**: \(x = \mathit{sig} \times 10^e\) → \(\sqrt{x} = \sqrt{\mathit{sig}} \times 10^{e/2}\) (multiply by \(\sqrt{10}\) when e is odd). The core is **integer sqrt of the significand**, and must meet **decimal precision** (e.g. 7 decimal digits for decimal32), not binary 0.5 ULP.
- **Cohorts**: Before table lookup or iteration, normalize to a fixed range (e.g. \(\mathit{sig} \in [1, 10)\)), then look up or iterate.

---

## 6. SoftFloat-style remainder elimination: Can decimal do it?

Algorithm is the same; only the radix differs: **binary 2^e** vs **decimal 10^e**. Below, the flow is written in parallel so each step has a direct decimal counterpart.

### Two-table scheme: (k0, k1) per bin, r = k0 − k1·eps

SoftFloat does **not** store a single 1/√x value per bin. It stores **two coefficients per bin**:

- **k0** (table `approxRecipSqrt_1k0s`): base approximation of 1/√x at the bin (intercept).
- **k1** (table `approxRecipSqrt_1k1s`): first-order correction coefficient (slope).

Within each bin, the position is encoded by **eps** (fractional part within the bin). Then:

```
r0 = k0 - k1 * eps   (with appropriate scale/shift)
```

So **recip_sqrt** is a **piecewise linear** approximation to 1/√x: each bin has its own (k0, k1), giving a line segment instead of a constant. That keeps error small without increasing the number of bins.

**Decimal equivalent:** For gx ∈ [1, 10), split into bins (e.g. 16 or 64). For each bin store (k0, k1). Define **eps** as the decimal “position within the bin” (e.g. `eps = (gx - x_lo) / step` in fixed scale, or derived from leading digits). Then:

```
r0 = k0 - k1 * eps
```

(scale and representation chosen to match decimal significand and exponent.) Optionally refine r0 → r (e.g. sigma0-style step) before using r in the remainder correction.

### Flow aligned: SoftFloat (2^e) vs Decimal (10^e)

| Step | SoftFloat (binary, 2^e) | Decimal (10^e) |
|------|--------------------------|----------------|
| **Normalize** | sig_a in [2^23, 2^24) or similar; exp_a odd/even | gx = sig × 10^−k ∈ [1, 10); e_odd = e mod 2 |
| **Index** | index = (sig_a >> 27 & 0xE) + exp_a_odd | index from gx (e.g. (gx−1)/step or leading digits) + e_odd |
| **eps** | eps = (sig_a >> 12) (position in bin) | eps = position of gx within bin (e.g. (gx − x_lo)/step) |
| **r0** | r0 = k0[index] − (k1[index] × eps) >> 20 | r0 = k0[index] − k1[index] × eps (decimal scale) |
| **r** | Optional: refine r0 → r (sigma0, etc.) | Optional: refine r0 → r |
| **Initial z** | z = sig_a × r (≈ √sig_a) | z = gx × r (≈ √gx) |
| **Remainder** | rem = sig_a − z² (exact integer) | rem = gx − z² (exact in decimal) |
| **Correction** | q = (rem >> 2) × r; z = z + q | q = rem × r / 2; z = z + q |
| **Rescale** | result = z × 2^(exp/2); ×√2 if exp odd | result = z × 10^(e/2); ×√10 if e odd |

So: **same steps; only 2 vs 10 and bit shift vs power-of-10 scaling.**

Mathematically, the correction is the Newton variant:

$$\sqrt{x} \approx z + \frac{x - z^2}{2\sqrt{x}} = z + \frac{\text{rem}}{2} \cdot \frac{1}{\sqrt{x}}$$

**Remainder elimination** = exact remainder × approximate 1/√x → correction to z.

### Initial value: z = gx × recip_sqrt, do NOT interpolate sqrt

**Correct (SoftFloat-style):**  
Initial **z = gx × recip_sqrt**, where **recip_sqrt** comes from the **(k0, k1)** scheme: r0 = k0 − k1·eps (and optional refinement to r). The same r is used for the correction term `rem × r / 2`.

**Incorrect (causes systematic bias):**  
Do **not** take z from a **sqrt table with linear interpolation** (e.g. z = z0 + (z1−z0)*frac). On [1, 10), √x is **concave**, so that chord lies above the curve and **overestimates** sqrt; remainder steps only partly correct, leaving a positive bias.

| Initial value source | Bias |
|----------------------|------|
| **z = gx × r** with **r = k0 − k1·eps** (two-table, per bin) | No inherent bias; matches SoftFloat. |
| **z = linear interpolation of sqrt table** | Systematic overestimate (concave function). |

### Why decimal can do the same

**Key insight: In BID format, the decimal significand IS an integer (uint32/64/128).** So remainder and scaling are exact; the only difference from SoftFloat is radix and scaling:

| Operation | Binary (2^e) | Decimal (10^e) |
|-----------|--------------|----------------|
| **sig** | Binary integer | Decimal integer (stored as uint32/64/128) |
| **rem = x − z²** | Integer subtraction, exact | Integer subtraction, exact (align powers of 10) |
| **Index / eps** | Bit extraction (a>>27, a>>12) | From gx (e.g. (gx−1)/step, (gx−x_lo)/step) |
| **Scaling** | `>> n`, `<< n` | `× 10^n`, `/ 10^n` |
| **q** | (rem>>2) × r | rem × r / 2 (or rem × r / (2×10^scale)) |

### Practical differences (only radix / scaling)

| | SoftFloat (binary, 2^e) | Decimal (10^e) |
|---|-------------------------|----------------|
| **Tables** | Two: k0[16], k1[16] (per bin) | Two: k0[n], k1[n] for [1, 10), n = 16–64 |
| **Index** | (sig_a>>27 & 0xE) + exp_odd | From gx (e.g. (gx−1)/step) + e_odd |
| **eps** | sig_a>>12 (position in bin) | (gx − x_lo)/step or equivalent |
| **r0** | k0 − (k1×eps)>>20 | k0 − k1×eps (decimal scale) |
| **Scaling** | Bit shift >>, << | ×10^n, /10^n |
| **Exponent parity** | exp_odd → sqrt(2·sig) vs sqrt(sig) | e_odd → ×√10 vs ×1 |

### Conclusion

1. **Same algorithm, radix difference only**: SoftFloat (2^e) and decimal (10^e) use the same flow; only normalization, index/eps, and scaling use 2 vs 10.
2. **Two-table scheme**: Each bin stores **(k0, k1)**. **r0 = k0 − k1·eps** (eps = position within bin). Optionally refine r0 → r; then **z = gx × r**, **q = rem × r / 2**, **z = z + q**.
3. **Correct implementation steps**:
   - Build **two** decimal-range tables: **k0** and **k1** for [1, 10) (e.g. 16 or 64 bins).
   - **Index** from gx (and e_odd); **eps** = position of gx within the bin.
   - **r0 = k0[index] − k1[index] × eps**; optionally refine to r.
   - **Initial value: z = gx × r** (same as SoftFloat’s sig × recip_sqrt). Do **not** use a sqrt table with linear interpolation for z.
   - **Remainder**: rem = gx − z²; **correction**: q = rem × r / 2, z = z + q.
   - Handle exponent parity (×√10 when e odd) and rescale (×10^(e/2)).

**Bottom line**: Decimal can match SoftFloat’s sqrt flow exactly: **(k0, k1)** per bin, **r = k0 − k1·eps**, **z = gx × r**, remainder correction. Only 2^e vs 10^e and bit shift vs power-of-10 scaling differ.

### Implementation note

`sqrt.hpp` now follows this doc and SoftFloat naming:

- **Tables**: `approxRecipSqrt_1k0s` and `approxRecipSqrt_1k1s` (same semantics as SoftFloat’s `softfloat_approxRecipSqrt_1k0s` / `_1k1s`).
- **Lookup**: One function **`approx_recip_sqrt_1<T>(index, eps)`** wraps the table lookup and returns **r0 = k0 − k1·eps**; `sqrt_impl` uses **z = gx × r** with this r.
- **Types**: SoftFloat uses the same k0/k1 tables for f16 and f32; decimal uses one set of tables for all decimal types. Per-type tables can be added later if needed (same or different, follow SoftFloat).
- **Remainder handling**: Same 2^e → 10^e mapping as in the doc: `rem = gx − z²`, `q = rem×r/2`, `z = z + q` (SoftFloat: `rem = sig_a − z²`, `q = (rem>>2)×r` fixed-point, `z += q`). Comments in `sqrt.hpp` spell out this mapping.

---

*References: IEEE 754-2019, Boost.Decimal `conversions.adoc`, `decimal32_t.hpp` comments, SoftFloat library by John R. Hauser.*
