add_library(buddylib core/iface/unix_pty.cpp)

target_include_directories(buddylib PUBLIC 
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_SOURCE_DIR}/src/core/cpu
    ${PROJECT_SOURCE_DIR}/src/core/iface
    ${PROJECT_SOURCE_DIR}/src/core/bus
    ${PROJECT_SOURCE_DIR}/src/util
    ${PROJECT_SOURCE_DIR}/src/ux
    ${PROJECT_SOURCE_DIR}/extern/toml11/include
)

add_executable(buddy8800 main.cpp)
target_link_libraries(buddy8800 PRIVATE buddylib toml11)
if(DISABLE_TRACE)
    target_compile_definitions(buddy8800 PRIVATE DISABLE_TRACE)
else()
    set(CURSES_NEED_NCURSES TRUE)
    find_package(Curses REQUIRED)
    target_sources(buddy8800 PRIVATE ux/tui.cpp)
    target_include_directories(buddy8800 PRIVATE ${CURSES_INCLUDE_DIRS})
    target_link_libraries(buddy8800 PRIVATE ${CURSES_LIBRARIES})
endif()
configure_file(${PROJECT_SOURCE_DIR}/static/config.toml ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/config.toml COPYONLY)
configure_file(${PROJECT_SOURCE_DIR}/static/f800mon.bin ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/f800mon.bin COPYONLY)
