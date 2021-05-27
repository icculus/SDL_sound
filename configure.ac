# Process this file with autoconf to produce a configure script.
AC_INIT
AC_CONFIG_SRCDIR([SDL_sound.c])
AC_CONFIG_MACRO_DIR([acinclude])
AC_CONFIG_AUX_DIR(build-scripts)

dnl ---------------------------------------------------------------------
dnl System/version info
dnl ---------------------------------------------------------------------

# Making releases:
#   MICRO_VERSION += 1;
#   INTERFACE_AGE += 1;
#   BINARY_AGE += 1;
# if any functions have been added, set INTERFACE_AGE to 0.
# if backwards compatibility has been broken,
# set BINARY_AGE and INTERFACE_AGE to 0.

MAJOR_VERSION=1
MINOR_VERSION=0
MICRO_VERSION=4
INTERFACE_AGE=3
BINARY_AGE=3
VERSION=$MAJOR_VERSION.$MINOR_VERSION.$MICRO_VERSION

AC_SUBST(MAJOR_VERSION)
AC_SUBST(MINOR_VERSION)
AC_SUBST(MICRO_VERSION)
AC_SUBST(INTERFACE_AGE)
AC_SUBST(BINARY_AGE)
AC_SUBST(VERSION)

# libtool versioning
LT_RELEASE=$MAJOR_VERSION.$MINOR_VERSION
LT_CURRENT=`expr $MICRO_VERSION - $INTERFACE_AGE`
LT_REVISION=$INTERFACE_AGE
LT_AGE=`expr $BINARY_AGE - $INTERFACE_AGE`

AC_SUBST(LT_RELEASE)
AC_SUBST(LT_CURRENT)
AC_SUBST(LT_REVISION)
AC_SUBST(LT_AGE)

dnl Detect the canonical host and target build environment
AC_CANONICAL_HOST
AC_CANONICAL_TARGET

dnl Setup for automake
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE(SDL_sound, $VERSION)

dnl Compilers and other tools
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LN_S
LT_INIT([win32-dll])
PKG_PROG_PKG_CONFIG


dnl ---------------------------------------------------------------------
dnl Debug mode?
dnl ---------------------------------------------------------------------
AC_ARG_ENABLE(debug,
[AS_HELP_STRING([--enable-debug], [enable debug mode [default=no]])],
              , enable_debug=no)
if test x$enable_debug = xyes; then
  if test x$ac_cv_prog_cc_g = xyes; then
    CFLAGS="-g -O0"
  else
    CFLAGS="-O0"
  fi
  AC_DEFINE(DEBUG, 1, [Define for debug builds.])
  AC_DEFINE(DEBUG_CHATTER, 1, [Define for debug build chattering.])
else
  AC_DEFINE(NDEBUG, 1, [Define to disable debugging.])
fi


dnl ---------------------------------------------------------------------
dnl Have GCC's -Wall option?
dnl ---------------------------------------------------------------------
saved_CFLAGS="$CFLAGS"
CFLAGS="$CFLAGS -Wall"
AC_MSG_CHECKING([if ${CC} supports -Wall])
AC_COMPILE_IFELSE([AC_LANG_SOURCE([[int foo;]])],
  [AC_MSG_RESULT([yes])],
  [AC_MSG_RESULT([no])
  CFLAGS="$saved_CFLAGS"])


dnl ---------------------------------------------------------------------
dnl Have GCC's -fvisibility option?
dnl ---------------------------------------------------------------------
saved_CFLAGS="$CFLAGS"
CFLAGS="$CFLAGS -fvisibility=hidden -Werror"
AC_MSG_CHECKING([if compiler supports visibility attributes])
AC_LANG_PUSH([C])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
__attribute__((visibility("default"))) int foo(void);
__attribute__((visibility("hidden")))  int bar(void);
int foo(void) { return 0; }
int bar(void) { return 1; }]], [])],
  [have_visibility=yes],[have_visibility=no])
AC_LANG_POP([C])
AC_MSG_RESULT([$have_visibility])
CFLAGS="$saved_CFLAGS"
if test $have_visibility = yes; then
    CFLAGS="$CFLAGS -fvisibility=hidden"
fi


dnl ---------------------------------------------------------------------
dnl Checks for libraries.
dnl ---------------------------------------------------------------------

dnl Check for SDL
AC_ARG_ENABLE(sdl2,
[AS_HELP_STRING([--enable-sdl2], [use SDL2 instead of SDL-1.2 [default=no]])],
              , enable_sdl2=no)

if test x$enable_sdl2 = xyes; then
    SDL_VERSION=2.0.0
    AM_PATH_SDL2($SDL_VERSION,,AC_MSG_ERROR([*** SDL version $SDL_VERSION not found!]))
else
    SDL_VERSION=1.2.4
    AM_PATH_SDL($SDL_VERSION,,AC_MSG_ERROR([*** SDL version $SDL_VERSION not found!]))
fi
CFLAGS="$CFLAGS $SDL_CFLAGS"
LIBS="$LIBS $SDL_LIBS"


