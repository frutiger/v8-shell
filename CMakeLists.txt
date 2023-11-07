cmake_minimum_required(VERSION 3.27)
project(v8-shell CXX)

add_library(
    v8 INTERFACE
)

target_include_directories(
    v8 INTERFACE
    ${V8_DIR}/include
)

target_link_libraries(
    v8 INTERFACE
    -L${V8_DIR}/${V8_BUILD}
    -lv8
)

add_library(
    v8-platform INTERFACE
)

target_include_directories(
    v8-platform INTERFACE
    ${V8_DIR}/libplatform
)

target_link_libraries(
    v8-platform INTERFACE
    -L${V8_DIR}/${V8_BUILD}
    -lv8_libplatform
)

add_executable(
    v8-shell
    v8-shell.m.cpp
)

target_compile_definitions(
    v8-shell PRIVATE
    V8_COMPRESS_POINTERS
    V8_ENABLE_SANDBOX
)

target_link_libraries(
    v8-shell PUBLIC
    v8
    v8-platform
)

set_target_properties(
    v8-shell PROPERTIES
    CXX_STANDARD 17
)

