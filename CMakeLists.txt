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
option(OPEN_ELITE_USE_SDL3 "Build the SDL3 gamepad backend and gamepad_test if SDL3 is found" ON)
option(OPEN_ELITE_USE_SDL2 "Build the game window (open_elite_game) if SDL2 is found" ON)

# The renderer: ship models, a software rasteriser, star field and text. No dependencies.
add_library(open_elite
  src/adlib_design.cpp
  src/adlib_synth.cpp
  src/ai.cpp
  src/asteroids.cpp
  src/blueprint.cpp
  src/bot.cpp
  src/builtin_blueprints.cpp
  src/cockpit.cpp
  src/commander.cpp
  src/commodity_models.cpp
  src/equipment_models.cpp
  src/galaxy.cpp
  src/game.cpp
  src/input.cpp
  src/json.cpp
  src/market.cpp
  src/music.cpp
  src/nebula.cpp
  src/obj.cpp
  src/png.cpp
  src/renderer.cpp
  src/screens.cpp
  src/showcase.cpp
  src/sound.cpp
  src/ships.cpp
  src/spawner.cpp
  src/stars.cpp
  src/steering.cpp
  src/text.cpp
  src/ui.cpp
  src/world.cpp
)
target_include_directories(open_elite PUBLIC include)
target_compile_features(open_elite PUBLIC cxx_std_17)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(open_elite PRIVATE -Wall -Wextra)
endif()

if(OPEN_ELITE_BUILD_APPS)
  add_executable(ship_viewer apps/ship_viewer/main.cpp)
  target_link_libraries(ship_viewer PRIVATE open_elite)
  add_executable(engine_demo apps/engine_demo/main.cpp)
  target_link_libraries(engine_demo PRIVATE open_elite)
  add_executable(flight_view apps/flight_view/main.cpp)
  target_link_libraries(flight_view PRIVATE open_elite)
  add_executable(sound_demo apps/sound_demo/main.cpp)
  target_link_libraries(sound_demo PRIVATE open_elite)
endif()

# Controller input through SDL3: Xbox 360, Xbox One and Xbox Elite pads. Optional, the rest builds without SDL.
if(OPEN_ELITE_USE_SDL3)
  find_package(SDL3 CONFIG QUIET)
  if(SDL3_FOUND)
    add_library(open_elite_sdl src/sdl_gamepad.cpp)
    target_link_libraries(open_elite_sdl PUBLIC open_elite SDL3::SDL3)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
      target_compile_options(open_elite_sdl PRIVATE -Wall -Wextra)
    endif()
    if(OPEN_ELITE_BUILD_APPS)
      add_executable(gamepad_test apps/gamepad_test/main.cpp)
      target_link_libraries(gamepad_test PRIVATE open_elite_sdl)
    endif()
  else()
    message(STATUS "SDL3 not found: building without gamepad support (install libsdl3-dev or set SDL3_DIR)")
  endif()
endif()

# The game itself, in a window: keyboard and Xbox style controller through SDL2. Optional.
if(OPEN_ELITE_USE_SDL2 AND OPEN_ELITE_BUILD_APPS)
  find_package(SDL2 CONFIG QUIET)
  if(SDL2_FOUND)
    add_executable(open_elite_game apps/play/main.cpp)
    target_link_libraries(open_elite_game PRIVATE open_elite)
    if(TARGET SDL2::SDL2)
      target_link_libraries(open_elite_game PRIVATE SDL2::SDL2)
    else()
      target_include_directories(open_elite_game PRIVATE ${SDL2_INCLUDE_DIRS})
      target_link_libraries(open_elite_game PRIVATE ${SDL2_LIBRARIES})
    endif()
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
      target_compile_options(open_elite_game PRIVATE -Wall -Wextra)
    endif()
  else()
    message(STATUS "SDL2 not found: the game window (open_elite_game) is not built (install libsdl2-dev)")
  endif()
endif()

if(OPEN_ELITE_BUILD_TESTS)
  enable_testing()
  add_executable(open_elite_tests tests/open_elite_tests.cpp)
  target_link_libraries(open_elite_tests PRIVATE open_elite)
  add_test(NAME open_elite_tests COMMAND open_elite_tests)
  add_executable(open_elite_engine_tests tests/engine_tests.cpp)
  target_link_libraries(open_elite_engine_tests PRIVATE open_elite)
  add_test(NAME open_elite_engine_tests COMMAND open_elite_engine_tests)
  add_executable(open_elite_view_tests tests/view_tests.cpp)
  target_link_libraries(open_elite_view_tests PRIVATE open_elite)
  add_test(NAME open_elite_view_tests COMMAND open_elite_view_tests)
  add_executable(open_elite_ui_tests tests/ui_tests.cpp)
  target_link_libraries(open_elite_ui_tests PRIVATE open_elite)
  add_test(NAME open_elite_ui_tests COMMAND open_elite_ui_tests)
  add_executable(open_elite_sound_tests tests/sound_tests.cpp)
  target_link_libraries(open_elite_sound_tests PRIVATE open_elite)
  add_test(NAME open_elite_sound_tests COMMAND open_elite_sound_tests)
  add_executable(open_elite_music_tests tests/music_tests.cpp)
  target_link_libraries(open_elite_music_tests PRIVATE open_elite)
  add_test(NAME open_elite_music_tests COMMAND open_elite_music_tests)
endif()
