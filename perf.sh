#!/usr/bin/bash

set -e

if ! command -v perf &> /dev/null
then
  echo "perf could not be found."
  exit 1
fi

# TEST WITH PERF STAT
perf stat --repeat=5 --table --detailed bin/buddy8800 tests/res/diag2.com

# TEST WITH PERF RECORD
#perf record -F 8000 -g -- bin/buddy8800 tests/res/diag2.com
#perf report
#rm perf.data
