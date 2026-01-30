# Decimal vs Binary Float: Representation and Bit Layout

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

- **Combination field**: Encodes whether exponent uses 6 or 8 bits and significand 21 or 23 bits; semantics remain “decimal exponent + integer significand”.

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

*References: IEEE 754-2019, Boost.Decimal `conversions.adoc`, `decimal32_t.hpp` comments.*
