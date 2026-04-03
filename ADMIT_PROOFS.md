# Proof Audit of `admit` Annotations in tiny_batcher.c

Each `admit` in Frama-C ACSL is an unproven assumption the prover accepts on
faith.  This document provides a manual proof or counterexample for every
`admit` in the codebase.

**Verdict: 3 of 4 admits are correct.  Admit 3 is FALSE, making the formal
proof unsound — though the runtime code is still correct.**

---

## Shared context

Throughout `tiny_batcher_generate`, the following loop invariants hold at the
top of the `while (true)` loop (lines 80–91):

- `state->len > SIZE_MAX / 2`  (state is initialized)
- `state->c.v.ilen + 1 < CHAR_BIT * sizeof(size_t)`  (on 64-bit: `ilen <= 62`)
- `state->c.v.outer <= state->c.v.inner <= state->c.v.ilen`
- `state->next_idx + state->len <= SIZE_MAX`

Let `B_ll = CHAR_BIT * sizeof(long long)` and `B_st = CHAR_BIT * sizeof(size_t)`.
The compile-time check at line 61 ensures `B_ll >= B_st`.  On all common 64-bit
platforms, `B_st = 64`.

---

## Admit 1 — `ilen_init_bound` (line 73–74): CORRECT

```c
state->c.v.ilen = CHAR_BIT * sizeof(long long) - 1 - __builtin_clzll(len - 1);
/*@ admit ilen_init_bound:
  @   state->c.v.ilen + 1 < CHAR_BIT * sizeof(size_t); */
```

### Proof

At this program point:
- `len > 1`  (line 58 guard)
- `len <= SIZE_MAX / 2`  (line 54 guard)
- `sizeof(long long) >= sizeof(size_t)`  (line 61 compile-time check)

Since `SIZE_MAX = 2^B_st - 1`, integer division gives
`SIZE_MAX / 2 = 2^(B_st - 1) - 1`.  So `len <= 2^(B_st-1) - 1`, hence
`len - 1 <= 2^(B_st-1) - 2 < 2^(B_st-1)`.

The highest set bit in `len - 1` is at most position `B_st - 2` (zero-indexed).
Represented as a `B_ll`-bit unsigned long long, there are at least
`B_ll - (B_st - 1)` leading zeros:

    clzll(len - 1) >= B_ll - B_st + 1

Therefore:

    ilen = B_ll - 1 - clzll(len - 1)
         <= B_ll - 1 - (B_ll - B_st + 1)
          = B_st - 2

So `ilen + 1 <= B_st - 1 < B_st = CHAR_BIT * sizeof(size_t)`.  **QED.**

---

## Admit 2 — `1 <= p <= q <= (1 << 62)` (line 101): CORRECT

```c
size_t p = (size_t)1 << state->c.v.outer;
size_t q = (size_t)1 << state->c.v.inner;
/*@ admit 1 ≤ p ≤ q ≤ (1 << 62); */
```

### Proof

From the loop invariants:
- `outer` and `inner` are `uint8_t`, so `outer >= 0` and `inner >= 0`.
- `outer <= inner <= ilen <= 62`  (from `counter_order` and `ilen_bound`)

Therefore:

- `p = 2^outer >= 2^0 = 1`
- `q = 2^inner >= 2^outer = p`  (since `inner >= outer`)
- `q = 2^inner <= 2^62`  (since `inner <= 62`)

All shifts are well-defined: shifting `(size_t)1` by at most 62 in a type that
is at least 64 bits wide.  **QED.**

---

## Admit 3 — `0 < d < (1 << 62)` (line 105): FALSE

```c
size_t d = 2 * q - p;
/*@ admit 0 < d < (1 << 62); */
bool is_first_inner = state->c.v.inner == state->c.v.ilen;
```

### Counterexample

When `is_first_inner` is true (which happens routinely — on the first iteration
after initialization and after every outer-loop step), take:

    ilen = 62,  inner = ilen = 62,  outer = 0

Then:
- `q = 2^62`
- `p = 2^0 = 1`
- `d = 2 * 2^62 - 1 = 2^63 - 1`

Since `2^63 - 1 >> 2^62`, the admitted property `d < 2^62` is **false**.

### Why the code is still correct

On the `is_first_inner` path, `d` is immediately overwritten to `p` at line 111
before any use.  After the conditional block (line 113), `d` satisfies:

- **Non-first-inner** (`inner < ilen`, so `inner <= 61`):
  `d = 2*q - p`, where `q = 2^inner <= 2^61`.
  Upper bound: `d <= 2*2^61 - 1 = 2^62 - 1 < 2^62`.
  Lower bound: `d >= 2*p - p = p >= 1 > 0`.

- **First-inner** (d overwritten): `d = p`, where `1 <= p <= 2^62`.

In both cases `0 < d <= 2^62`, so the assertion at line 119 (`d <= (1 << 62)`)
holds and the subsequent arithmetic (`idx + d`) cannot overflow.  The runtime
behavior is safe.

### Impact on the formal proof

A false `admit` makes the Frama-C proof **formally unsound**: from a false
premise, the prover can derive any conclusion (ex falso quodlibet).  Even though
the stale value of `d` is never read on the first-inner path, the false fact
remains in the proof context and could be used to discharge unrelated proof
obligations.

### Suggested fix

Move the annotation below the conditional, where it is true in both paths:

```c
if (__builtin_expect(is_first_inner, 0))
{
    d = p;
    increment ^= p;
}
/*@ assert 0 < d ≤ (1 << 62); */
```

Or split into two path-specific annotations before the conditional.

---

## Admit 4 — `increment <= (1 << 62)` (line 118): CORRECT

```c
size_t increment = (~idx) & p;  // ensure the outer bit is set
if (__builtin_expect(is_first_inner, 0))
{
    d = p;
    increment ^= p;  // ensure the outer bit is not set.
}
/*@ admit increment ≤ (1 << 62); */
```

### Proof

We know `p = 2^outer` where `0 <= outer <= 62`, so `p` is a power of two with
`p <= 2^62`.

**Non-first-inner path:**
`increment = (~idx) & p`.  Bitwise AND with a single-bit mask `p` yields either
`0` or `p`.  So `increment <= p <= 2^62`.

**First-inner path:**
`increment = ((~idx) & p) ^ p`.  XOR with the single-bit value `p` flips that
bit: if `(~idx) & p` was `0`, the result is `p`; if it was `p`, the result is
`0`.  So `increment` is either `0` or `p`, hence `increment <= p <= 2^62`.

In all cases `increment <= 2^62`.  **QED.**

---

## Summary

| Admit | Line | Verdict | Notes |
|-------|------|---------|-------|
| 1. `ilen_init_bound` | 73 | **Correct** | Follows from `len <= SIZE_MAX/2` and compile-time size checks |
| 2. `1 <= p <= q <= 2^62` | 101 | **Correct** | Direct consequence of the loop invariants |
| 3. `0 < d < 2^62` | 105 | **FALSE** | Counterexample: ilen=62, outer=0, is_first_inner=true gives d=2^63-1. Code is safe because d is overwritten before use; proof is unsound. |
| 4. `increment <= 2^62` | 118 | **Correct** | Bitwise ops with single-bit p can only produce 0 or p |
