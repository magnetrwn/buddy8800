find_package(Catch2 REQUIRED)

file(GLOB_RECURSE TEST_SOURCES *.cpp)
add_executable(tests ${TEST_SOURCES})
target_link_libraries(tests PRIVATE Catch2::Catch2WithMain buddylib)
target_include_directories(tests PRIVATE ${PROJECT_SOURCE_DIR}/src)

configure_file(${PROJECT_SOURCE_DIR}/tests/res/cpudiag.bin ${CMAKE_CURRENT_BINARY_DIR} COPYONLY)
configure_file(${PROJECT_SOURCE_DIR}/tests/res/passed.txt ${CMAKE_CURRENT_BINARY_DIR} COPYONLY)

include(CTest)
include(Catch)
catch_discover_tests(tests)
