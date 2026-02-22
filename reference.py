import sys

# https://oeis.org/A375649: Number of comparisons and swaps in the
# Batcher odd-even merge sort needed to sort n items, plus an extra
# 0 for n = 0.
#
# We actually implement a slightly different algorithm (still by
# Batcher) that needs slightly fewer swaps.
EXPECTED_SWAPS = [
    0, 0, 1, 3, 5, 9, 12, 16, 19, 28, 32, 38, 42, 48, 53, 59, 63, 85,
    90, 98, 103, 112, 119, 127, 132, 140, 147, 156, 162, 171, 178,
    186, 191, 246, 252, 262, 268, 280, 289, 299, 305, 317, 327, 339,
    347, 359, 368, 378, 384, 394, 403, 415, 423, 436, 446, 457, 464,
    476, 486, 498, 506
]


# A generator of indices for Batcher's merge-exchange sort transliterated
# from TAoCP Vol 3, 2nd edition (Section 5.2.2, p111).
def algorithM(N):
    if N < 2:  # Assume N >= 2
        return

    # M1
    t = (N - 1).bit_length()
    p = 2**(t - 1)

    while True:  # M2
        q = 2**(t - 1)
        r = 0
        d = p
        while True:  # M3
            # M3
            for i in range(N - d):
                if (i & p) == r:  # M4
                    yield i, i + d

            # M5
            if q == p:
                break
            d = q - p
            q //= 2
            r = p

        # M6
        p //= 2
        if p == 0:
            break


if __name__ == "__main__":
    n = int(sys.argv[1])
    swaps = 0
    for i, j in algorithM(n):
        swaps += 1
        print(f"{i} {j}")

    assert n >= len(EXPECTED_SWAPS) or swaps <= EXPECTED_SWAPS[n], \
        f"unexpected number of swaps {swaps} > {EXPECTED_SWAPS[n]} in algorithm M"
