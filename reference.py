import math
import sys

# A generator of indices for Batcher's odd/even merge sort,
# transliterated from TAoCP Vol 3, 2nd edition.
def algorithM(N):
    if N < 2:  # Assume N >= 2
        return

    # M1
    t = math.ceil(math.log2(N))
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
    for i, j in algorithM(int(sys.argv[1])):
        print(f"{i} {j}")
