#pragma once
#include "tiny_batcher.h"

#include <algorithm>
#include <utility>

template <typename It, typename Comp>
[[gnu::always_inline]] inline void tiny_batcher_sort(It base, size_t n, Comp comparator)
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

template <typename It>
void tiny_batcher_sort(It base, size_t n)
{
    size_t left, right;
    TINY_BATCHER_SORT_LOOP(n, left, right)
    {
	auto &x = *(base + left);
	auto &y = *(base + right);

	auto comp = std::minmax(x, y);

	auto xp = std::move(comp.first);
	auto yp = std::move(comp.second);

	x = std::move(xp);
	y = std::move(yp);
    }
}

template <typename T>
[[gnu::always_inline]] inline void tiny_batcher_sort(T &container)
{
    tiny_batcher_sort(container.begin(), container.size());
}

template <typename T, typename Comp>
[[gnu::always_inline]] inline void tiny_batcher_sort(T &container, Comp comparator)
{
    tiny_batcher_sort(container.begin(), container.size(), std::move(comparator));
}

