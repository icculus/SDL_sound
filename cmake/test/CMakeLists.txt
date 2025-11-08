# This cmake build script is meant for verifying the various CMake configuration script.

cmake_minimum_required(VERSION 3.12)
project(sdl_test LANGUAGES C)

cmake_policy(SET CMP0074 NEW)

# Override CMAKE_FIND_ROOT_PATH_MODE to allow search for SDL3_sound outside of sysroot
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE NEVER)

include(FeatureSummary)

option(TEST_SHARED "Test linking to shared SDL3_sound library" ON)
add_feature_info("TEST_SHARED" TEST_SHARED "Test linking with shared library")

option(TEST_STATIC "Test linking to static SDL3_sound libary" ON)
add_feature_info("TEST_STATIC" TEST_STATIC "Test linking with static library")

if(TEST_SHARED)
    find_package(SDL3_sound REQUIRED CONFIG)
    find_package(SDL3 REQUIRED CONFIG)
    add_executable(main_shared main.c)
    target_link_libraries(main_shared PRIVATE SDL3_sound::SDL3_sound-shared SDL3::SDL3-shared)
endif()

if(TEST_STATIC)
    find_package(SDL3_sound REQUIRED CONFIG)
    find_package(SDL3 REQUIRED CONFIG)
    add_executable(main_static main.c)
    target_link_libraries(main_static PRIVATE SDL3_sound::SDL3_sound-static SDL3::SDL3)
endif()

feature_summary(WHAT ALL)
