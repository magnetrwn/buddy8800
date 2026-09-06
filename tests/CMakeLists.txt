file(GLOB_RECURSE TEST_SOURCES *.cpp)
add_executable(tests ${TEST_SOURCES})
target_link_libraries(tests PRIVATE Catch2::Catch2WithMain buddylib toml11)
set(BUDDY8800_MONITOR_FIXTURE_DIR ${CMAKE_CURRENT_BINARY_DIR}/fixtures/altmon)
foreach(BUDDY8800_MONITOR_FIXTURE config.toml f800mon.bin)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/fixtures/altmon/${BUDDY8800_MONITOR_FIXTURE}
        ${BUDDY8800_MONITOR_FIXTURE_DIR}/${BUDDY8800_MONITOR_FIXTURE} COPYONLY)
endforeach()
target_compile_definitions(tests PRIVATE
    BUDDY8800_MONITOR_CONFIG="${BUDDY8800_MONITOR_FIXTURE_DIR}/config.toml")
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
catch_discover_tests(tests PROPERTIES TIMEOUT 60 RUN_SERIAL TRUE)
find_package(Python3 REQUIRED COMPONENTS Interpreter)
add_test(NAME monitor_pty COMMAND ${Python3_EXECUTABLE}
    ${CMAKE_CURRENT_SOURCE_DIR}/test_monitor.py $<TARGET_FILE:buddy8800>
    ${BUDDY8800_MONITOR_FIXTURE_DIR}/config.toml)
set_tests_properties(monitor_pty PROPERTIES TIMEOUT 30)
