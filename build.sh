#!/usr/bin/bash

# --- Defaults ---

BUILD_TYPE="Release"
BUILD_DOCS="ON"
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
    -P|--perf-stat)       RUN_PERF_STAT="ON";;
       --perf-record)     RUN_PERF_RECORD="ON";;
    -V|--memcheck)        RUN_MEMCHECK="ON";;
    *)
      echo "$0: unknown option \"$i\""
      exit 1
      ;;
  esac
done

mkdir -p bin
mkdir -p build
cd build
cmake .. \
  -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
  -DENABLE_TESTING=$ENABLE_TESTING \
  -DENABLE_TRACE=$ENABLE_TRACE \
  -DENABLE_TRACE_ESSENTIAL=$ENABLE_TRACE_ESSENTIAL

make -j8
mv compile_commands.json .. || true
cp ../static/config.toml ../bin/
cp ../static/config.toml ../build/

if [ "$ENABLE_TESTING" = "ON" ]
then
  ctest --output-on-failure || true
fi

cd ..

if [ "$BUILD_DOCS" = "ON" ]
then
  cp extern/doxygen_theme_flat_design/img/* .doxygen/html/
  doxygen Doxyfile
fi

if ! command -v perf &> /dev/null
then
  echo "perf could not be found."
else
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

if ! command -v valgrind &> /dev/null
then
  echo "valgrind could not be found."
else
  if [ "$RUN_MEMCHECK" = "ON" ]
  then
    #valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes bin/buddy8800 tests/res/diag2.com
    valgrind --tool=memcheck bin/buddy8800 "tests/res/cpudiag.bin"
  fi
fi
