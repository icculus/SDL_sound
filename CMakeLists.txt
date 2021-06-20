# SDL_sound; An abstract sound format decoding API.
#
# Please see the file LICENSE.txt in the source's root directory.

cmake_minimum_required(VERSION 2.8.12)

project(SDL_sound)
set(SDLSOUND_VERSION 2.0.0)

# Increment this if/when we break backwards compatibility.
set(SDLSOUND_SOVERSION 2)

# I hate that they define "WIN32" ... we're about to move to Win64...I hope!
if(WIN32 AND NOT WINDOWS)
    set(WINDOWS TRUE)
endif()

include_directories(src)

if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_C_COMPILER_ID MATCHES "Clang")
    add_definitions(-std=gnu99)
    add_definitions(-Wall)
    # Don't use -rpath.
    set(CMAKE_SKIP_RPATH ON CACHE BOOL "Skip RPATH" FORCE)
endif()

if(CMAKE_C_COMPILER_ID STREQUAL "SunPro")
    add_definitions(-erroff=E_EMPTY_TRANSLATION_UNIT)
    add_definitions(-xldscope=hidden)
endif()

find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})
if(WIN32)
    # -lmingw32: gcc adds it automatically.
    # -mwindows: doesn't make sense.
    # -lSDL2main: the dll doesn't need it, the test apps are console-only
    #  and specifically define SDL_MAIN_HANDLED.  it will be ignored.
    string(REGEX REPLACE "-mwindows" "" SDL2_LIBRARIES "${SDL2_LIBRARIES}")
    string(REGEX REPLACE "-lmingw32" "" SDL2_LIBRARIES "${SDL2_LIBRARIES}")
#   string(REGEX REPLACE "-lSDL2main" "" SDL2_LIBRARIES "${SDL2_LIBRARIES}")
    string(STRIP "${SDL2_LIBRARIES}" SDL2_LIBRARIES)
endif()

# Decoders ...
# These are (mostly) on by default now, so these options are only useful for
#  disabling them.

macro(sdlsound_decoder_option _VAR _NAME _EXTS)
    option(SDLSOUND_DECODER_${_VAR} "Enable ${_NAME} support (${_EXTS})" TRUE)
    if(NOT SDLSOUND_DECODER_${_VAR})
        add_definitions("-DSOUND_SUPPORTS_${_VAR}=0")
    endif()
endmacro()

sdlsound_decoder_option(WAV "Wave" ".WAV")
sdlsound_decoder_option(AIFF "Audio Interchange" ".AIFF")
sdlsound_decoder_option(AU "Sun/NeXT audio" ".AU")
sdlsound_decoder_option(VOC "Creative Labs voice" ".VOC")
sdlsound_decoder_option(FLAC "Free Lossless Audio Codec" ".FLAC")
sdlsound_decoder_option(VORBIS "Ogg Vorbis" ".OGG")
sdlsound_decoder_option(RAW "raw PCM audio" ".RAW")
sdlsound_decoder_option(SHN "Shorten" ".SHN")
sdlsound_decoder_option(MIDI "Midi" ".MID")
sdlsound_decoder_option(MODPLUG "ModPlug" "various tracker formats")
sdlsound_decoder_option(MP3 "MPEG-1 Layers I-III" ".MP3, .MP2, .MP1")

if(APPLE)
    sdlsound_decoder_option(COREAUDIO "CoreAudio" "various audio formats")
    if(SDLSOUND_DECODER_COREAUDIO)
        set(OPTIONAL_LIBRARY_LIBS ${OPTIONAL_LIBRARY_LIBS} "-framework AudioToolbox")
    endif()
endif()

if(SDLSOUND_DECODER_MIDI)
    set(TIMIDITY_SRCS
        src/timidity/common.c
        src/timidity/instrum.c
        src/timidity/mix.c
        src/timidity/output.c
        src/timidity/playmidi.c
        src/timidity/readmidi.c
        src/timidity/resample.c
        src/timidity/tables.c
        src/timidity/timidity.c
    )
endif()
if(SDLSOUND_DECODER_MODPLUG)
    set(LIBMODPLUG_SRCS
        src/libmodplug/fastmix.c
        src/libmodplug/load_669.c
        src/libmodplug/load_amf.c
        src/libmodplug/load_ams.c
        src/libmodplug/load_dbm.c
        src/libmodplug/load_dmf.c
        src/libmodplug/load_dsm.c
        src/libmodplug/load_far.c
        src/libmodplug/load_it.c
        src/libmodplug/load_mdl.c
        src/libmodplug/load_med.c
        src/libmodplug/load_mod.c
        src/libmodplug/load_mt2.c
        src/libmodplug/load_mtm.c
        src/libmodplug/load_okt.c
        src/libmodplug/load_gdm.c
        src/libmodplug/load_psm.c
        src/libmodplug/load_ptm.c
        src/libmodplug/load_s3m.c
        src/libmodplug/load_stm.c
        src/libmodplug/load_ult.c
        src/libmodplug/load_umx.c
        src/libmodplug/load_xm.c
        src/libmodplug/modplug.c
        src/libmodplug/snd_dsp.c
        src/libmodplug/snd_flt.c
        src/libmodplug/snd_fx.c
        src/libmodplug/sndfile.c
        src/libmodplug/sndmix.c
    )
