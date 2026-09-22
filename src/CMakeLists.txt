add_library(buddy8800_platform core/iface/unix_pty.cpp platform/executable_path.cpp)
target_include_directories(buddy8800_platform PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_library(buddy8800_core core/cards/serial_card.cpp)
target_include_directories(buddy8800_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(buddy8800_core PUBLIC buddy8800_platform)

add_library(buddy8800_application
    app/binary_file.cpp app/configuration.cpp app/system_config.cpp
    app/options.cpp app/program_loader.cpp app/emulator.cpp app/execution.cpp)
target_link_libraries(buddy8800_application PUBLIC buddy8800_core PRIVATE toml11)

add_library(buddy8800_presentation ux/machine_view.cpp ux/plain.cpp)
target_link_libraries(buddy8800_presentation PUBLIC buddy8800_application)

add_library(buddy8800_frontend ux/frontend.cpp)
target_link_libraries(buddy8800_frontend PUBLIC buddy8800_presentation)
if(DISABLE_TRACE)
    target_compile_definitions(buddy8800_frontend PRIVATE DISABLE_TRACE)
else()
    set(CURSES_NEED_NCURSES TRUE)
    find_package(Curses REQUIRED)
    target_sources(buddy8800_frontend PRIVATE ux/tui.cpp ux/tui/screen.cpp)
    target_include_directories(buddy8800_frontend PRIVATE ${CURSES_INCLUDE_DIRS})
    target_link_libraries(buddy8800_frontend PRIVATE ${CURSES_LIBRARIES})
endif()

add_executable(buddy8800 main.cpp)
target_link_libraries(buddy8800 PRIVATE buddy8800_frontend)

configure_file(${PROJECT_SOURCE_DIR}/static/config.toml ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/config.toml COPYONLY)
configure_file(${PROJECT_SOURCE_DIR}/static/altmon.bin ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/altmon.bin COPYONLY)
configure_file(${PROJECT_SOURCE_DIR}/static/8kbas.bin ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/8kbas.bin COPYONLY)
