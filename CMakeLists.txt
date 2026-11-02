cmake_minimum_required(VERSION 3.10)
project(avm2_vm VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Library target
add_library(avm2_vm STATIC
    src/avm2_vm.cpp
)

target_include_directories(avm2_vm PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# Demo executable
add_executable(avm2_demo
    example/main.cpp
)

target_link_libraries(avm2_demo PRIVATE avm2_vm)
