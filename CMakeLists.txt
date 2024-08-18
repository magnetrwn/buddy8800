cmake_minimum_required(VERSION 3.12)
project(buddy8800)

set(CMAKE_BUILD_TYPE Debug)
option(ENABLE_TESTING "Enable testing" ON)

add_compile_definitions(-DENABLE_TRACE)
add_compile_definitions(-DENABLE_TRACE_ESSENTIAL)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)
set(CMAKE_STATIC_LIBRARY_PREFIX "")
set(CMAKE_CXX_FLAGS "-Wall -Wextra -Wpedantic")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -no-pie")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -Werror")

add_subdirectory(src)

if(ENABLE_TESTING)
    message(STATUS "Testing enabled. Looking for Catch2...")
    enable_testing()

    find_package(Catch2 3)

    if(NOT Catch2_FOUND)
        message(STATUS "Catch2 not found, fetching from submodule.")
        add_subdirectory(extern/Catch2)
    else()
        message(STATUS "Catch2 found.")
    endif()

    add_subdirectory(tests)
endif()
