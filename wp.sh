#!/bin/bash
set -euo pipefail

WP_IMAGE="${WP_IMAGE:-frama-c-32}"
PAR=$(nproc 2>/dev/null || echo 8)
PAR=$((PAR < 16 ? PAR : 16))

WP_FCT="${WP_FCT:-tiny_batcher_generate}"
WP_FCT_FLAG="-wp-fct=$WP_FCT"

exec docker run \
  -v "$PWD:/workspace:rw,Z" \
  --user root:root --env=OPAMROOTISOK=1 \
  -w /workspace --rm "$WP_IMAGE" \
  frama-c \
    -lib-entry -constfold \
    -cpp-extra-args="-std=c2x" -machdep=gcc_x86_64 -std=c23 \
    tiny_batcher.c \
  -then \
    -wp-rte -wp-smoke-tests \
    -wp-model="typed+var+int+float+ref" \
    -wp-split -wp-cache=update \
    -wp-par="$PAR" -wp-timeout="${WP_TIMEOUT:-5}" \
    -wp-prover="alt-ergo,z3,cvc4" \
    $WP_FCT_FLAG \
    -wp -wp-status \
  -then \
    -report -report-classify -report-no-proven
