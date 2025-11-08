# SDL3_sound CMake configuration file:
# This file is meant to be placed in a cmake subfolder of SDL3_sound-devel-3.x.y-mingw

if(CMAKE_SIZEOF_VOID_P EQUAL 4)
    set(sdl3_sound_config_path "${CMAKE_CURRENT_LIST_DIR}/../i686-w64-mingw32/lib/cmake/SDL3_sound/SDL3_soundConfig.cmake")
elseif(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(sdl3_sound_config_path "${CMAKE_CURRENT_LIST_DIR}/../x86_64-w64-mingw32/lib/cmake/SDL3_sound/SDL3_soundConfig.cmake")
else("${CMAKE_SIZEOF_VOID_P}" STREQUAL "")
    set(SDL3_sound_FOUND FALSE)
    return()
endif()

if(NOT EXISTS "${sdl3_sound_config_path}")
    message(WARNING "${sdl3_sound_config_path} does not exist: MinGW development package is corrupted")
    set(SDL3_sound_FOUND FALSE)
    return()
endif()

include("${sdl3_sound_config_path}")
