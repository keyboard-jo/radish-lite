cmake_minimum_required(VERSION 3.16.0)
project(radish-lite VERSION 0.1.0 LANGUAGES CXX) # C is likely unnecessary

set(CMAKE_CXX_STANDARD 17) # Ensure C++17 for <optional> and other features
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        v3.8.1
)
FetchContent_MakeAvailable(Catch2)

list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)

add_library(kvstore_lib src/KeyValueStore.cpp)
target_include_directories(kvstore_lib PUBLIC include)

add_executable(radish-lite src/main.cpp)
target_link_libraries(radish-lite PRIVATE kvstore_lib)

add_executable(tests tests/test_key_value_store.cpp)
target_link_libraries(tests PRIVATE Catch2::Catch2WithMain kvstore_lib)

include(CTest)
include(Catch)
catch_discover_tests(tests)