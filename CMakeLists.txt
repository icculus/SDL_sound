# NOTE: If you installed SDL3 at an unusual place and find_package()
# fails, define SDL3_DIR environment variable to point to your SDL3
# installation.

set(MAJOR_VERSION 3)
set(MINOR_VERSION 0)
set(MICRO_VERSION 0)
set(SDLSOUND_VERSION "${MAJOR_VERSION}.${MINOR_VERSION}.${MICRO_VERSION}")

# Increment this if/when we break backwards compatibility.
set(SDLSOUND_SOVERSION 3)

cmake_minimum_required(VERSION 3.0...3.10)
cmake_policy(SET CMP0074 NEW)

project(SDL_sound
    VERSION ${SDLSOUND_VERSION}
    LANGUAGES C
)

include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

include_directories(src)

if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_C_COMPILER_ID MATCHES "Clang")
    add_compile_options(-Wall)
endif()

if(CMAKE_C_COMPILER_ID STREQUAL "SunPro")
    add_compile_options(-erroff=E_EMPTY_TRANSLATION_UNIT)
    add_compile_options(-xldscope=hidden)
endif()

find_package(SDL3 REQUIRED CONFIG REQUIRED COMPONENTS SDL3)

# Decoders ...
# These are (mostly) on by default now, so these options are only useful for
#  disabling them.

macro(sdlsound_decoder_option _VAR _NAME _EXTS _DEFLT)
    option(SDLSOUND_DECODER_${_VAR} "Enable ${_NAME} support (${_EXTS})" ${_DEFLT})
    if(NOT SDLSOUND_DECODER_${_VAR})
        add_definitions("-DSOUND_SUPPORTS_${_VAR}=0")
    endif()
endmacro()

sdlsound_decoder_option(WAV "Wave" ".WAV" TRUE)
sdlsound_decoder_option(AIFF "Audio Interchange" ".AIFF" TRUE)
sdlsound_decoder_option(AU "Sun/NeXT audio" ".AU" TRUE)
sdlsound_decoder_option(VOC "Creative Labs voice" ".VOC" TRUE)
sdlsound_decoder_option(FLAC "Free Lossless Audio Codec" ".FLAC" TRUE)
sdlsound_decoder_option(VORBIS "Ogg Vorbis" ".OGG" TRUE)
sdlsound_decoder_option(RAW "raw PCM audio" ".RAW" TRUE)
sdlsound_decoder_option(SHN "Shorten" ".SHN" TRUE)
sdlsound_decoder_option(MODPLUG "ModPlug" "various tracker formats" TRUE)
sdlsound_decoder_option(MP3 "MPEG-1 Layers I-III" ".MP3, .MP2, .MP1" TRUE)

# timidity is not public domain code, so default to not building it.
sdlsound_decoder_option(MIDI "Midi" ".MID" FALSE)

if(APPLE)
    sdlsound_decoder_option(COREAUDIO "CoreAudio" "various audio formats" TRUE)
    if(SDLSOUND_DECODER_COREAUDIO)
        set(OPTIONAL_LIBRARY_LIBS ${OPTIONAL_LIBRARY_LIBS} "-framework AudioToolbox")
        set(PC_LIBS ${PC_LIBS} "-framework AudioToolbox")
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
    set(TIMIDITY_HDRS
        src/timidity/common.h
        src/timidity/instrum.h
        src/timidity/mix.h
        src/timidity/options.h
        src/timidity/output.h
        src/timidity/playmidi.h
        src/timidity/readmidi.h
        src/timidity/resample.h
        src/timidity/tables.h
        src/timidity/timidity.h
    )
    source_group("src/timidity" FILES ${TIMIDITY_SRCS} ${TIMIDITY_HDRS})
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
        src/libmodplug/mmcmp.c
        src/libmodplug/modplug.c
        src/libmodplug/snd_dsp.c
        src/libmodplug/snd_flt.c
        src/libmodplug/snd_fx.c
        src/libmodplug/sndfile.c
        src/libmodplug/sndmix.c
    )
    set(LIBMODPLUG_HDRS
        src/libmodplug/libmodplug.h
        src/libmodplug/modplug.h
        src/libmodplug/tables.h
    )
    source_group("src/libmodplug" FILES ${LIBMODPLUG_SRCS} ${LIBMODPLUG_HDRS})
endif()
if(WIN32)
    set(SDLSOUND_SHARED_SRCS
        src/version.rc
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
)
set(SDLSOUND_HDRS
    src/dr_flac.h
    src/dr_mp3.h
    "include/SDL3_sound/SDL_sound.h"
    src/SDL_sound_internal.h
    src/stb_vorbis.h
)
source_group("src" FILES ${SDLSOUND_SRCS} ${SDLSOUND_HDRS} ${SDLSOUND_SHARED_SRCS})

