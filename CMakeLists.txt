cmake_minimum_required(VERSION 3.12)
project(buddy8800)

# Set the C++ standard to C++11
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set the output directory for the build artifacts
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Add the source files
file(GLOB_RECURSE SOURCES src/*.cpp)

# Add the executable target
add_executable(buddy8800 ${SOURCES})

# Add the include directories
target_include_directories(buddy8800 PRIVATE src)

# Add the test files
file(GLOB_RECURSE TEST_SOURCES tests/*.cpp)

# Add the test executable target
add_executable(tests ${TEST_SOURCES})

# Add the include directories for tests
target_include_directories(tests PRIVATE src)

# Link the Catch2 library for tests
target_link_libraries(tests PRIVATE Catch2::Catch2)

# Enable CTest for running tests
include(CTest)
include(Catch)
catch_discover_tests(tests)