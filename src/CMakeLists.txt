file(GLOB_RECURSE SOURCE_FILES *.cpp)
add_library(buddylib ${SOURCE_FILES})

target_include_directories(buddylib PUBLIC ${PROJECT_SOURCE_DIR}/src)
add_executable(buddy8800 main.cpp)
target_link_libraries(buddy8800 PRIVATE buddylib)
