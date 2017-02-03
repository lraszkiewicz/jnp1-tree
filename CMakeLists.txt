cmake_minimum_required(VERSION 3.6)
project(tree)

set(CMAKE_CXX_STANDARD 14)

set(SOURCE_FILES tree.h)
add_library(tree ${SOURCE_FILES})
add_executable(tree_example tree_example.cc)
add_executable(tree_test tree_test.cc)