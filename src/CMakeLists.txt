file(GLOB_RECURSE SOURCE_FILES *.cpp)
add_library(buddylib ${SOURCE_FILES})

target_include_directories(buddylib PUBLIC 
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_SOURCE_DIR}/src/cpu
    ${PROJECT_SOURCE_DIR}/src/util
)
add_executable(buddy8800 main.cpp)
target_link_libraries(buddy8800 PRIVATE buddylib)
