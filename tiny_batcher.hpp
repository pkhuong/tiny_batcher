#pragma once
#include "tiny_batcher.h"

#include <algorithm>
#include <utility>

// Ensures `[base, base + n)` is in ascending order wrt `comparator`.
//
// This sort is *not* stable.
template <typename It, typename Comp>
[[gnu::always_inline]] inline void
tiny_batcher_sort(It base, size_t n, Comp comparator)
{
    size_t left, right;
    TINY_BATCHER_SORT_LOOP(n, left, right)
    {
        auto &x = *(base + left);
        auto &y = *(base + right);

        if (comparator(y, x))
        {
            using namespace std;
            swap(x, y);
        }
    }
}

// Unstable sort of `[base, base + n)`.
//
// This template isn't `always_inline` because there's a good chance of
// reuse, and the compiler should be able to inline single-use expansions.
template <typename It>
void
tiny_batcher_sort(It base, size_t n)
{
    size_t left, right;
    TINY_BATCHER_SORT_LOOP(n, left, right)
    {
        auto &x = *(base + left);
        auto &y = *(base + right);

        auto comp = std::minmax(x, y);

        auto xp = std::move(const_cast<decltype(x)>(comp.first));
        auto yp = std::move(const_cast<decltype(y)>(comp.second));

        x = std::move(xp);
        y = std::move(yp);
    }
}

// Trivial container -> (random access) iterator wrappers.
template <typename T>
[[gnu::always_inline]] inline void
tiny_batcher_sort(T &container)
{
    tiny_batcher_sort(container.begin(), container.size());
}

template <typename T, typename Comp>
[[gnu::always_inline]] inline void
tiny_batcher_sort(T &container, Comp comparator)
{
    tiny_batcher_sort(container.begin(), container.size(), std::move(comparator));
}