endif()

# Almost everything is "compiled" here, but things that don't apply to the
#  build are #ifdef'd out. This is to make it easy to embed SDL_sound into
#  another project or bring up a new build system: just compile all the source
#  code and #define the things you want.
set(SDLSOUND_SRCS
    src/SDL_sound.c
    src/SDL_sound_aiff.c
    src/SDL_sound_au.c
    src/SDL_sound_coreaudio.c
    src/SDL_sound_flac.c
    src/SDL_sound_midi.c
    src/SDL_sound_modplug.c
    src/SDL_sound_mp3.c
    src/SDL_sound_raw.c
    src/SDL_sound_shn.c
    src/SDL_sound_voc.c
    src/SDL_sound_vorbis.c
    src/SDL_sound_wav.c
    ${TIMIDITY_SRCS}
    ${LIBMODPLUG_SRCS}
)

option(SDLSOUND_BUILD_STATIC "Build static library" TRUE)
if(SDLSOUND_BUILD_STATIC)
    add_library(SDL2_sound-static STATIC ${SDLSOUND_SRCS})
    # Don't rename this on Windows, since DLLs will also produce an import
    #  library named "SDL2_sound.lib" which would conflict; Unix tend to like
    #  the same library name with a different extension for static libs, but
    #  Windows can just have a separate name.
    if(NOT MSVC)
        set_target_properties(SDL2_sound-static PROPERTIES OUTPUT_NAME "SDL2_sound")
    endif()
    set(SDLSOUND_LIB_TARGET SDL2_sound-static)
    set(SDLSOUND_INSTALL_TARGETS ${SDLSOUND_INSTALL_TARGETS} ";SDL2_sound-static")
endif()

option(SDLSOUND_BUILD_SHARED "Build shared library" TRUE)
if(SDLSOUND_BUILD_SHARED)
    add_library(SDL2_sound SHARED ${SDLSOUND_SRCS})
    set_target_properties(SDL2_sound PROPERTIES MACOSX_RPATH 1)
    set_target_properties(SDL2_sound PROPERTIES VERSION ${SDLSOUND_VERSION})
    set_target_properties(SDL2_sound PROPERTIES SOVERSION ${SDLSOUND_SOVERSION})
    if(WIN32)
        SET_TARGET_PROPERTIES(SDL2_sound PROPERTIES DEFINE_SYMBOL SDL_SOUND_DLL_EXPORTS)
        # avoid DLL having "lib" prefix
        SET(CMAKE_SHARED_LIBRARY_PREFIX "")
    endif()
    target_link_libraries(SDL2_sound ${SDL2_LIBRARIES} ${OPTIONAL_LIBRARY_LIBS} ${OTHER_LDFLAGS})
    set(SDLSOUND_LIB_TARGET SDL2_sound)
    set(SDLSOUND_INSTALL_TARGETS ${SDLSOUND_INSTALL_TARGETS} ";SDL2_sound")
endif()

if(NOT SDLSOUND_BUILD_SHARED AND NOT SDLSOUND_BUILD_STATIC)
    message(FATAL "Both shared and static libraries are disabled!")
endif()

# CMake FAQ says I need this...
if(SDLSOUND_BUILD_SHARED AND SDLSOUND_BUILD_STATIC AND NOT WINDOWS)
    set_target_properties(SDL2_sound PROPERTIES CLEAN_DIRECT_OUTPUT 1)
    set_target_properties(SDL2_sound-static PROPERTIES CLEAN_DIRECT_OUTPUT 1)
endif()

