#!/usr/bin/bash

# --- Defaults ---

BUILD_TYPE="Release"
ENABLE_TESTING="OFF"
ENABLE_TRACE="OFF"
ENABLE_TRACE_ESSENTIAL="OFF"

# --- Defaults ---

set -e

for i in "$@"
do
  case $i in
    -d|--debug)
      BUILD_TYPE="Debug"
      shift
      ;;
    -r|--release)
      BUILD_TYPE="Release"
      shift
      ;;
    -t|--tests)
      ENABLE_TESTING="ON"
      shift
      ;;
    --trace)
      ENABLE_TRACE="ON"
      shift
      ;;
    --trace-essential)
      ENABLE_TRACE_ESSENTIAL="ON"
      shift
      ;;
    *)
      echo "Unknown option: $i"
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

make
mv compile_commands.json .. || true

if [ "$ENABLE_TESTING" = "ON" ]
then
  ctest --output-on-failure || true
fi

cd ..