dnl Check for voc decoder inclusion...
AC_ARG_ENABLE(voc,
[AS_HELP_STRING([--enable-voc], [enable VOC decoding [default=yes]])],
              , enable_voc=yes)
if test x$enable_voc = xyes; then
    AC_DEFINE(SOUND_SUPPORTS_VOC, 1, [Define if VOC support is desired.])
fi

dnl Check for wav decoder inclusion...
AC_ARG_ENABLE(wav,
[AS_HELP_STRING([--enable-wav], [enable WAV decoding [default=yes]])],
              , enable_wav=yes)
if test x$enable_wav = xyes; then
    AC_DEFINE(SOUND_SUPPORTS_WAV, 1, [Define if WAV support is desired.])
fi

dnl Check for raw decoder inclusion...
AC_ARG_ENABLE(raw,
[AS_HELP_STRING([--enable-raw], [enable raw audio "decoding" [default=yes]])],
              , enable_raw=yes)
if test x$enable_raw = xyes; then
    AC_DEFINE(SOUND_SUPPORTS_RAW, 1, [Define if RAW support is desired.])
fi

dnl Check for aiff decoder inclusion...
AC_ARG_ENABLE(aiff,
[AS_HELP_STRING([--enable-aiff], [enable AIFF decoding [default=yes]])],
              , enable_aiff=yes)
if test x$enable_aiff = xyes; then
    AC_DEFINE(SOUND_SUPPORTS_AIFF, 1, [Define if AIFF support is desired.])
fi

dnl Check for au decoder inclusion
AC_ARG_ENABLE(au,
[AS_HELP_STRING([--enable-au], [enable AU decoding [default=yes]])],
              , enable_au=yes)
if test x$enable_au = xyes; then
    AC_DEFINE(SOUND_SUPPORTS_AU, 1, [Define if AU support is desired.])
fi

dnl Check for shn decoder inclusion...
AC_ARG_ENABLE(shn,
[AS_HELP_STRING([--enable-shn], [enable SHN decoding [default=yes]])],
              , enable_shn=yes)
if test x$enable_shn = xyes; then
    AC_DEFINE(SOUND_SUPPORTS_SHN, 1, [Define if SHN support is desired.])
fi

dnl Check for the MIDI decoder...
AC_ARG_ENABLE(midi,
[AS_HELP_STRING([--enable-midi], [enable software MIDI music [default=yes]])],
              , enable_midi=yes)
if test x$enable_midi = xyes; then
  AC_DEFINE(SOUND_SUPPORTS_MIDI, 1, [Define if MIDI support is desired.])
fi

dnl Check for MPG123
AC_ARG_ENABLE(mpg123,
[AS_HELP_STRING([--enable-mpg123], [enable MP3 decoding via mpg123 [default=yes]])],
              , enable_mpg123=yes)
if test x$enable_mpg123 = xyes; then
  AC_CHECK_HEADER([mpg123.h], [have_mpg123_hdr=yes])
  AC_CHECK_LIB([mpg123], [mpg123_replace_reader_handle], [have_mpg123_lib=yes])
  if test x$have_mpg123_hdr = xyes -a x$have_mpg123_lib = xyes; then
    LIBS="$LIBS -lmpg123"
    AC_DEFINE(SOUND_SUPPORTS_MPG123, 1, [Define if MPG123 support is desired.])
  fi
fi

dnl Check for libmikmod
AC_ARG_ENABLE(mikmod,
[AS_HELP_STRING([--enable-mikmod], [enable MOD decoding via mikmod [default=yes]])],
              , enable_mikmod=yes)
if test x$enable_mikmod = xyes; then
  AC_CHECK_HEADER(mikmod.h, have_mikmod_hdr=yes)
  AC_CHECK_LIB(mikmod, MikMod_RegisterDriver, have_mikmod_lib=yes)
  if test x$have_mikmod_hdr = xyes -a x$have_mikmod_lib = xyes; then
    LIBS="$LIBS -lmikmod"
    AC_DEFINE(SOUND_SUPPORTS_MIKMOD, 1, [Define if MIKMOD support is desired.])
  fi
fi

dnl Check for libmodplug
AC_ARG_ENABLE(modplug,
[AS_HELP_STRING([--enable-modplug], [enable MOD decoding via modplug [default=yes]])],
              , enable_modplug=yes)
if test x$enable_modplug = xyes; then
  have_modplug_hdr=no
  AC_CHECK_HEADER(modplug.h, have_modplug_hdr=yes)
  if test x$have_modplug_hdr = xno; then
    AC_CHECK_HEADER(libmodplug/modplug.h, have_modplug_hdr=yes)
    if test x$have_modplug_hdr = xyes; then
      have_modplug_hdr=yes
      AC_DEFINE(SOUND_MODPLUG_IN_OWN_PATH, 1, [Define if modplug header is in own directory.])
    fi
  fi

  AC_CHECK_LIB(modplug, ModPlug_Load, have_modplug_lib=yes)
  if test x$have_modplug_hdr = xyes -a x$have_modplug_lib = xyes; then
    LIBS="$LIBS -lmodplug"
    AC_DEFINE(SOUND_SUPPORTS_MODPLUG, 1, [Define if MODPLUG support is desired.])
  fi
