cmake_minimum_required(VERSION 3.5)
project(cpp-reduce)

enable_testing()
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -export-dynamic -O3")

# add_subdirectory(src)
add_subdirectory(tests)

include(dependencies.cmake)
