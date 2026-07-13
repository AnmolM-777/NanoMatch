cmake_minimum_required(VERSION 3.15)
project(NanoMatch LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)

add_executable(NanoMatch src/main.cpp)
add_executable(Benchmark tests/benchmark.cpp)