fi

AC_CHECK_HEADER(ogg/ogg.h, have_ogg_hdr=yes)
AC_CHECK_LIB(ogg, ogg_sync_init, have_ogg_lib=yes)
have_ogg=no
if test x$have_ogg_hdr = xyes -a x$have_ogg_lib = xyes; then
  have_ogg=yes
fi

dnl !!! FIXME: should be --enable-vorbis.
dnl Check for Ogg Vorbis
AC_ARG_ENABLE(ogg,
[AS_HELP_STRING([--enable-ogg], [enable OGG decoding via libvorbis [default=yes]])],
              , enable_ogg=yes)
if test x$enable_ogg = xyes; then
  AC_CHECK_HEADER(vorbis/codec.h, have_vorbis_hdr=yes)
  AC_CHECK_LIB(vorbis, vorbis_info_init, have_vorbis_lib=yes)
  AC_CHECK_HEADER(vorbis/vorbisfile.h, have_vorbisfile_hdr=yes)
  AC_CHECK_LIB(vorbisfile, ov_open_callbacks, have_vorbisfile_lib=yes)
  have_vorbis=no
  if test x$have_ogg = xyes; then
    if test x$have_vorbis_hdr = xyes -a x$have_vorbis_lib = xyes; then
      if test x$have_vorbisfile_hdr = xyes -a x$have_vorbisfile_lib = xyes; then
        have_vorbis=yes
      fi
    fi
  fi

  if test x$have_vorbis = xyes; then
    LIBS="$LIBS -logg -lvorbis -lvorbisfile"
    AC_DEFINE(SOUND_SUPPORTS_OGG, 1, [Define if OGG support is desired.])
  fi
fi

dnl Starting with FLAC 1.1.3:
dnl   libFLAC supports Ogg FLAC (no more libOggFLAC) so we also need -logg
dnl   the libFLAC .so version is also #defined in FLAC/export.h

dnl Hooray for shitty autoconf bugs!
x="C__stream_decoder_process_single"
flcsym="FLA$x"
dnl Check for libFLAC
AC_ARG_ENABLE(flac,
[AS_HELP_STRING([--enable-flac], [enable FLAC decoding via libFLAC [default=yes]])],
              , enable_flac=yes)
if test x$enable_flac = xyes; then
  AC_CHECK_HEADER(FLAC/stream_decoder.h, have_flac_hdr=yes)
  AC_CHECK_LIB(FLAC, $flcsym, have_flac_lib=yes)
  if test x$have_ogg = xyes; then
    if test x$have_flac_hdr = xyes -a x$have_flac_lib = xyes; then
      LIBS="$LIBS -lFLAC -logg"
      AC_DEFINE(SOUND_SUPPORTS_FLAC, 1, [Define if FLAC support is desired.])
    fi
  fi
fi

dnl Check for speex
AC_ARG_ENABLE(speex,
[AS_HELP_STRING([--enable-speex], [enable SPX decoding via libspeex [default=yes]])],
              , enable_speex=yes)
if test x$enable_speex = xyes; then
  AC_CHECK_HEADER(ogg/ogg.h, have_ogg_hdr=yes)
  AC_CHECK_LIB(ogg, ogg_sync_init, have_ogg_lib=yes)
  AC_CHECK_HEADER(speex/speex.h, have_speex_hdr=yes)
  AC_CHECK_LIB(speex, speex_bits_init, have_speex_lib=yes)
  if test "x$have_ogg_hdr" = "xyes" -a "x$have_ogg_lib" = "xyes" -a "x$have_speex_hdr" = "xyes" -a "x$have_speex_lib" = "xyes"; then
    LIBS="$LIBS -logg -lspeex"
    AC_DEFINE(SOUND_SUPPORTS_SPEEX, 1, [Define if SPEEX support is desired.])
  fi
fi

dnl Check whether the EXPERIMENTAL audio converter is preferred
AC_ARG_ENABLE(altcvt,
[AS_HELP_STRING([--enable-altcvt], [enable EXPERIMENTAL audio converter [default=no]])],
              , enable_altcvt=no)
if test x$enable_altcvt = xyes; then
  AC_DEFINE(SOUND_USE_ALTCVT, 1, [Define to use alternate audio converter.])
fi


# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([stdlib.h string.h signal.h assert.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_SIZE_T

# Checks for library functions.

AC_FUNC_MEMCMP
AC_CHECK_FUNCS([memset strrchr setbuf])

dnl Add Makefile conditionals
AM_CONDITIONAL(USE_TIMIDITY, test x$enable_midi = xyes)
AM_CONDITIONAL(USE_SDL2, test x$enable_sdl2 = xyes)

AC_CONFIG_FILES([
Makefile
decoders/Makefile
decoders/timidity/Makefile
examples/Makefile
])

dnl fix exports from MinGW builds
AC_CONFIG_COMMANDS_POST([sed -i 's/-DDLL_EXPORT/-DSDL_SOUND_DLL_EXPORTS/' config.status])
AC_OUTPUT
