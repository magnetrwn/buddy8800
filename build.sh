#!/usr/bin/bash

# --- Defaults ---

BUILD_TYPE="Release"
BUILD_DOCS="OFF"
ENABLE_TESTING="OFF"
ENABLE_TRACE="OFF"
ENABLE_TRACE_ESSENTIAL="OFF"
RUN_PERF_STAT="OFF"
RUN_PERF_RECORD="OFF"
RUN_MEMCHECK="OFF"

# --- Defaults ---

set -e

for i in "$@"
do
  case $i in
    -d|--debug)           BUILD_TYPE="Debug";;
    -r|--release)         BUILD_TYPE="Release";;
    -T|--tests)           ENABLE_TESTING="ON";;
       --trace)           ENABLE_TRACE="ON";;
       --trace-essential) ENABLE_TRACE_ESSENTIAL="ON";;
       --docs)            BUILD_DOCS="ON";;
    -P|--perf-stat)       RUN_PERF_STAT="ON";;
       --perf-record)     RUN_PERF_RECORD="ON";;
    -V|--memcheck)        RUN_MEMCHECK="ON";;
    *)
      echo "$0: unknown option \"$i\""
      exit 1
      ;;
  esac
done

cd -- "$(dirname -- "${BASH_SOURCE[0]}")"
build_dir="${BUDDY8800_BUILD_DIR:-build-linux}"
cmake -S . -B "$build_dir" \
  -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
  -DENABLE_TESTING=$ENABLE_TESTING \
  -DENABLE_TRACE=$ENABLE_TRACE \
  -DENABLE_TRACE_ESSENTIAL=$ENABLE_TRACE_ESSENTIAL

cmake --build "$build_dir" -j8
cp "$build_dir/compile_commands.json" compile_commands.json

if [ "$ENABLE_TESTING" = "ON" ]
then
  ctest --test-dir "$build_dir" --output-on-failure
fi

if [ "$BUILD_DOCS" = "ON" ]
then
  doxygen Doxyfile
fi

if [ "$RUN_PERF_STAT" = "ON" ] || [ "$RUN_PERF_RECORD" = "ON" ]
then
  command -v perf > /dev/null || { echo "perf could not be found."; exit 1; }
  if [ "$RUN_PERF_STAT" = "ON" ]
  then
    perf stat --repeat=5 --table --detailed bin/buddy8800 tests/res/diag2.com 0x100
  fi

  if [ "$RUN_PERF_RECORD" = "ON" ]
  then
    perf record -F 8000 -g -- bin/buddy8800 tests/res/diag2.com 0x100
    perf report
    rm perf.data
  fi
fi

if [ "$RUN_MEMCHECK" = "ON" ]
then
    command -v valgrind > /dev/null || { echo "valgrind could not be found."; exit 1; }
    #valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes bin/buddy8800 tests/res/diag2.com
    valgrind --tool=memcheck bin/buddy8800 "tests/res/cpudiag.bin"
fi
