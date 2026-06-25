cmake_minimum_required(VERSION 3.10.0)
project(radish-lite VERSION 0.1.0 LANGUAGES C CXX)

add_executable(radish-lite main.cpp)

include(CTest)
enable_testing()

