# Native build for the Android app: SDL2 (shared, fetched by fetch-deps.sh) + the game (repo root) -> libtinkerbell.so
cmake_minimum_required(VERSION 3.16)
project(tinkerbell_android C CXX)

if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/SDL/CMakeLists.txt)
  message(FATAL_ERROR "SDL sources are missing - run ./fetch-deps.sh from the repo root first")
endif()

set(SDL_SHARED ON CACHE BOOL "" FORCE)
set(SDL_STATIC OFF CACHE BOOL "" FORCE)
set(SDL_TEST OFF CACHE BOOL "" FORCE)
add_subdirectory(SDL)

# the game itself: /CMakeLists.txt (sees the SDL2 target above and builds libtinkerbell.so)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../.. game)
