#!/usr/bin/env bash

set -e

cc -W -Wall -Os -c tiny_batcher.c
c++ -W -Wall -Os test.cpp tiny_batcher.o -o test

RC=0

for idx in `seq 0 1200`; do
    diff -q <(./test $idx) <(python3 reference.py $idx) >/dev/null || { echo "Failed $idx"; RC=1; };
done

# enough to fully sort up to 2**20
LIMIT=$((10 * 10 * 1024 * 1024))

for idx in `seq 3 62`; do
    for delta in -1 0 1; do
        n=$(((1 << $idx) + $delta))
        diff -q <(./test ${n} | head -n $LIMIT) \
             <(python3 reference.py ${n} 2>/dev/null | head -n $LIMIT) >/dev/null || \
            { echo "Failed ${n}"; RC=1; };
    done
done

exit $RC
