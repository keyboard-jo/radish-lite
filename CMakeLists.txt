cmake_minimum_required(VERSION 3.16.0)
project(radish-lite VERSION 0.1.0 LANGUAGES CXX) # C is likely unnecessary

add_compile_definitions(
  ASIO_STANDALONE
  _WIN32_WINNT=0x0A00
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
  Catch2
  GIT_REPOSITORY  https://github.com/catchorg/Catch2.git
  GIT_TAG         v3.8.1
)


FetchContent_Declare(
  asio
  GIT_REPOSITORY  https://github.com/chriskohlhoff/asio.git
  GIT_TAG         asio-1-36-0
)


FetchContent_MakeAvailable(Catch2 asio)

list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)


add_library(
  kvstore_lib 
  src/KeyValueStore.cpp
)
target_include_directories(
  kvstore_lib 
  PUBLIC 
  include
)


add_library(
  server_lib 
  src/Server.cpp
  src/Session.cpp
)
target_include_directories(
  server_lib
  PUBLIC
  include
  ${asio_SOURCE_DIR}/asio/include
)
target_link_libraries(
  server_lib
  PUBLIC
  kvstore_lib
  ws2_32
  mswsock
)

add_executable(
  radish-lite 
  src/main.cpp
)
target_link_libraries(
  radish-lite 
  PRIVATE 
  kvstore_lib
  server_lib
)

add_executable(
  tests 
  tests/test_key_value_store.cpp
)
target_link_libraries(
  tests 
  PRIVATE 
  Catch2::Catch2WithMain 
  kvstore_lib
)

include(CTest)
include(Catch)
catch_discover_tests(tests)