option(SDLSOUND_BUILD_TEST "Build stdio test program." TRUE)
mark_as_advanced(SDLSOUND_BUILD_TEST)
if(SDLSOUND_BUILD_TEST)
    add_executable(playsound examples/playsound.c)
    target_link_libraries(playsound ${SDLSOUND_LIB_TARGET} ${OTHER_LDFLAGS})
    # assuming assert.h, signal.h and setbuf() being available universally
    target_compile_definitions(playsound PRIVATE "HAVE_SETBUF")
    target_compile_definitions(playsound PRIVATE "HAVE_ASSERT_H")
    target_compile_definitions(playsound PRIVATE "HAVE_SIGNAL_H")

    set(SDLSOUND_INSTALL_TARGETS ${SDLSOUND_INSTALL_TARGETS} ";playsound")
    add_executable(playsound_simple examples/playsound_simple.c)
    target_link_libraries(playsound_simple ${SDLSOUND_LIB_TARGET} ${OTHER_LDFLAGS})
    #set(SDLSOUND_INSTALL_TARGETS ${SDLSOUND_INSTALL_TARGETS} ";playsound_simple")
    IF (WIN32 AND MSVC)
        SET_TARGET_PROPERTIES(playsound PROPERTIES LINK_FLAGS "/SUBSYSTEM:CONSOLE")
        SET_TARGET_PROPERTIES(playsound_simple PROPERTIES LINK_FLAGS "/SUBSYSTEM:CONSOLE")
        SET_TARGET_PROPERTIES(playsound PROPERTIES COMPILE_DEFINITIONS _CONSOLE)
        SET_TARGET_PROPERTIES(playsound_simple PROPERTIES COMPILE_DEFINITIONS _CONSOLE)
    ENDIF ()
    IF (CMAKE_COMPILER_IS_MINGW)
        SET_TARGET_PROPERTIES(playsound PROPERTIES LINK_FLAGS "-mconsole")
        SET_TARGET_PROPERTIES(playsound_simple PROPERTIES LINK_FLAGS "-mconsole")
    ENDIF ()
    if(NOT SDLSOUND_BUILD_SHARED)
        target_link_libraries(playsound ${SDL2_LIBRARIES} ${OPTIONAL_LIBRARY_LIBS} ${OTHER_LDFLAGS})
        target_link_libraries(playsound_simple ${SDL2_LIBRARIES} ${OPTIONAL_LIBRARY_LIBS} ${OTHER_LDFLAGS})
    endif()
endif()

install(TARGETS ${SDLSOUND_INSTALL_TARGETS}
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib${LIB_SUFFIX}
        ARCHIVE DESTINATION lib${LIB_SUFFIX})
install(FILES src/SDL_sound.h DESTINATION include/SDL2)

find_package(Doxygen)
if(DOXYGEN_FOUND)
    set(SDLSOUND_OUTPUT_DOXYFILE "${CMAKE_CURRENT_BINARY_DIR}/Doxyfile")
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/docs/Doxyfile"
        "${SDLSOUND_OUTPUT_DOXYFILE}"
        COPYONLY
    )
    file(APPEND "${SDLSOUND_OUTPUT_DOXYFILE}" "\n\n# Below auto-generated by cmake...\n\n")
    file(APPEND "${SDLSOUND_OUTPUT_DOXYFILE}" "PROJECT_NUMBER = \"${SDLSOUND_VERSION}\"\n")
    file(APPEND "${SDLSOUND_OUTPUT_DOXYFILE}" "OUTPUT_DIRECTORY = \"${CMAKE_CURRENT_BINARY_DIR}/docs\"\n")
    file(APPEND "${SDLSOUND_OUTPUT_DOXYFILE}" "\n# End auto-generated section.\n\n")

    add_custom_target(
        SDL2_sound-docs
        ${DOXYGEN_EXECUTABLE} "${SDLSOUND_OUTPUT_DOXYFILE}"
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        COMMENT "Building documentation..."
    )
else()
    message(STATUS "Doxygen not found. You won't be able to build documentation.")
endif()

if(UNIX)
    set(SDLSOUND_TARBALL "${CMAKE_CURRENT_SOURCE_DIR}/../SDL_sound-${SDLSOUND_VERSION}.tar.bz2")
    add_custom_target(
        SDL2_sound-dist
        hg archive -t tbz2 "${SDLSOUND_TARBALL}"
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        COMMENT "Building source tarball '${SDLSOUND_TARBALL}'..."
    )
    add_custom_target(
        SDL2_sound-uninstall
        "${CMAKE_CURRENT_SOURCE_DIR}/docs/uninstall.sh"
        WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
        COMMENT "Uninstall the project..."
    )
endif()

macro(message_bool_option _NAME _VALUE)
    if(${_VALUE})
        message(STATUS "  ${_NAME}: enabled")
    else()
        message(STATUS "  ${_NAME}: disabled")
    endif()
endmacro()

message(STATUS "SDL_sound will build with the following options:")
message_bool_option("WAV support" SDLSOUND_DECODER_WAV)
message_bool_option("AIFF support" SDLSOUND_DECODER_AIFF)
message_bool_option("AU support" SDLSOUND_DECODER_AU)
message_bool_option("VOC support" SDLSOUND_DECODER_VOC)
message_bool_option("FLAC support" SDLSOUND_DECODER_FLAC)
message_bool_option("VORBIS support" SDLSOUND_DECODER_VORBIS)
message_bool_option("RAW support" SDLSOUND_DECODER_RAW)
message_bool_option("SHN support" SDLSOUND_DECODER_SHN)
message_bool_option("MODPLUG support" SDLSOUND_DECODER_MODPLUG)
message_bool_option("TiMidity support" SDLSOUND_DECODER_MIDI)
message_bool_option("MP3 support" SDLSOUND_DECODER_MP3)
message_bool_option("COREAUDIO support" SDLSOUND_DECODER_COREAUDIO)
message_bool_option("Build static library" SDLSOUND_BUILD_STATIC)
message_bool_option("Build shared library" SDLSOUND_BUILD_SHARED)
message_bool_option("Build stdio test program" SDLSOUND_BUILD_TEST)

# end of CMakeLists.txt
