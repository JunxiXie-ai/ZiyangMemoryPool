cmake_minimum_required(VERSION 3.16)

project(ZiyangMemoryPool LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(memory_pool_test
    tests/main.cpp
)

target_include_directories(memory_pool_test
    PRIVATE ${PROJECT_SOURCE_DIR}/include
)