cmake_minimum_required(VERSION 3.16)
# The version is in the VERSION file in the root
file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" CORIOLIS_VERSION_FROM_FILE LIMIT_COUNT 1)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/VERSION")
project(coriolis VERSION ${CORIOLIS_VERSION_FROM_FILE} LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release)
endif()

option(CORIOLIS_BUILD_APPS "Build the ship viewer" ON)
option(CORIOLIS_BUILD_TESTS "Build the tests" ON)
option(CORIOLIS_USE_SDL3 "Build the SDL3 gamepad backend and gamepad_test if SDL3 is found" ON)
option(CORIOLIS_USE_SDL2 "Build the game window (coriolis_game) if SDL2 is found" ON)

# The renderer: ship models, a software rasteriser, star field and text. No dependencies.
add_library(coriolis
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
  src/missions.cpp
  src/music.cpp
  src/nebula.cpp
  src/obj.cpp
  src/parallel.cpp
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
find_package(Threads REQUIRED)
target_link_libraries(coriolis PUBLIC Threads::Threads)
target_include_directories(coriolis PUBLIC include)
target_compile_features(coriolis PUBLIC cxx_std_17)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(coriolis PRIVATE -Wall -Wextra)
endif()

if(CORIOLIS_BUILD_APPS)
  add_executable(ship_viewer apps/ship_viewer/main.cpp)
  target_link_libraries(ship_viewer PRIVATE coriolis)
  add_executable(engine_demo apps/engine_demo/main.cpp)
  target_link_libraries(engine_demo PRIVATE coriolis)
  add_executable(flight_view apps/flight_view/main.cpp)
  target_link_libraries(flight_view PRIVATE coriolis)
  add_executable(sound_demo apps/sound_demo/main.cpp)
  target_link_libraries(sound_demo PRIVATE coriolis)
  add_executable(site_gen apps/site_gen/main.cpp)
  target_link_libraries(site_gen PRIVATE coriolis)
endif()

# Controller input through SDL3: Xbox 360, Xbox One and Xbox Elite pads. Optional, the rest builds without SDL.
if(CORIOLIS_USE_SDL3)
  find_package(SDL3 CONFIG QUIET)
  if(SDL3_FOUND)
    add_library(coriolis_sdl src/sdl_gamepad.cpp)
    target_link_libraries(coriolis_sdl PUBLIC coriolis SDL3::SDL3)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
      target_compile_options(coriolis_sdl PRIVATE -Wall -Wextra)
    endif()
    if(CORIOLIS_BUILD_APPS)
      add_executable(gamepad_test apps/gamepad_test/main.cpp)
      target_link_libraries(gamepad_test PRIVATE coriolis_sdl)
    endif()
  else()
    message(STATUS "SDL3 not found: building without gamepad support (install libsdl3-dev or set SDL3_DIR)")
  endif()
endif()

# The game itself, in a window: keyboard and Xbox style controller through SDL2. Optional.
if(CORIOLIS_USE_SDL2 AND CORIOLIS_BUILD_APPS)
  find_package(SDL2 CONFIG QUIET)
  if(SDL2_FOUND)
    add_executable(coriolis_game apps/play/main.cpp)
    target_link_libraries(coriolis_game PRIVATE coriolis)
    if(TARGET SDL2::SDL2)
      target_link_libraries(coriolis_game PRIVATE SDL2::SDL2)
    else()
      target_include_directories(coriolis_game PRIVATE ${SDL2_INCLUDE_DIRS})
      target_link_libraries(coriolis_game PRIVATE ${SDL2_LIBRARIES})
    endif()
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
      target_compile_options(coriolis_game PRIVATE -Wall -Wextra)
    endif()
  else()
    message(STATUS "SDL2 not found: the game window (coriolis_game) is not built (install libsdl2-dev)")
  endif()
endif()

if(CORIOLIS_BUILD_TESTS)
  enable_testing()
  add_executable(coriolis_tests tests/coriolis_tests.cpp)
  target_link_libraries(coriolis_tests PRIVATE coriolis)
  add_test(NAME coriolis_tests COMMAND coriolis_tests)
  add_executable(coriolis_engine_tests tests/engine_tests.cpp)
  target_link_libraries(coriolis_engine_tests PRIVATE coriolis)
  add_test(NAME coriolis_engine_tests COMMAND coriolis_engine_tests)
  add_executable(coriolis_view_tests tests/view_tests.cpp)
  target_link_libraries(coriolis_view_tests PRIVATE coriolis)
  add_test(NAME coriolis_view_tests COMMAND coriolis_view_tests)
  add_executable(coriolis_ui_tests tests/ui_tests.cpp)
  target_link_libraries(coriolis_ui_tests PRIVATE coriolis)
  add_test(NAME coriolis_ui_tests COMMAND coriolis_ui_tests)
  add_executable(coriolis_sound_tests tests/sound_tests.cpp)
  target_link_libraries(coriolis_sound_tests PRIVATE coriolis)
  add_test(NAME coriolis_sound_tests COMMAND coriolis_sound_tests)
  add_executable(coriolis_music_tests tests/music_tests.cpp)
  target_link_libraries(coriolis_music_tests PRIVATE coriolis)
  add_test(NAME coriolis_music_tests COMMAND coriolis_music_tests)
endif()

# Installing and packaging: `cpack -G DEB` in the build directory makes a .deb of the game (needs SDL2, so that
# coriolis_game is built). CORIOLIS_DISTRO names the distribution it was built on, for the file name, and
# the package version is in the VERSION file, and CORIOLIS_PACKAGE_VERSION overrides it (the release workflow sets it
# from the tag).
if(TARGET coriolis_game)
  include(GNUInstallDirs)
  install(TARGETS coriolis_game RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
  install(FILES packaging/coriolis.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
  install(FILES packaging/coriolis.png DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/256x256/apps)
  install(FILES README.md DESTINATION ${CMAKE_INSTALL_DOCDIR})
  install(FILES LICENSE DESTINATION ${CMAKE_INSTALL_DOCDIR} RENAME copyright)
  install(FILES VERSION DESTINATION ${CMAKE_INSTALL_DOCDIR})

  # The package version is the one in the VERSION file unless CORIOLIS_PACKAGE_VERSION says otherwise
  set(CORIOLIS_PACKAGE_VERSION "" CACHE STRING "Version of the package, if not the one in the VERSION file")
  if(NOT CORIOLIS_PACKAGE_VERSION)
    set(CORIOLIS_PACKAGE_VERSION "${PROJECT_VERSION}")
  endif()
  set(CORIOLIS_DISTRO "" CACHE STRING "Distribution the package is built for, such as ubuntu24.04")
  execute_process(COMMAND dpkg --print-architecture OUTPUT_VARIABLE CORIOLIS_DEB_ARCH
                  OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
  if(NOT CORIOLIS_DEB_ARCH)
    set(CORIOLIS_DEB_ARCH "${CMAKE_SYSTEM_PROCESSOR}")
  endif()

  set(CPACK_PACKAGE_NAME "coriolis")
  set(CPACK_PACKAGE_VERSION "${CORIOLIS_PACKAGE_VERSION}")
  set(CPACK_PACKAGE_CONTACT "Ross Newman <37403843+ross-newman@users.noreply.github.com>")
  set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/ross-newman/coriolis")
  set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Open source space trading and dogfighting game inspired by Elite")
  set(CPACK_PACKAGE_DESCRIPTION
      "A playable space trading and dogfighting game: flight, combat with AI pilots, stations and docking, hyperspace, markets and trading, with a software rendered 3D cockpit, FM synthesized sound and music.")
  set(CPACK_GENERATOR "DEB")
  set(CPACK_PACKAGING_INSTALL_PREFIX "/usr")
  set(CPACK_STRIP_FILES ON)
  set(CPACK_DEBIAN_PACKAGE_SECTION "games")
  set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
  set(CPACK_DEBIAN_PACKAGE_RELEASE "1")
  if(CORIOLIS_DISTRO)
    set(CPACK_DEBIAN_FILE_NAME
        "coriolis_${CPACK_PACKAGE_VERSION}-${CPACK_DEBIAN_PACKAGE_RELEASE}_${CORIOLIS_DISTRO}_${CORIOLIS_DEB_ARCH}.deb")
  else()
    set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
  endif()
  include(CPack)
endif()
