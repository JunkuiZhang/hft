cmake_minimum_required(VERSION 3.16)
project(hft_engine VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# HFT specific compiler optimizations
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-O3 -march=native -Wall -Wextra -Wpedantic)
endif()

include_directories(include)

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/lib)

add_executable(hft_engine
    src/main.cpp
    src/market/MockMdReceiver.cpp
    src/strategy/StrategyEngine.cpp
    src/execution/ExecutionEngine.cpp
)

target_include_directories(hft_engine PRIVATE include)
