#!/usr/bin/env bash

set -e

cc -Os -c tiny_batcher.c
c++ -Os test.cpp tiny_batcher.o -o test

for idx in `seq 0 1200`; do
    diff -q <(./test $idx) <(python3 reference.py $idx) >/dev/null || echo "Failed $idx";
done

for idx in `seq 20`; do
    diff -q <(./test $((1 << $idx))) <(python3 reference.py $((1 << $idx))) >/dev/null || \
        echo "Failed $((1 << $idx))";
done
