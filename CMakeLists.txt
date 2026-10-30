cmake_minimum_required(VERSION 3.16)
project(open_elite VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release)
endif()

option(OPEN_ELITE_BUILD_APPS "Build the ship viewer" ON)
option(OPEN_ELITE_BUILD_TESTS "Build the tests" ON)

# The renderer: ship models, a software rasteriser, star field and text. No dependencies.
add_library(open_elite
  src/png.cpp
  src/renderer.cpp
  src/showcase.cpp
  src/ships.cpp
  src/stars.cpp
  src/text.cpp
)
target_include_directories(open_elite PUBLIC include)
target_compile_features(open_elite PUBLIC cxx_std_17)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(open_elite PRIVATE -Wall -Wextra)
endif()

if(OPEN_ELITE_BUILD_APPS)
  add_executable(ship_viewer apps/ship_viewer/main.cpp)
  target_link_libraries(ship_viewer PRIVATE open_elite)
endif()

if(OPEN_ELITE_BUILD_TESTS)
  enable_testing()
  add_executable(open_elite_tests tests/open_elite_tests.cpp)
  target_link_libraries(open_elite_tests PRIVATE open_elite)
  add_test(NAME open_elite_tests COMMAND open_elite_tests)
endif()
