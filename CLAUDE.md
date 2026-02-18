# CLAUDE.md

## Project Overview

`tiny_batcher` is a BSD-licensed, space-optimized sorting library implementing Batcher's odd-even merge sort (Algorithm M from TAoCP Vol 3). It generates comparison/exchange index pairs for a sorting network, letting callers supply their own comparison and swapping logic. The design prioritizes small code footprint and guaranteed O(n log² n) worst-case runtime over raw speed. The sorting network is data-oblivious, making it suitable for constant-time code.

## Repository Structure

Flat layout, no subdirectories:

```
tiny_batcher.c      # Core generator function (tiny_batcher_generate), dependency-free
tiny_batcher.h      # C header: state struct, inline helpers, TINY_BATCHER_SORT_LOOP macro
tiny_batcher.hpp    # C++ template wrappers for convenient sorting
test.cpp            # C++ test suite (zero-one exhaustive, bound checks, reference comparison)
test.sh             # Test runner script
reference.py        # Python reference implementation of Algorithm M for verification
.clang-format       # Code formatting rules
```

## Build and Test

There is no build system (no Makefile, CMake, etc.). The library is header-only with one optional `.c` file.

### Compile

```bash
cc -Os -c tiny_batcher.c
c++ -Os test.cpp tiny_batcher.o -o test
```

### Run Tests

```bash
./test.sh
```

This runs:
1. Exhaustive zero-one testing for all array sizes up to n=21 (all 2^n binary permutations)
2. Reference comparison against `reference.py` for n=0..1200 and powers of 2 up to 2^60
3. Boundary checks near SIZE_MAX values

Tests pass silently on success. Failures print `Failed <n>` or abort via `__builtin_trap()`.

## Code Conventions

### Formatting
- **Formatter**: clang-format (config in `.clang-format`)
- **Style**: WebKit-based with Allman braces
- **Indentation**: 4 spaces (no tabs)
- **Column limit**: 110 characters
- **Pointer alignment**: Right-aligned (`char *p`, not `char* p`)
- `TINY_BATCHER_SORT_LOOP` is registered as a ForEachMacro (no parens space before it)

### Naming
- `snake_case` for functions and variables: `tiny_batcher_generate`, `next_idx`
- `UPPER_CASE` for macros: `TINY_BATCHER_SORT_LOOP`
- Single-letter variables in algorithmic loops follow TAoCP notation: `p`, `q`, `r`, `d`
- Prefix `tiny_batcher_` for all public symbols

### Language Details
- **C code**: C99+, uses `<stdbool.h>`, `<stdint.h>`, `<stddef.h>`, `<limits.h>`
- **C++ code**: C++17 features (`[[gnu::always_inline]]`, `auto`, structured bindings)
- **Compiler extensions**: `__builtin_clzll()`, `__builtin_expect()`, `__builtin_trap()`, `__attribute__((noinline))`, inline `__asm__` comments for branch/predication hints
- **Types**: `size_t` for indices and lengths, `uint8_t` for log₂-space counters
- **Error handling**: No error codes or exceptions; uses preconditions (`len <= SIZE_MAX / 2`), sentinel return values (`left == right == 0` signals completion), and `__builtin_trap()` for test assertions

### Architecture
- Single non-inline function (`tiny_batcher_generate` in `tiny_batcher.c`) keeps the code footprint minimal
- Everything else is `static inline __attribute__((__always_inline__))` for zero-overhead wrappers
- State struct uses negative `len` to distinguish initialized vs uninitialized state
- The `TINY_BATCHER_SORT_LOOP` macro uses `__COUNTER__` to generate unique state variable names

## Key Preconditions

- `len <= SIZE_MAX / 2` (the generator uses sign bit of `size_t` internally)
- Generator returns `left == right == 0` as the sentinel for "sort complete"
- During iteration, `0 <= left < right < len` is guaranteed

## Dependencies

None. `tiny_batcher.c` only includes `<limits.h>` (via `tiny_batcher.h`). Test code uses the C++ standard library and Python 3.
