file(GLOB_RECURSE TEST_SOURCES *.cpp)
add_executable(tests ${TEST_SOURCES})
target_link_libraries(tests PRIVATE Catch2::Catch2WithMain buddylib toml11)
target_include_directories(tests PRIVATE 
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_SOURCE_DIR}/extern/toml11/include
)

file(GLOB TEST_RESOURCES "${PROJECT_SOURCE_DIR}/tests/res/*")
foreach(TEST_FILE ${TEST_RESOURCES})
    get_filename_component(FILENAME ${TEST_FILE} NAME)
    configure_file(${TEST_FILE} ${CMAKE_CURRENT_BINARY_DIR}/${FILENAME} COPYONLY)
endforeach()

include(CTest)
include(Catch)
catch_discover_tests(tests)
