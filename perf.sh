#!/usr/bin/bash

set -e

if ! command -v perf &> /dev/null; then
    echo "perf could not be found."
    exit 1
fi

perf stat -d bin/buddy8800 tests/res/diag2.com
perf record -F 8000 -g -- bin/buddy8800 tests/res/diag2.com
#perf stat -B -e cache-misses,cache-references,branches,branch-misses,instructions,cycles bin/buddy8800 tests/res/diag2.com
perf report
rm perf.data
