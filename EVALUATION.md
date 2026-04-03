# Evaluation of tiny_batcher for High-Reliability C Projects

## What it is

A compact (~200 bytes of generated code) generator for Batcher's merge-exchange
sorting network (TAoCP Vol 3, Algorithm M). It yields `(left, right)` index
pairs; the caller performs compare-and-swap. Zero dynamic allocation, no
external dependencies beyond `<stdint.h>`, `<stddef.h>`, `<stdbool.h>`.

## Overall verdict: Reasonably suitable, with caveats

This is a well-crafted, minimal library with unusually strong correctness
evidence for its size. However, several gaps remain before it meets a "high
reliability" bar.

---

## Strengths

1. **Formal verification with Frama-C/WP.** The core function
   `tiny_batcher_generate` has ACSL contracts covering termination (proved via
   `state_measure` variant), output bounds (`left < right < len`), progress
   (each call strictly decreases the measure or returns done), and counter
   invariants / arithmetic overflow safety.

2. **No dynamic allocation.** The entire state fits in a ~40-byte stack struct.
   No `malloc`, no `free`, no leak paths.

3. **Thorough testing.** Exhaustive zero-one principle test for n <= 21 (2^n
   cases each), comparison against a Python reference implementation for n up to
   2^62, and bounds-checking for `left < right < n`.

4. **Tiny attack surface.** One exported function, ~177 lines of C, no I/O, no
   system calls.

5. **Data-oblivious.** The index sequence is independent of data values, which
   is good for constant-time / side-channel-resistant code.

---

## Concerns

### 1. Four `admit` annotations -- unproven assumptions in the formal proof

At `tiny_batcher.c:73,101,105,118`, the Frama-C proof uses `/*@ admit ... */` --
these are axioms the prover accepts without proof. They assert bounds on `ilen`,
`p`, `q`, `d`, and `increment`. The accompanying comments (lines 122-130)
provide convincing manual reasoning, but the proof chain has gaps. A bug in one
of these admitted bounds would invalidate the termination and overflow-safety
proofs.

**Risk: Medium.** The manual arguments are sound, but the whole point of formal
verification is to not rely on manual arguments.

### 2. Deliberately uninitialized struct fields

`tiny_batcher_make()` only sets `ret.len` and leaves the counters and `next_idx`
uninitialized (`tiny_batcher.h:70`). This is safe because `tiny_batcher_generate`
detects the uninitialized state via the sign of `len` and initializes before use.
However:

- Tools like Valgrind, MSan, or UBSAN may flag this as uninitialized memory use.
- The struct may have trailing padding that is uninitialized, which matters if
  you ever `memcmp` or hash the struct.

**Risk: Low** for correctness, **Medium** for integration with sanitizers/tooling.

### 3. No input validation without `TINY_BATCHER_ASSERT`

If `len > SIZE_MAX / 2` is passed to `tiny_batcher_make` and assertions are
disabled (the default), the code sets `state->len = len`, and
`tiny_batcher_generate` interprets it as already-initialized (since `len >
SIZE_MAX / 2`). This reads the uninitialized counter fields, causing **undefined
behavior**.

**Risk: Medium.** Easy to misuse in production where `assert` is typically
compiled out.

### 4. GCC/Clang-specific builtins required

The code uses `__builtin_clzll`, `__builtin_expect`, `__builtin_trap`,
`__attribute__((always_inline, noinline))`, and inline `__asm__`. Not portable
to MSVC or non-GCC-family compilers.

**Risk: Low** if your toolchain is GCC/Clang. **High** otherwise.

### 5. Infinite loop risk -- confirmed safe

The `while (true)` loop at line 92 has a formally verified variant
(`state_measure`) that strictly decreases on each iteration. The only scenario
where this could loop indefinitely is if the admitted bounds (concern #1) are
wrong.

### 6. Arithmetic undefined behavior -- confirmed safe (modulo admits)

The comments at lines 122-130 provide a rigorous manual proof that `idx + d <=
SIZE_MAX`, `2 * q - p` cannot underflow, and shift amounts are <= 62. All of
this holds assuming the admitted bounds are correct.

### 7. Thread safety

The generator is stateful but stack-allocated. No shared mutable state exists.
No thread-safety issue as long as you don't share a `struct tiny_batcher` across
threads, which would be an unusual usage pattern.

---

## Recommendations to Improve Confidence

1. **Close the `admit` gaps in the Frama-C proof.** The four admitted properties
   are the single biggest weakness. The manual arguments suggest the provers may
   just need hints or lemma decomposition. Closing these would give a fully
   machine-checked proof of no UB and termination.

2. **Add unconditional input validation.** Replace the `#ifdef assert` guard
   with an unconditional check (e.g., `__builtin_trap()` or returning a
   zero-length batcher). Relying on `assert` in safety-critical code is a known
   anti-pattern.

3. **Run under sanitizers.** Compile with `-fsanitize=undefined,address,memory`
   and run the test suite. The intentionally uninitialized fields will likely
   trigger MSan; zero-initializing the struct (at minor code-size cost) would
   resolve this.

4. **Add a CI pipeline.** No CI configuration exists. At minimum:
   - `test.sh` on every commit
   - Sanitizer builds (ASAN, UBSAN, MSan)
   - Frama-C verification (`wp.sh` exists but isn't automated)

5. **Fuzz the generator.** Use AFL or libFuzzer to exercise
   `tiny_batcher_generate` with random `len` values and call sequences. This
   complements formal verification by testing the actual compiled binary.

6. **Vendor and pin the code.** At ~300 total lines, this is small enough to
   vendor directly rather than taking as an external dependency. This eliminates
   supply-chain risk and lets you apply hardening yourself.

7. **Zero-initialize the struct** in `tiny_batcher_make` if code size is not
   your primary constraint:
   ```c
   struct tiny_batcher ret = {0};
   ret.len = len;
   ```

---

## Summary

| Aspect                       | Rating                                                          |
|------------------------------|-----------------------------------------------------------------|
| Likelihood of UB             | Low -- formal proof covers most paths, 4 unproven lemmas remain |
| Likelihood of infinite loops | Very low -- termination formally proved (modulo admits)         |
| Code quality                 | High -- minimal, well-commented, formally annotated             |
| Test coverage                | Good -- exhaustive for small n, reference-checked for large n   |
| Production readiness         | Medium -- needs hardened input validation, CI, sanitizer runs   |
| Portability                  | GCC/Clang only                                                  |
