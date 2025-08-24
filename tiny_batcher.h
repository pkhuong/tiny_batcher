#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// A compact generator for exchange pairs that implement Batcher's
// odd/even merge sort.  This implementation is far from optimized
// for speed.  We only use a sorting network because the data-obliviousness
// makes for a nice easily testable interface.
struct tiny_batcher
{
    // Positive len when uninitialized (except for the length).
    // negative len once the state is initialized.
    //
    // 0 is unambiguous / uninteresting because always sorted.
    size_t len;

    // Remainder can be left uninitialised for `tiny_batcher_generate`.

    // State from TAoCP Vol 3's Algorithm M

    // in log_2 space
    union
    {
        struct
        {
            uint8_t ilen;   // t - 1
            uint8_t outer;  // p
            uint8_t inner;  // q
        } v;  // values
        size_t padding;
    } c; // counters

    size_t next_idx;
};

// One compare exchange step.  We can assume `0 <= left <= right < len`,
// and `left == right == 0` when the sort is done.
struct tiny_batcher_step
{
    size_t left;
    size_t right;
};

#ifdef __cplusplus
extern "C"
{
#endif

// Initialises a tiny batcher for a sort of `len` elements.
//
// Assumes `len <= SIZE_MAX / 2`.
static inline __attribute__((__always_inline__)) struct tiny_batcher
tiny_batcher_make(size_t len)
{
    struct tiny_batcher ret;
    ret.len = len;
    // Deliberately leave the rest uninitialised, for code size.
    return ret;
}

// Generates one step (another pair of indices to compare/exchange such that
// x[left] <= x[right])
//
// Returns an identity exchange (left == right) when the generator is empty.
struct tiny_batcher_step tiny_batcher_generate(struct tiny_batcher *state);

// Convenient for-loop wrapper around `tiny_batcher_generate`.
//
// Generates the next pair of indices from `state` into `left` and `right`,
// and returns whether there is still work to do.
static inline __attribute__((__always_inline__)) bool
tiny_batcher_next(struct tiny_batcher *state, size_t *left, size_t *right)
{
    struct tiny_batcher_step step = tiny_batcher_generate(state);
    *left = step.left;
    *right = step.right;

    return step.right != 0;
}

#ifdef __cplusplus
}  // extern "C"
#endif

#define TINY_BATCHER_SORT_LOOP(N, LEFT, RIGHT)     TINY_BATCHER_SORT_LOOP_(__COUNTER__, N, LEFT, RIGHT)

#define TINY_BATCHER_SORT_LOOP_(U, N, LEFT, RIGHT) TINY_BATCHER_SORT_LOOP__(U, N, LEFT, RIGHT)

#define TINY_BATCHER_SORT_LOOP__(U, N, LEFT, RIGHT)                                \
    for (struct tiny_batcher tiny_batcher_loop_state_##U = tiny_batcher_make((N)); \
        tiny_batcher_next(&tiny_batcher_loop_state_##U, &(LEFT), &(RIGHT));)
