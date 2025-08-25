`tiny_batcher`: a BSD-licensed space optimised sorting library
==============================================================

The `tiny_batcher` library implements a generator for the list of
conditionally swapped indices in Batcher's odd-even merge sort
(Algorithm M from TAoCP Vol 3).  The generator itself lives in the
dependency-free `tiny_batcher.c` file.  Convenience wrappers are
defined in `tiny_batcher.h` and `tiny_batcher.hpp`.

The use of a sorting network has nothing to do with vectorisation or
speed.  It simply makes for a small and easily testable interface.
The intended use case is sort calls that should compile quickly to a
small code footprint, without sacrificing worst-case asymptotics too
much; for example, for setup logic that runs rarely.  The generator
structure avoids duplicating comparison or swapping logic, and
Batcher's odd-even sort guarantees (n log^2 n) runtime, just a little
worse than the optimal O(n log n).  The library doesn't observe any
of the input values, so it can be used by constant-time code.

See `tiny_batcher.hpp` for sample usage, or simply

```
void sort_ints(int *xs, size_t n)
{
    size_t i, j;
    TINY_BATCHER_SORT_LOOP(n, i, j)
    {
        int left = xs[i], right = xs[j];

        xs[i] = left < right ? left : right;
        xs[j] = left >= right ? left : right;
    }
}
```

The `TINY_BATCHER_SORT_LOOP` macro expands to:

1. `struct tiny_batcher state = tiny_batcher_make(n);`
2. calls to `tiny_batcher_next(&state, &left, &right)`

And we simply check if we want to swap the values at left/right as
long as `tiny_batcher_next` returns true.

Testing
-------

Run `./test.sh` to:

1. exhaustively test all array sizes up to n=21 with 0/1 inputs
2. compare the list of comparison with the python reference implementation for
   n <= 1200, and powers of 2 up to 2**20.

Code footprint
--------------

There's only one function in `tiny_batcher`, `tiny_batcher_generate`.

When targeting x86-64, gcc 12.3.0 at `-Os` compiles `tiny_batcher_generate` to 223 bytes;
for aarch64, clang-20 at `-Os` compiles the same code to 240 bytes.

The rest of the library consists of trivial inline functions.  On
x86-64 / gcc-12.3.0, the `sort_ints` function above compiles to 67
bytes:

```
        pushq   %rbp
        pushq   %rbx
        movq    %rdi, %rbx
        subq    $40, %rsp
        movq    %rsi, 8(%rsp)
        leaq    8(%rsp), %rbp
.L2:
        movq    %rbp, %rdi
        call    tiny_batcher_generate@PLT
        testq   %rdx, %rdx
        je      .L6
        leaq    (%rbx,%rax,4), %rsi
        leaq    (%rbx,%rdx,4), %rcx
        movl    (%rsi), %eax
        movl    (%rcx), %edx
        cmpl    %edx, %eax
        movl    %edx, %edi
        cmovle  %eax, %edi
        cmovl   %edx, %eax
        movl    %edi, (%rsi)
        movl    %eax, (%rcx)
        jmp     .L2
.L6:
        addq    $40, %rsp
        popq    %rbx
        popq    %rbp
        ret
```

For aarch64 / clang-20 (at `-Os)`, `sort_ints` turns into 84 bytes:

```
        sub     sp, sp, #64
        stp     x29, x30, [sp, #32]             // 16-byte Folded Spill
        str     x19, [sp, #48]                  // 8-byte Folded Spill
        add     x29, sp, #32
        mov     x19, x0
        str     x1, [sp, #8]
.LBB0_1:                                // =>This Inner Loop Header: Depth=1
        add     x0, sp, #8
        bl      tiny_batcher_generate
        cbz     x1, .LBB0_3
// %bb.2:                               //   in Loop: Header=BB0_1 Depth=1
        ldr     w8, [x19, x0, lsl #2]
        ldr     w9, [x19, x1, lsl #2]
        cmp     w8, w9
        csel    w10, w8, w9, lt
        str     w10, [x19, x0, lsl #2]
        csel    w8, w8, w9, gt
        str     w8, [x19, x1, lsl #2]
        b       .LBB0_1
.LBB0_3:
        ldp     x29, x30, [sp, #32]             // 16-byte Folded Reload
        ldr     x19, [sp, #48]                  // 8-byte Folded Reload
        add     sp, sp, #64
        ret
```