set(SDLSOUND_ALL_SRCS
    ${SDLSOUND_SRCS}
    ${SDLSOUND_HDRS}
    ${TIMIDITY_SRCS}
    ${TIMIDITY_HDRS}
    ${LIBMODPLUG_SRCS}
    ${LIBMODPLUG_HDRS}
)

option(SDLSOUND_BUILD_STATIC "Build static library" TRUE)
if(SDLSOUND_BUILD_STATIC)
    add_library(SDL3_sound-static STATIC ${SDLSOUND_ALL_SRCS})
    add_library(SDL3_sound::SDL3_sound-static ALIAS SDL3_sound-static)
    # Don't rename this on Windows, since DLLs will also produce an import
    #  library named "SDL3_sound.lib" which would conflict; Unix tend to like
    #  the same library name with a different extension for static libs, but
    #  Windows can just have a separate name.
    if(NOT MSVC)
        set_target_properties(SDL3_sound-static PROPERTIES OUTPUT_NAME "SDL3_sound")
    endif()
    target_include_directories(SDL3_sound-static PUBLIC
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
    target_include_directories(SDL3_sound-static PUBLIC "$<BUILD_INTERFACE:${SDL3_INCLUDE_DIRS}>")
    target_link_libraries(SDL3_sound-static PRIVATE SDL3::SDL3 ${OPTIONAL_LIBRARY_LIBS} ${OTHER_LDFLAGS})
    set(SDLSOUND_LIB_TARGET SDL3_sound-static)
endif()

option(SDLSOUND_BUILD_SHARED "Build shared library" TRUE)
if(SDLSOUND_BUILD_SHARED)
    add_library(SDL3_sound SHARED ${SDLSOUND_ALL_SRCS} ${SDLSOUND_SHARED_SRCS})
    add_library(SDL3_sound::SDL3_sound ALIAS SDL3_sound)
    set_target_properties(SDL3_sound PROPERTIES MACOSX_RPATH 1)
    set_target_properties(SDL3_sound PROPERTIES VERSION ${SDLSOUND_VERSION})
    set_target_properties(SDL3_sound PROPERTIES SOVERSION ${SDLSOUND_SOVERSION})
    # Use `Compatible Interface Properties` to ensure a shared SDL_sound is linked to a shared SDL library
    set_property(TARGET SDL3_sound PROPERTY INTERFACE_SDL3_SHARED ON)
    set_property(TARGET SDL3_sound APPEND PROPERTY COMPATIBLE_INTERFACE_BOOL SDL3_SHARED)
    if(WIN32)
        SET_TARGET_PROPERTIES(SDL3_sound PROPERTIES DEFINE_SYMBOL SDL_SOUND_DLL_EXPORTS)
        # avoid DLL having "lib" prefix
        SET(CMAKE_SHARED_LIBRARY_PREFIX "")
    endif()
    target_include_directories(SDL3_sound PUBLIC
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    )
    # target_include_directories(SDL3_sound PUBLIC "$<BUILD_INTERFACE:${SDL3_INCLUDE_DIRS}>")
    target_link_libraries(SDL3_sound PRIVATE SDL3::SDL3 ${OPTIONAL_LIBRARY_LIBS} ${OTHER_LDFLAGS})
    set(SDLSOUND_LIB_TARGET SDL3_sound)
endif()

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/cmake/sdl3-sound.pc.in" "${CMAKE_CURRENT_BINARY_DIR}/sdl3-sound.pc" @ONLY)

if(NOT SDLSOUND_BUILD_SHARED AND NOT SDLSOUND_BUILD_STATIC)
    message(FATAL "Both shared and static libraries are disabled!")
endif()

option(SDLSOUND_BUILD_TEST "Build stdio test program." TRUE)
mark_as_advanced(SDLSOUND_BUILD_TEST)
if(SDLSOUND_BUILD_TEST)
    add_executable(playsound examples/playsound.c)
    target_link_libraries(playsound PRIVATE SDL3::SDL3)
    if(TARGET SDL3::SDL3)
        target_link_libraries(playsound PRIVATE SDL3::SDL3)
    else()
        target_include_directories(playsound PRIVATE ${SDL3_INCLUDE_DIRS})
        target_link_libraries(playsound PRIVATE ${SDL3_LIBRARIES})
    endif()
    target_link_libraries(playsound PRIVATE ${SDLSOUND_LIB_TARGET} ${OTHER_LDFLAGS})
    # assuming assert.h, signal.h and setbuf() being available universally
    target_compile_definitions(playsound PRIVATE "HAVE_SETBUF")
    target_compile_definitions(playsound PRIVATE "HAVE_ASSERT_H")
    target_compile_definitions(playsound PRIVATE "HAVE_SIGNAL_H")

    list(APPEND SDLSOUND_INSTALL_TARGETS playsound)
    add_executable(playsound_simple examples/playsound_simple.c)
    target_link_libraries(playsound_simple PRIVATE SDL3::SDL3)
    if(TARGET SDL3::SDL3)
        target_link_libraries(playsound_simple PRIVATE SDL3::SDL3)
    else()
        target_include_directories(playsound_simple PRIVATE ${SDL3_INCLUDE_DIRS})
        target_link_libraries(playsound_simple PRIVATE ${SDL3_LIBRARIES})
    endif()
    target_link_libraries(playsound_simple PRIVATE ${SDLSOUND_LIB_TARGET} ${OTHER_LDFLAGS})
    #list(APPEND SDLSOUND_INSTALL_TARGETS playsound_simple)
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
        target_link_libraries(playsound ${SDL3_LIBRARIES} ${OPTIONAL_LIBRARY_LIBS} ${OTHER_LDFLAGS})
        target_link_libraries(playsound_simple ${SDL3_LIBRARIES} ${OPTIONAL_LIBRARY_LIBS} ${OTHER_LDFLAGS})
    endif()
endif()

set(PKGCONFIG_INSTALLDIR "${CMAKE_INSTALL_LIBDIR}/pkgconfig")

if(WIN32 AND NOT MINGW)
    set(SDLSOUND_INSTALL_CMAKEDIR_DEFAULT "cmake")
else()
    set(SDLSOUND_INSTALL_CMAKEDIR_DEFAULT "${CMAKE_INSTALL_LIBDIR}/cmake/SDL3_sound")
endif()
set(SDLSOUND_INSTALL_CMAKEDIR "${SDLSOUND_INSTALL_CMAKEDIR_DEFAULT}" CACHE STRING "Location where to install SDL3_sound.cmake")

configure_package_config_file(cmake/SDL3_soundConfig.cmake.in SDL3_soundConfig.cmake
    INSTALL_DESTINATION "${SDLSOUND_INSTALL_CMAKEDIR}"
)
write_basic_package_version_file("${CMAKE_CURRENT_BINARY_DIR}/SDL3_soundConfigVersion.cmake"
    VERSION "${SDLSOUND_VERSION}"
    COMPATIBILITY AnyNewerVersion
)
if(SDLSOUND_BUILD_SHARED)
    install(TARGETS SDL3_sound
        EXPORT SDLSoundSharedExports
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT library
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT devel
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT library
    )
    install(EXPORT SDLSoundSharedExports
        FILE "${SDLSOUND_LIB_TARGET}-shared-targets.cmake"
        NAMESPACE SDL3_sound::
        DESTINATION "${SDLSOUND_INSTALL_CMAKEDIR}"
    )
endif()
if(SDLSOUND_BUILD_STATIC)
    install(TARGETS SDL3_sound-static
        EXPORT SDLSoundStaticExports
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT library
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT devel
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT library
    )
    install(EXPORT SDLSoundStaticExports
        FILE "${SDLSOUND_LIB_TARGET}-static-targets.cmake"
        NAMESPACE SDL3_sound::
        DESTINATION "${SDLSOUND_INSTALL_CMAKEDIR}"
    )
endif()
install(TARGETS ${SDLSOUND_INSTALL_TARGETS}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}/SDL3_soundConfig.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/SDL3_soundConfigVersion.cmake"
    DESTINATION "${SDLSOUND_INSTALL_CMAKEDIR}"
)
install(
    FILES "${CMAKE_CURRENT_SOURCE_DIR}/include/SDL3_sound/SDL_sound.h"
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/SDL3_sound" COMPONENT DEVEL
)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/sdl3-sound.pc
    DESTINATION "${PKGCONFIG_INSTALLDIR}" COMPONENT DEVEL)

