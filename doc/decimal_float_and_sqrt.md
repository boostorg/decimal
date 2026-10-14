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
1/10 = 0.0001 1001 1001 1001 1001 …₂  (1001 repeats)
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

- BID 32-bit: `bits_ = 0x33800001`
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

## 5. Sqrt Implementation

### 5.1 Core Algorithm

sqrt works on the integer significand. For x = s × 10^e with p digits in s, it picks k = p − 1 or k = p so that e − k is even. Then x = N × 10^(e−k) with N = s × 10^k, and

```
sqrt(x) = sqrt(N) × 10^((e − k) / 2)
```

N has 2p − 1 or 2p digits, so sqrt(N) has p digits. The result is sqrt(N) rounded once to an integer in the current rounding mode, with the exponent (e − k) / 2. All arithmetic is integer, and no step divides except by the constant 10.

1. A binary estimate of sqrt(N) that is within one unit (sections 5.2 and 5.3).
2. z = floor(sqrt(N)) from the estimate, with rem = N − z² ≥ 0.
3. The rounding: the upward mode adds 1 if rem ≠ 0, the nearest modes add 1 if rem > z, because (z + 1/2)² < N exactly when N − z² > z. A root is never a tie (`sqrt_steps_up` in `fenv_rounding.hpp`).

x = 10^(2m) returns 1 × 10^m. A root that rounds up to 10^p becomes 10^(p−1), with one more in the exponent.

### 5.2 Root and Reciprocal Square Root

`sqrt_estimate(a, steps)` gives y = sqrt(a / 2^62) × 2^61 and t = 2^63 / sqrt(a / 2^62) for a in [2^62, 2^64):

- A table of 49 values 2^31 / sqrt(1 + i/16) and a chord between two points, less a parabola from a second table of 48 values, give t with an error below 2^-18, as for cbrt. Then y = a × t.
- Each Goldschmidt step y += y × e / 2 and t += t × e / 2 with e = 1 − y × t squares the error of both. The step has two products in a chain, because the two updates are independent. The term e gets an offset of 2^-10, so it is never negative and the step needs no sign test.

Two steps give y and t within 2^-59.5 (a check of 10^9 random a gives 2^-59.7).

### 5.3 Estimate for Each Type

| Type | N | a | Steps | After the estimate |
|------|---|---|-------|--------------------|
| decimal32 | < 10^14, uint64 | N × 2^sh | 1 | none |
| decimal64 | < 10^32, uint128 | top 63 or 64 bits of N | 2 | none |
| decimal128 | < 10^68, u256 | top 63 or 64 bits of N | 2 | z += (N − z²) / (2 sqrt(N)) |

The shifts are even, so the root of a × 2^-62 gives the root of N with a shift. For decimal128, z = y × 2^h, so z² is the square of the 64-bit y with a shift. One more Newton step on z takes 128 bits of N − z² and multiplies them by t, because 1 / (2 sqrt(N)) = t × 2^-(95 + r/2) for N = a × 2^r. This gives z and 64 bits of fraction within R × 2^-120 + R × 2^-119 < 0.024 of the root R < 2^113. If the fraction is more than 2^-5 from 0 and from 1/2, it gives the floor and the rounding at once.

Else, and for the other types, z is within one unit of floor(sqrt(N)), and one correction step on rem = N − z² finds the floor. rem is small, so it is computed in the low word of N only, as in CORE-MATH `sqrtq.c`: 64 bits for decimal32 and decimal64, 128 bits for decimal128. A check of all decimal32 significands shows that its estimate is the floor or one below it.

---

## 6. File Structure

```
include/boost/decimal/detail/
├── cmath/sqrt.hpp       # table, sqrt_estimate, sqrt_root for each width, sqrt_impl
├── fenv_rounding.hpp    # sqrt_steps_up
└── u256.hpp             # umul128 (64×64→128), umul256 (128×128→256)
```

---

## 7. Performance (sqrt_bench.py)

| Type               | Baseline    | Before       | Current       | Speedup |
|--------------------|-------------|--------------|---------------|---------|
| decimal32_t        | 3.18M ops/s | 16.84M ops/s | 67.48M ops/s  | 21.2x   |
| decimal64_t        | 2.20M ops/s | 7.29M ops/s  | 47.68M ops/s  | 21.7x   |
| decimal128_t       | 0.85M ops/s | 1.73M ops/s  | 21.89M ops/s  | 25.8x   |
| decimal_fast32_t   | 2.62M ops/s | 16.18M ops/s | 114.25M ops/s | 43.6x   |
| decimal_fast64_t   | 1.39M ops/s | 6.08M ops/s  | 63.78M ops/s  | 45.9x   |
| decimal_fast128_t  | 0.84M ops/s | 1.74M ops/s  | 27.52M ops/s  | 32.8x   |

Baseline is `sqrt_baseline.hpp`, Before is the integer Newton design with divisions, and Current is this design. The speedup is Current against Baseline. The results of Before and Current are the same bits.

*Benchmark scripts `sqrt_bench.py` and `run_srqt_test.sh` are not in the main branch. To run: `git checkout d54af19 -- run_srqt_test.sh sqrt_bench.py test/benchmark_sqrt.cpp include/boost/decimal/detail/cmath/sqrt_baseline.hpp`.*

---

*References: IEEE 754-2019, CORE-MATH (https://core-math.gitlabpages.inria.fr), Boost.Decimal implementation.*
