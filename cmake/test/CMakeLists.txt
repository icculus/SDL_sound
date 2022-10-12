# This cmake build script is meant for verifying the various CMake configuration script.

cmake_minimum_required(VERSION 3.12)
project(sdl_test LANGUAGES C)

cmake_policy(SET CMP0074 NEW)

# Override CMAKE_FIND_ROOT_PATH_MODE to allow search for SDL2_sound outside of sysroot
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE NEVER)

include(FeatureSummary)

option(TEST_SHARED "Test linking to shared SDL2_sound library" ON)
add_feature_info("TEST_SHARED" TEST_SHARED "Test linking with shared library")

option(TEST_STATIC "Test linking to static SDL2_sound libary" ON)
add_feature_info("TEST_STATIC" TEST_STATIC "Test linking with static library")

macro(my_find_sdl2)
    # FIXME: this should unconditionally use CMake config file
    find_package(SDL2)
    if(NOT TARGET SDL2::SDL2)
        find_library(SDL2_LIBRARY NAMES SDL2)
        find_path(SDL2_INCLUDE_DIRS FILES SDL.h SUFFIXES SDL2)
        if(NOT SDL2_LIBRARY OR NOT SDL2_INCLUDE_DIRS)
            message(FATAL_ERROR "Faild to find SDL2 (configure SDL2_LIBRARY and SDL2_INCLUDE_DIRS)")
        endif()
        add_library(SDL2::SDL2 SHARED IMPORTED)
        set_target_properties(SDL2::SDL2 PROPERTIES IMPORTED_LOCATION "${SDL2_LIBRARY}" INTERFACE_INCLUDE_DIRECTORIES "${SDL2_INCLUDE_DIRS}")
    endif()
endmacro()

if(TEST_SHARED)
    find_package(SDL2_sound REQUIRED CONFIG)
    my_find_sdl2()
    add_executable(main_shared main.c)
    target_link_libraries(main_shared PRIVATE SDL2_sound::SDL2_sound SDL2::SDL2)
endif()

if(TEST_STATIC)
    find_package(SDL2_sound REQUIRED CONFIG)
    my_find_sdl2()
    add_executable(main_static main.c)
    target_link_libraries(main_static PRIVATE SDL2_sound::SDL2_sound-static SDL2::SDL2)
endif()

feature_summary(WHAT ALL)
