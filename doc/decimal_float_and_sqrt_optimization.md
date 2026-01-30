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

### What SoftFloat does (essence)

The SoftFloat fixed-point sqrt algorithm follows this core flow:

```
1. Table lookup: recip_sqrt32 ≈ 1/sqrt(sig_a)
2. Initial value: sig32_z = sig_a × recip_sqrt32  (approx sqrt(sig_a))
3. Remainder: rem = sig_a − sig32_z²              (exact integer)
4. Correction: q = (rem >> 2) × recip_sqrt32      (remainder × 1/sqrt)
5. Result: sig_z = sig32_z + q
```

Mathematically, this is a Newton-Raphson variant:

$$\sqrt{x} \approx z + \frac{x - z^2}{2\sqrt{x}} = z + \frac{\text{rem}}{2} \cdot \frac{1}{\sqrt{x}}$$

**Remainder elimination** = use exact remainder × approximate 1/sqrt to estimate and correct the error.

### Why decimal can do the same

**Key insight: In BID format, the decimal significand IS an integer (uint32/64/128).**

| Operation | Binary/Fixed-point | Decimal |
|-----------|-------------------|---------|
| **sig** | Binary integer | Decimal integer (stored as uint32/64) |
| **Remainder rem = x − z²** | Integer subtraction, exact | Integer subtraction, exact (after aligning powers of 10) |
| **Table index** | `(a >> 27) & 0xE` (extract top bits) | `sig / 10^k` (extract leading decimal digits) |
| **Scaling** | `>> n` (bit shift) | `/ 10^n` or `× 10^n` |
| **Correction** | `q = (rem >> 2) × recip_sqrt` | `q = rem × recip_sqrt / (2 × 10^scale)` |

**Remainder is exact**:
```
rem = sig_x × 10^e_x − sig_z² × 10^(2·e_z)
```
Align both sides to the same power of 10, then it's integer subtraction — just like SoftFloat's `sig_a - sig32_z * sig32_z`.

**Correction term is computable**:
```
correction ≈ rem × recip_sqrt(x) / 2
```
`recip_sqrt(x)` comes from the decimal lookup table (or table + one Newton step); multiplication and division by 2 (or powers of 10) are all integer operations.

### Practical differences

| | SoftFloat (binary) | Decimal |
|---|-------------------|---------|
| **Table size** | 16 entries (4-bit index + parity) | 16–128 entries (decimal range, e.g. [1,10) split into 16 or 32 segments) |
| **Index method** | Bit extraction `(a >> 27) & 0xE` | Numeric calculation `sig / 10^(digits-1)` or `(sig - 10^k) / step` |
| **Scaling** | Cheap (bit shift) | Slightly more expensive (multiply/divide by powers of 10, but can precompute) |
| **Intermediate width** | uint32 → uint64 | decimal32 sig is uint32, squaring needs uint64; decimal128 needs uint256 |
| **Exponent parity** | `exp_a_odd` determines `ESqrR0 <<= 1` | Same: when e is odd, compute `sqrt(10·sig)`; when even, compute `sqrt(sig)` |

### Conclusion

1. **Mathematical principle is identical**: Remainder elimination = Newton variant = `correction = rem × (1/sqrt) / 2`.
2. **Decimal can do it**: Because sig is an integer, remainder is exact, 1/sqrt can be looked up, correction can be computed.
3. **Main work**:
   - Build a **decimal-range** 1/sqrt table (e.g. 128 entries for [1, 10) already exists);
   - Use the **same table** for both initial value and `rem × 1/sqrt` correction (just like SoftFloat uses `recip_sqrt32` twice);
   - Handle exponent parity and scaling (multiply/divide by powers of 10).

**Bottom line**: Decimal can achieve SoftFloat-style remainder elimination — just replace "bit-indexed table + bit shift" with "decimal-range-indexed table + multiply/divide by powers of 10".

---

*References: IEEE 754-2019, Boost.Decimal `conversions.adoc`, `decimal32_t.hpp` comments, SoftFloat library by John R. Hauser.*