option(SDLSOUND_BUILD_DOCS "Build documentation" TRUE)
mark_as_advanced(SDLSOUND_BUILD_DOCS)

if(SDLSOUND_BUILD_DOCS)
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
            SDL3_sound-docs
            ${DOXYGEN_EXECUTABLE} "${SDLSOUND_OUTPUT_DOXYFILE}"
            WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
            COMMENT "Building documentation..."
        )
    else()
        message(STATUS "Doxygen not found. You won't be able to build documentation.")
    endif()
endif()

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
    IMMEDIATE @ONLY)

add_custom_target(
    SDL3_sound-uninstall
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    COMMENT "Uninstall the project..."
)

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
message_bool_option("MP3 support" SDLSOUND_DECODER_MP3)
message_bool_option("TiMidity support" SDLSOUND_DECODER_MIDI)
message_bool_option("COREAUDIO support" SDLSOUND_DECODER_COREAUDIO)
message_bool_option("Build static library" SDLSOUND_BUILD_STATIC)
message_bool_option("Build shared library" SDLSOUND_BUILD_SHARED)
message_bool_option("Build stdio test program" SDLSOUND_BUILD_TEST)
message_bool_option("Build documentation" SDLSOUND_BUILD_DOCS)

# Make sure SDL3_sound::SDL3_sound always exists
if(TARGET SDL3_sound::SDL3_sound-static AND NOT TARGET SDL3_sound::SDL3_sound)
    add_library(SDL3_sound::SDL3_sound ALIAS SDL3_sound-static)
endif()

# end of CMakeLists.txt
