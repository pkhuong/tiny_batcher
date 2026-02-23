#include "tiny_batcher.h"

#include <limits.h>

__attribute__((__noinline__)) struct tiny_batcher_step
tiny_batcher_generate(struct tiny_batcher *state)
{
    size_t len = -state->len;
    if (__builtin_expect(state->len <= SIZE_MAX / 2, 0))
    {
        len = state->len;

        if (__builtin_expect(len <= 1, 0))
            goto done;

        char clzll_must_not_truncate_size_t[2 * (sizeof(long long) >= sizeof(size_t)) - 1];
        (void)clzll_must_not_truncate_size_t;

        char bitscan_must_fit_in_uint8_t[2 * (CHAR_BIT * sizeof(long long) < UINT8_MAX) - 1];
        (void)bitscan_must_fit_in_uint8_t;

        state->len = -len;

        // clzll(len - 1) is safe because len > 1.
        state->c.v.ilen = CHAR_BIT * sizeof(long long) - 1 - __builtin_clzll(len - 1);
        state->c.v.outer = state->c.v.inner = state->c.v.ilen;
        state->next_idx = 0;
        __asm__(" # opaque " : "+m"(*state));
    }

    while (true)
    {
#if defined(__i386__) || defined(__x86_64__)
        // Try to help with register pressure: we have memory operands on x86
        __asm__(" # force to mem " : "+m"(state->c.v.ilen));
#endif

        size_t p = (size_t)1 << state->c.v.outer;
        size_t q = (size_t)1 << state->c.v.inner;
        bool is_first_inner = state->c.v.inner == state->c.v.ilen;

        size_t d = 2 * q - p;
        size_t idx = state->next_idx;
        size_t increment = (~idx) & p;  // ensure the outer bit is set

        if (__builtin_expect(is_first_inner, 0))  // special-case the first inner loop (the whole loop)
        {
            d = p;
            increment ^= p;  // ensure the outer bit is not set.
        }

        idx += increment;
        if (__builtin_expect(idx + d < len, 1))
        {
            struct tiny_batcher_step ret;
            ret.left = idx;
            ret.right = idx + d;

            state->next_idx = idx + 1;
            return ret;
        }

        // Done with the inner iteration.

        // v.outer == v.inner == 0 <===> p == q == 1, we're done.
        //
        // N.B., we haven't updated `state` yet, so the next call
        // will do the same thing.
        if (__builtin_expect((state->c.v.outer | state->c.v.inner) == 0, 0))
            goto done;

        state->next_idx = 0;
        // bool is_last_inner = state->c.v.outer == state->c.v.inner;
        // and inner is monotonically decreasing.

        state->c.v.inner--;

        bool is_last_inner = state->c.v.outer > state->c.v.inner;
        if (__builtin_expect(is_last_inner, 0))
        {
#if defined(__i386__) || defined(__x86_64__)
            // Force memory operand on x86
            __asm__(" # force to mem " : "+m"(state->c.v.ilen));
#endif

            state->c.v.inner = state->c.v.ilen;
            state->c.v.outer--;
        }
    }

done:;
    struct tiny_batcher_step ret;
    ret.left = ret.right = 0;
    return ret;
}
