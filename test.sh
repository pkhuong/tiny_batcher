#!/usr/bin/env bash

set -e

cc -Os -c tiny_batcher.c
c++ -Os test.cpp tiny_batcher.o -o test

for idx in `seq 0 1200`; do
    diff -q <(./test $idx) <(python3 reference.py $idx) >/dev/null || echo "Failed $idx";
done

# enough to fully sort up to 2**20
LIMIT=$((10 * 10 * 1024 * 1024))

for idx in `seq 60`; do
    diff -q <(./test $((1 << $idx)) | head -n $LIMIT) \
            <(python3 reference.py $((1 << $idx)) 2>/dev/null | head -n $LIMIT) >/dev/null || \
        echo "Failed $((1 << $idx))";
done
