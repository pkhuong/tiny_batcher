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

	state->len = -len;
	state->ilen = CHAR_BIT * sizeof(size_t) - 1 - __builtin_clzll(len - 1);
	state->outer = state->inner = state->ilen;
	state->next_idx = 0;
    }

    while (true)
    {
	size_t p = 1UL << state->outer;
	size_t q = 1UL << state->inner;
	bool is_first_inner = state->inner == state->ilen;
	bool is_subsequent_inner = !is_first_inner;

	size_t d = is_first_inner ? p : 2 * q - p;
	__asm__(" # block branching " : "+r"(is_subsequent_inner));
	size_t r = p & -(size_t)is_subsequent_inner;  // = is_first_inner ? 0 : p;

	size_t idx = state->next_idx;

	idx += ((idx & p) != r) ? p : 0;
	__asm__(" # force predication " : "+r"(idx));

	if (__builtin_expect(idx + d < len, 1))
	{
	    struct tiny_batcher_step ret;
	    ret.left = idx;
	    ret.right = idx + d;

	    state->next_idx = idx + 1;
	    return ret;
	}

	// Done with the inner iteration.

	// p == q == 1, we're done.
	//
	// N.B., we haven't updated `state` yet, so the next call
	// will do the same thing.
	if (__builtin_expect((p | q) == 1, 0))
	    goto done;

	state->next_idx = 0;
	bool is_last_inner = p == q;
	state->inner = is_last_inner ? state->ilen : state->inner - 1;
	state->outer -= is_last_inner;
    }

done:
    {
	struct tiny_batcher_step ret;
	ret.left = ret.right = 0;
	return ret;
    }
}
