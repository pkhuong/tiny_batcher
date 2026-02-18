# Test Coverage Analysis

## Current Coverage

The test suite has strong coverage in a few critical areas:

1. **Zero-one exhaustive correctness (n=0..21)** — `test.cpp:18-54`. By the zero-one
   principle for sorting networks, if a network correctly sorts all 2^n binary sequences,
   it sorts all inputs. This is the strongest test in the suite.

2. **Reference comparison (n=0..1200 + powers of 2 up to 2^62)** — `test.sh:10-21`.
   Validates that the generated compare-exchange index pairs match the Python reference
   transliteration of Algorithm M.

3. **Bound checks near SIZE_MAX** — `test.cpp:57-76`. Verifies `0 <= left < right < n`
   for values near SIZE_MAX/2 and SIZE_MAX/4.

4. **Post-exhaustion sentinel** — `test.cpp:189-194`. Confirms the generator keeps
   returning `(0, 0)` after completion.

5. **C++ API surface** — reverse comparator (`test.cpp:79`), string sorting (`test.cpp:93`),
   nested `TINY_BATCHER_SORT_LOOP` macro (`test.cpp:105`).

## Gaps and Proposed Improvements

### High Priority

#### 1. Randomized sorting test for sizes beyond n=21

The zero-one test is exhaustive but capped at n=21 (2^21 permutations). For larger sizes,
only the compare-exchange *indices* are verified against the reference — actual sorting
correctness is never checked. A randomized test that sorts, say, 1000 random permutations
at sizes like 50, 100, 500, 1000 would catch bugs that only manifest at larger sizes (e.g.,
if the index pairs are correct but the C++ sorting templates misapply them).

#### 2. Element preservation check on non-binary inputs

The zero-one test checks that the count of 1s is preserved, but there is no general test
confirming that sorting preserves all elements (no duplications, no losses) for arbitrary
integer arrays. A simple approach: sort a copy with `std::sort`, sort the original via
`tiny_batcher_sort`, and compare.

#### 3. Container + comparator overload is untested

`tiny_batcher_sort(T &container, Comp comparator)` at `tiny_batcher.hpp:50` is never called.
The `reverse_comparator_test` uses the iterator+size+comparator overload instead. This means
the container+comparator template is entirely untested.

### Medium Priority

#### 4. Pure C compilation test

All tests are in C++. The C API (`tiny_batcher_make`, `tiny_batcher_generate`,
`TINY_BATCHER_SORT_LOOP`, `tiny_batcher_next`) is never tested from a C compilation unit.
A C compiler may handle the inline functions and macro expansion differently.

#### 5. Move-only or non-trivial-copy types

The no-comparator path in `tiny_batcher.hpp:25-41` uses `std::minmax`, which returns
`pair<const T&, const T&>`. This works for copyable types but would fail for move-only
types. There is no test confirming this works with types that have expensive or deleted copy
constructors, or that the comparator-based path (which uses `std::swap`) works with
move-only types.

#### 6. Multiple TINY_BATCHER_SORT_LOOP at the same scope level

The nested test uses loops in *different* scopes. There is no test with two sequential
`TINY_BATCHER_SORT_LOOP` invocations at the *same* scope level, which would verify that
`__COUNTER__`-based variable naming avoids collisions.

#### 7. All-equal elements

An array of identical values is a degenerate case. While the zero-one test covers all-zeros
and all-ones at small sizes, there is no explicit test that, say, 100 copies of the same
value remain unchanged after sorting.

### Low Priority

#### 8. Already-sorted and reverse-sorted inputs

Common edge cases for sorting. While covered implicitly for small n by the zero-one test,
explicitly testing at larger sizes (e.g., n=1000) through the actual sorting API would
increase confidence.

#### 9. Generator reuse / re-initialization

There is no test that creates a `tiny_batcher` for one size, exhausts it, then creates a
new one for a different size and verifies independent operation. This would catch any hidden
global state or struct reuse issues.

#### 10. Direct tiny_batcher_next test

`tiny_batcher_next` (`tiny_batcher.h:73-80`) is only exercised through the
`TINY_BATCHER_SORT_LOOP` macro. A direct test calling `tiny_batcher_next` in a manual loop
and checking both the return value and the output parameters would improve coverage of this
inline function independently.

#### 11. Custom swap via the compare-exchange loop

The library's core design is that callers provide their own comparison and swapping logic.
There is no test where a custom swap operation (e.g., one that tracks swap count or does
conditional swapping based on domain logic) is used with the raw `TINY_BATCHER_SORT_LOOP`.

#### 12. n=SIZE_MAX/2 sentinel behavior

The `bound_check` tests only iterate 1000 steps for large n values. There is no test
verifying that the generator *eventually terminates* (returns sentinel) for these boundary
sizes — only that the first 1000 steps produce valid indices.
