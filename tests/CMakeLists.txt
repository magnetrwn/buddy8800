find_package(Catch2 REQUIRED)

file(GLOB_RECURSE TEST_SOURCES *.cpp)
add_executable(tests ${TEST_SOURCES})
target_link_libraries(tests PRIVATE Catch2::Catch2WithMain)
target_include_directories(tests PRIVATE ${PROJECT_SOURCE_DIR}/src)

include(CTest)
include(Catch)
catch_discover_tests(tests)
