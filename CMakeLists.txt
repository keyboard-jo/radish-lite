cmake_minimum_required(VERSION 3.10.0)
project(radish-lite VERSION 0.1.0 LANGUAGES C CXX)

add_executable(
    radish-lite 
    src/main.cpp
    src/KeyValueStore.cpp
)

target_include_directories(radish-lite PRIVATE include)

include(CTest)
enable_testing()

