#include "tiny_batcher.hpp"

#include <algorithm>
#include <climits>
#include <stdio.h>
#include <stdlib.h>
#include <vector>

__attribute__((noinline)) void
int_sort(int *xs, size_t n)
{
    tiny_batcher_sort(xs, n);
}

static void
zero_one_test(size_t n)
{
    std::vector<int> xs;

    if (n > 30)
        __builtin_trap();

    xs.resize(n);
    for (size_t bits = 0; bits < (1UL << n); bits++)
    {
        size_t ones = 0;

        for (size_t idx = 0; idx < n; idx++)
        {
            int value = (bits & (1UL << idx)) != 0;
            ones += value;
            xs[idx] = value;
        }

        int_sort(xs.data(), n);
        if (!std::is_sorted(xs.begin(), xs.end()))
            __builtin_trap();

        size_t final_ones = 0;
        for (int x : xs)
        {
            if (x != 0 && x != 1)
                __builtin_trap();
            final_ones += x;
        }

        if (final_ones != ones)
            __builtin_trap();
    }

    fprintf(stderr, "Completed zero-one test for %zu\n", n);
}

static void
bound_check(size_t n, size_t iter)
{
    size_t i = 0;

    size_t left, right;
    TINY_BATCHER_SORT_LOOP(n, left, right)
    {
        if (left >= right)
            __builtin_trap();

        if (right >= n)
            __builtin_trap();

        if (left >= n)
            __builtin_trap();

        if (i++ > iter)
            break;
    }
}

int
main(int argc, char **argv)
{
    bound_check(SIZE_MAX / 2, 1000);
    bound_check(SIZE_MAX / 2 - 1, 1000);
    bound_check(SIZE_MAX / 4 + 2, 1000);
    bound_check(SIZE_MAX / 4 + 1, 1000);
    bound_check(SIZE_MAX / 4, 1000);
    bound_check(SIZE_MAX / 4 - 1, 1000);

    // Only one argument, dump the list of compare-exchanges for
    // argv[1] items.
    if (argc == 2)
    {
        size_t n = atoll(argv[1]);

        if (n <= 21)
            zero_one_test(n);

        size_t i, j;
        TINY_BATCHER_SORT_LOOP(n, i, j)
            printf("%ld %ld\n", i, j);

        return 0;
    }

    // Otherwise, sort argv.
    std::vector<int> xs;

    for (int idx = 1; idx < argc; idx++)
        xs.push_back(atoi(argv[idx]));

    tiny_batcher_sort(xs);
    for (int x : xs)
        printf("%d\n", x);

    return std::is_sorted(xs.begin(), xs.end()) ? 0 : 1;
}
