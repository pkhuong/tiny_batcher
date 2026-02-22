#define TINY_BATCHER_ASSERT
#include "tiny_batcher.hpp"

#include <algorithm>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <random>
#include <string>
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
random_sort_test(size_t n)
{
    std::vector<int> xs;

    xs.reserve(n);

    {
        std::minstd_rand prng;

        prng.seed(12345);
        for (size_t i = 0; i < n; i++)
            xs.push_back(int(prng()));
    }

    std::vector<int> expected = xs;
    std::sort(expected.begin(), expected.end());

    tiny_batcher_sort(xs);

    if (xs != expected)
        __builtin_trap();
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

static void
reverse_comparator_test()
{
    std::vector<int> xs = { 3, 1, 4, 1, 5, 9, 2, 6, 5, 3, 5 };

    tiny_batcher_sort(xs, std::greater<int>());

    if (!std::is_sorted(xs.begin(), xs.end(), std::greater<int>()))
        __builtin_trap();
}

struct MoveOnlyString
{
    std::string value;

    MoveOnlyString(const char *s)
        : value(s)
    {
    }

    MoveOnlyString(MoveOnlyString &&) noexcept = default;
    MoveOnlyString &operator=(MoveOnlyString &&) noexcept = default;
    MoveOnlyString(const MoveOnlyString &) = delete;
    MoveOnlyString &operator=(const MoveOnlyString &) = delete;

    bool operator<(const MoveOnlyString &other) const { return value < other.value; }
};

static void
string_sort_test()
{
    std::vector<MoveOnlyString> xs;
    // shuf /usr/share/dict/words | head
    xs.emplace_back("popliteus");
    xs.emplace_back("jackpile");
    xs.emplace_back("Cordylanthus");
    xs.emplace_back("Bonne");
    xs.emplace_back("sloot");
    xs.emplace_back("mesoscapula");
    xs.emplace_back("associatedness");

    tiny_batcher_sort(xs);

    if (!std::is_sorted(xs.begin(), xs.end()))
        __builtin_trap();
}

static void
nested_sort_test()
{
    std::vector<std::vector<int>> vecs = {
        { 9, 3, 7, 1 },
        { 2, 8, 4, 6 },
        { 5, 1, 3, 9 },
        { 4, 6, 2, 8 },
        { 7, 1, 5, 3 },
    };

    size_t outer_left, outer_right;
    TINY_BATCHER_SORT_LOOP(vecs.size(), outer_left, outer_right)
    {
        auto &a = vecs[outer_left];
        auto &b = vecs[outer_right];

        if (!std::is_sorted(a.begin(), a.end()) || !std::is_sorted(b.begin(), b.end()))
        {
            size_t inner_left, inner_right;
            TINY_BATCHER_SORT_LOOP(a.size(), inner_left, inner_right)
            {
                if (a[inner_left] > a[inner_right])
                    std::swap(a[inner_left], a[inner_right]);
            }

            TINY_BATCHER_SORT_LOOP(b.size(), inner_left, inner_right)
            {
                if (b[inner_left] > b[inner_right])
                    std::swap(b[inner_left], b[inner_right]);
            }

            if (!std::is_sorted(a.begin(), a.end()))
                __builtin_trap();
            if (!std::is_sorted(b.begin(), b.end()))
                __builtin_trap();
        }

        if (a > b)
            std::swap(a, b);
    }

    if (!std::is_sorted(vecs.begin(), vecs.end()))
        __builtin_trap();
}

int
main(int argc, char **argv)
{
    // Stop after 1000 iterations, to minimally exercise the first
    // couple transitions in the generator state machine.
    bound_check(SIZE_MAX / 2, 1000);
    bound_check(SIZE_MAX / 2 - 1, 1000);
    bound_check(SIZE_MAX / 4 + 2, 1000);
    bound_check(SIZE_MAX / 4 + 1, 1000);
    bound_check(SIZE_MAX / 4, 1000);
    bound_check(SIZE_MAX / 4 - 1, 1000);

    reverse_comparator_test();
    string_sort_test();
    nested_sort_test();

    // Only one argument, dump the list of compare-exchanges for
    // argv[1] items.
    if (argc == 2)
    {
        size_t n = strtoull(argv[1], NULL, 10);

        if (n <= 21)
            zero_one_test(n);

        if (n < 1000000)
            random_sort_test(n);

        // Manually expand TINY_BATCHER_SORT_LOOP to keep our hands
        // on the batcher struct.
        struct tiny_batcher batcher = tiny_batcher_make(n);
        for (;;)
        {
            size_t i, j;

            if (!tiny_batcher_next(&batcher, &i, &j))
            {
                // stop when both are 0.
                if (i != 0 || j != 0)
                    __builtin_trap();

                break;
            }

            // should only have right == 0 when left and right are 0.
            if (j == 0)
                __builtin_abort();

            if (i >= n || j >= n || i >= j) // bound check
                __builtin_abort();

            printf("%zu %zu\n", i, j);
        }

        // Now we should be stuck at 0/0.
        for (size_t k = 0; k < 10; k++)
        {
            struct tiny_batcher_step step = tiny_batcher_generate(&batcher);
            if (step.left != 0 || step.right != 0)
            {
                printf("bad EOF value after k=%zu (%zu %zu)\n", k, step.left, step.right);
                fflush(NULL);
                __builtin_trap();
            }
        }

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
