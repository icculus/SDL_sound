# Configure paths for SDL
# Sam Lantinga 9/21/99
# stolen from Manish Singh
# stolen back from Frank Belew
# stolen from Manish Singh
# Shamelessly stolen from Owen Taylor

dnl AM_PATH_SDL([MINIMUM-VERSION, [ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]]])
dnl Test for SDL, and define SDL_CFLAGS and SDL_LIBS
dnl
AC_DEFUN(AM_PATH_SDL,
[dnl 
dnl Get the cflags and libraries from the sdl-config script
dnl
AC_ARG_WITH(sdl-prefix,[  --with-sdl-prefix=PFX   Prefix where SDL is installed (optional)],
            sdl_prefix="$withval", sdl_prefix="")
AC_ARG_WITH(sdl-exec-prefix,[  --with-sdl-exec-prefix=PFX Exec prefix where SDL is installed (optional)],
            sdl_exec_prefix="$withval", sdl_exec_prefix="")
AC_ARG_ENABLE(sdltest, [  --disable-sdltest       Do not try to compile and run a test SDL program],
		    , enable_sdltest=yes)

  if test x$sdl_exec_prefix != x ; then
     sdl_args="$sdl_args --exec-prefix=$sdl_exec_prefix"
     if test x${SDL_CONFIG+set} != xset ; then
        SDL_CONFIG=$sdl_exec_prefix/bin/sdl-config
     fi
  fi
  if test x$sdl_prefix != x ; then
     sdl_args="$sdl_args --prefix=$sdl_prefix"
     if test x${SDL_CONFIG+set} != xset ; then
        SDL_CONFIG=$sdl_prefix/bin/sdl-config
     fi
  fi

  AC_PATH_PROG(SDL_CONFIG, sdl-config, no)
  min_sdl_version=ifelse([$1], ,0.11.0,$1)
  AC_MSG_CHECKING(for SDL - version >= $min_sdl_version)
  no_sdl=""
  if test "$SDL_CONFIG" = "no" ; then
    no_sdl=yes
  else
    SDL_CFLAGS=`$SDL_CONFIG $sdlconf_args --cflags`
    SDL_LIBS=`$SDL_CONFIG $sdlconf_args --libs`

    sdl_major_version=`$SDL_CONFIG $sdl_args --version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\1/'`
    sdl_minor_version=`$SDL_CONFIG $sdl_args --version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\2/'`
    sdl_micro_version=`$SDL_CONFIG $sdl_config_args --version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\3/'`
    if test "x$enable_sdltest" = "xyes" ; then
      ac_save_CFLAGS="$CFLAGS"
      ac_save_LIBS="$LIBS"
      CFLAGS="$CFLAGS $SDL_CFLAGS"
      LIBS="$LIBS $SDL_LIBS"
dnl
dnl Now check if the installed SDL is sufficiently new. (Also sanity
dnl checks the results of sdl-config to some extent
dnl
      rm -f conf.sdltest
      AC_TRY_RUN([
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "SDL.h"

char*
my_strdup (char *str)
{
  char *new_str;
  
  if (str)
    {
      new_str = (char *)malloc ((strlen (str) + 1) * sizeof(char));
      strcpy (new_str, str);
    }
  else
    new_str = NULL;
  
  return new_str;
}

int main (int argc, char *argv[])
{
  int major, minor, micro;
  char *tmp_version;

  /* This hangs on some systems (?)
  system ("touch conf.sdltest");
  */
  { FILE *fp = fopen("conf.sdltest", "a"); if ( fp ) fclose(fp); }

  /* HP/UX 9 (%@#!) writes to sscanf strings */
  tmp_version = my_strdup("$min_sdl_version");
  if (sscanf(tmp_version, "%d.%d.%d", &major, &minor, &micro) != 3) {
     printf("%s, bad version string\n", "$min_sdl_version");
     exit(1);
   }

   if (($sdl_major_version > major) ||
      (($sdl_major_version == major) && ($sdl_minor_version > minor)) ||
      (($sdl_major_version == major) && ($sdl_minor_version == minor) && ($sdl_micro_version >= micro)))
    {
      return 0;
    }
  else
    {
      printf("\n*** 'sdl-config --version' returned %d.%d.%d, but the minimum version\n", $sdl_major_version, $sdl_minor_version, $sdl_micro_version);
      printf("*** of SDL required is %d.%d.%d. If sdl-config is correct, then it is\n", major, minor, micro);
      printf("*** best to upgrade to the required version.\n");
      printf("*** If sdl-config was wrong, set the environment variable SDL_CONFIG\n");
      printf("*** to point to the correct copy of sdl-config, and remove the file\n");
      printf("*** config.cache before re-running configure\n");
      return 1;
    }
}

],, no_sdl=yes,[echo $ac_n "cross compiling; assumed OK... $ac_c"])
       CFLAGS="$ac_save_CFLAGS"
       LIBS="$ac_save_LIBS"
     fi
  fi
  if test "x$no_sdl" = x ; then
     AC_MSG_RESULT(yes)
     ifelse([$2], , :, [$2])     
  else
     AC_MSG_RESULT(no)
     if test "$SDL_CONFIG" = "no" ; then
       echo "*** The sdl-config script installed by SDL could not be found"
       echo "*** If SDL was installed in PREFIX, make sure PREFIX/bin is in"
       echo "*** your path, or set the SDL_CONFIG environment variable to the"
       echo "*** full path to sdl-config."
     else
       if test -f conf.sdltest ; then
        :
       else
          echo "*** Could not run SDL test program, checking why..."
          CFLAGS="$CFLAGS $SDL_CFLAGS"
          LIBS="$LIBS $SDL_LIBS"
          AC_TRY_LINK([
#include <stdio.h>
#include "SDL.h"

int main(int argc, char *argv[])
{ return 0; }
#undef  main
#define main K_and_R_C_main
],      [ return 0; ],
        [ echo "*** The test program compiled, but did not run. This usually means"
          echo "*** that the run-time linker is not finding SDL or finding the wrong"
          echo "*** version of SDL. If it is not finding SDL, you'll need to set your"
          echo "*** LD_LIBRARY_PATH environment variable, or edit /etc/ld.so.conf to point"
          echo "*** to the installed location  Also, make sure you have run ldconfig if that"
          echo "*** is required on your system"
	  echo "***"
          echo "*** If you have an old version installed, it is best to remove it, although"
          echo "*** you may also be able to get things to work by modifying LD_LIBRARY_PATH"],
        [ echo "*** The test program failed to compile or link. See the file config.log for the"
          echo "*** exact error that occured. This usually means SDL was incorrectly installed"
          echo "*** or that you have moved SDL since it was installed. In the latter case, you"
          echo "*** may want to edit the sdl-config script: $SDL_CONFIG" ])
          CFLAGS="$ac_save_CFLAGS"
          LIBS="$ac_save_LIBS"
       fi
     fi
     SDL_CFLAGS=""
     SDL_LIBS=""
     ifelse([$3], , :, [$3])
  fi
  AC_SUBST(SDL_CFLAGS)
  AC_SUBST(SDL_LIBS)
  rm -f conf.sdltest
])
# Configure paths for libmikmod
#
# Derived from glib.m4 (Owen Taylor 97-11-3)
# Improved by Chris Butler
#

dnl AM_PATH_LIBMIKMOD([MINIMUM-VERSION, [ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND ]]])
dnl Test for libmikmod, and define LIBMIKMOD_CFLAGS, LIBMIKMOD_LIBS and
dnl LIBMIKMOD_LDADD
dnl
AC_DEFUN(AM_PATH_LIBMIKMOD,
[dnl 
dnl Get the cflags and libraries from the libmikmod-config script
dnl
AC_ARG_WITH(libmikmod-prefix,[  --with-libmikmod-prefix=PFX   Prefix where libmikmod is installed (optional)],
            libmikmod_config_prefix="$withval", libmikmod_config_prefix="")
AC_ARG_WITH(libmikmod-exec-prefix,[  --with-libmikmod-exec-prefix=PFX Exec prefix where libmikmod is installed (optional)],
            libmikmod_config_exec_prefix="$withval", libmikmod_config_exec_prefix="")
AC_ARG_ENABLE(libmikmodtest, [  --disable-libmikmodtest       Do not try to compile and run a test libmikmod program],
		    , enable_libmikmodtest=yes)

  if test x$libmikmod_config_exec_prefix != x ; then
     libmikmod_config_args="$libmikmod_config_args --exec-prefix=$libmikmod_config_exec_prefix"
     if test x${LIBMIKMOD_CONFIG+set} != xset ; then
        LIBMIKMOD_CONFIG=$libmikmod_config_exec_prefix/bin/libmikmod-config
     fi
  fi
  if test x$libmikmod_config_prefix != x ; then
     libmikmod_config_args="$libmikmod_config_args --prefix=$libmikmod_config_prefix"
     if test x${LIBMIKMOD_CONFIG+set} != xset ; then
        LIBMIKMOD_CONFIG=$libmikmod_config_prefix/bin/libmikmod-config
     fi
  fi

  AC_PATH_PROG(LIBMIKMOD_CONFIG, libmikmod-config, no)
  min_libmikmod_version=ifelse([$1], ,3.1.5,$1)
  AC_MSG_CHECKING(for libmikmod - version >= $min_libmikmod_version)
  no_libmikmod=""
  if test "$LIBMIKMOD_CONFIG" = "no" ; then
    no_libmikmod=yes
  else
    LIBMIKMOD_CFLAGS=`$LIBMIKMOD_CONFIG $libmikmod_config_args --cflags`
    LIBMIKMOD_LIBS=`$LIBMIKMOD_CONFIG $libmikmod_config_args --libs`
    LIBMIKMOD_LDADD=`$LIBMIKMOD_CONFIG $libmikmod_config_args --ldadd`
    libmikmod_config_major_version=`$LIBMIKMOD_CONFIG $libmikmod_config_args --version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\).*/\1/'`
    libmikmod_config_minor_version=`$LIBMIKMOD_CONFIG $libmikmod_config_args --version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\).*/\2/'`
    libmikmod_config_micro_version=`$LIBMIKMOD_CONFIG $libmikmod_config_args --version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\).*/\3/'`
    if test "x$enable_libmikmodtest" = "xyes" ; then
      ac_save_CFLAGS="$CFLAGS"
      ac_save_LIBS="$LIBS"
	  AC_LANG_SAVE
	  AC_LANG_C
      CFLAGS="$CFLAGS $LIBMIKMOD_CFLAGS $LIBMIKMOD_LDADD"
      LIBS="$LIBMIKMOD_LIBS $LIBS"
dnl
dnl Now check if the installed libmikmod is sufficiently new. (Also sanity
dnl checks the results of libmikmod-config to some extent
dnl
      rm -f conf.mikmodtest
      AC_TRY_RUN([
#include <mikmod.h>
#include <stdio.h>
#include <stdlib.h>

char* my_strdup (char *str)
{
  char *new_str;

  if (str) {
    new_str = malloc ((strlen (str) + 1) * sizeof(char));
    strcpy (new_str, str);
  } else
    new_str = NULL;

  return new_str;
}

int main()
{
  int major,minor,micro;
  int libmikmod_major_version,libmikmod_minor_version,libmikmod_micro_version;
  char *tmp_version;

  system("touch conf.mikmodtest");

  /* HP/UX 9 (%@#!) writes to sscanf strings */
  tmp_version = my_strdup("$min_libmikmod_version");
  if (sscanf(tmp_version, "%d.%d.%d", &major, &minor, &micro) != 3) {
     printf("%s, bad version string\n", "$min_libmikmod_version");
     exit(1);
   }

  libmikmod_major_version=(MikMod_GetVersion() >> 16) & 255;
  libmikmod_minor_version=(MikMod_GetVersion() >>  8) & 255;
  libmikmod_micro_version=(MikMod_GetVersion()      ) & 255;

  if ((libmikmod_major_version != $libmikmod_config_major_version) ||
      (libmikmod_minor_version != $libmikmod_config_minor_version) ||
      (libmikmod_micro_version != $libmikmod_config_micro_version))
    {
      printf("\n*** 'libmikmod-config --version' returned %d.%d.%d, but libmikmod (%d.%d.%d)\n", 
             $libmikmod_config_major_version, $libmikmod_config_minor_version, $libmikmod_config_micro_version,
             libmikmod_major_version, libmikmod_minor_version, libmikmod_micro_version);
      printf ("*** was found! If libmikmod-config was correct, then it is best\n");
      printf ("*** to remove the old version of libmikmod. You may also be able to fix the error\n");
      printf("*** by modifying your LD_LIBRARY_PATH enviroment variable, or by editing\n");
      printf("*** /etc/ld.so.conf. Make sure you have run ldconfig if that is\n");
      printf("*** required on your system.\n");
      printf("*** If libmikmod-config was wrong, set the environment variable LIBMIKMOD_CONFIG\n");
      printf("*** to point to the correct copy of libmikmod-config, and remove the file config.cache\n");
      printf("*** before re-running configure\n");
    } 
  else if ((libmikmod_major_version != LIBMIKMOD_VERSION_MAJOR) ||
	   (libmikmod_minor_version != LIBMIKMOD_VERSION_MINOR) ||
           (libmikmod_micro_version != LIBMIKMOD_REVISION))
    {
      printf("*** libmikmod header files (version %d.%d.%d) do not match\n",
	     LIBMIKMOD_VERSION_MAJOR, LIBMIKMOD_VERSION_MINOR, LIBMIKMOD_REVISION);
      printf("*** library (version %d.%d.%d)\n",
	     libmikmod_major_version, libmikmod_minor_version, libmikmod_micro_version);
    }
  else
    {
      if ((libmikmod_major_version > major) ||
        ((libmikmod_major_version == major) && (libmikmod_minor_version > minor)) ||
        ((libmikmod_major_version == major) && (libmikmod_minor_version == minor) && (libmikmod_micro_version >= micro)))
      {
        return 0;
       }
     else
      {
        printf("\n*** An old version of libmikmod (%d.%d.%d) was found.\n",
               libmikmod_major_version, libmikmod_minor_version, libmikmod_micro_version);
        printf("*** You need a version of libmikmod newer than %d.%d.%d.\n",
	       major, minor, micro);
        printf("***\n");
        printf("*** If you have already installed a sufficiently new version, this error\n");
        printf("*** probably means that the wrong copy of the libmikmod-config shell script is\n");
        printf("*** being found. The easiest way to fix this is to remove the old version\n");
        printf("*** of libmikmod, but you can also set the LIBMIKMOD_CONFIG environment to point to the\n");
        printf("*** correct copy of libmikmod-config. (In this case, you will have to\n");
        printf("*** modify your LD_LIBRARY_PATH enviroment variable, or edit /etc/ld.so.conf\n");
        printf("*** so that the correct libraries are found at run-time))\n");
      }
    }
  return 1;
}
],, no_libmikmod=yes,[echo $ac_n "cross compiling; assumed OK... $ac_c"])
       CFLAGS="$ac_save_CFLAGS"
       LIBS="$ac_save_LIBS"
	   AC_LANG_RESTORE
     fi
  fi
  if test "x$no_libmikmod" = x ; then
     AC_MSG_RESULT([yes, `$LIBMIKMOD_CONFIG --version`])
     ifelse([$2], , :, [$2])     
  else
     AC_MSG_RESULT(no)
     if test "$LIBMIKMOD_CONFIG" = "no" ; then
       echo "*** The libmikmod-config script installed by libmikmod could not be found"
       echo "*** If libmikmod was installed in PREFIX, make sure PREFIX/bin is in"
       echo "*** your path, or set the LIBMIKMOD_CONFIG environment variable to the"
       echo "*** full path to libmikmod-config."
     else
       if test -f conf.mikmodtest ; then
        :
       else
          echo "*** Could not run libmikmod test program, checking why..."
          CFLAGS="$CFLAGS $LIBMIKMOD_CFLAGS"
          LIBS="$LIBS $LIBMIKMOD_LIBS"
		  AC_LANG_SAVE
		  AC_LANG_C
          AC_TRY_LINK([
#include <mikmod.h>
#include <stdio.h>
],      [ return (MikMod_GetVersion()!=0); ],
        [ echo "*** The test program compiled, but did not run. This usually means"
          echo "*** that the run-time linker is not finding libmikmod or finding the wrong"
          echo "*** version of libmikmod. If it is not finding libmikmod, you'll need to set your"
          echo "*** LD_LIBRARY_PATH environment variable, or edit /etc/ld.so.conf to point"
          echo "*** to the installed location. Also, make sure you have run ldconfig if that"
          echo "*** is required on your system."
	  echo "***"
          echo "*** If you have an old version installed, it is best to remove it, although"
          echo "*** you may also be able to get things to work by modifying LD_LIBRARY_PATH"],
        [ echo "*** The test program failed to compile or link. See the file config.log for the"
          echo "*** exact error that occured. This usually means libmikmod was incorrectly installed"
          echo "*** or that you have moved libmikmod since it was installed. In the latter case, you"
          echo "*** may want to edit the libmikmod-config script: $LIBMIKMOD_CONFIG" ])
          CFLAGS="$ac_save_CFLAGS"
          LIBS="$ac_save_LIBS"
		  AC_LANG_RESTORE
       fi
     fi
     LIBMIKMOD_CFLAGS=""
     LIBMIKMOD_LIBS=""
     LIBMIKMOD_LDADD=""
     ifelse([$3], , :, [$3])
  fi
  AC_SUBST(LIBMIKMOD_CFLAGS)
  AC_SUBST(LIBMIKMOD_LIBS)
  AC_SUBST(LIBMIKMOD_LDADD)
  rm -f conf.mikmodtest
])
# Configure paths for libvorbis
# Jack Moffitt <jack@icecast.org> 10-21-2000
# Shamelessly stolen from Owen Taylor and Manish Singh

dnl AM_PATH_VORBIS([ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]])
dnl Test for libvorbis, and define VORBIS_CFLAGS and VORBIS_LIBS
dnl
AC_DEFUN(AM_PATH_VORBIS,
[dnl 
dnl Get the cflags and libraries
dnl
AC_ARG_WITH(vorbis-prefix,[  --with-vorbis-prefix=PFX   Prefix where libvorbis is installed (optional)], vorbis_prefix="$withval", vorbis_prefix="")
AC_ARG_ENABLE(vorbistest, [  --disable-vorbistest       Do not try to compile and run a test Vorbis program],, enable_vorbistest=yes)

  if test x$vorbis_prefix != x ; then
    vorbis_args="$vorbis_args --prefix=$vorbis_prefix"
    VORBIS_CFLAGS="-I$vorbis_prefix/include"
    VORBIS_LIBDIR="-L$vorbis_prefix/lib"
  fi

  VORBIS_LIBS="$VORBIS_LIBDIR -lvorbis -lm"
  VORBISFILE_LIBS="-lvorbisfile"
  VORBISENC_LIBS="-lvorbisenc"

  AC_MSG_CHECKING(for Vorbis)
  no_vorbis=""


  if test "x$enable_vorbistest" = "xyes" ; then
    ac_save_CFLAGS="$CFLAGS"
    ac_save_LIBS="$LIBS"
    CFLAGS="$CFLAGS $VORBIS_CFLAGS"
    LIBS="$LIBS $VORBIS_LIBS $OGG_LIBS"
dnl
dnl Now check if the installed Vorbis is sufficiently new.
dnl
      rm -f conf.vorbistest
      AC_TRY_RUN([
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vorbis/codec.h>

int main ()
{
  system("touch conf.vorbistest");
  return 0;
}

],, no_vorbis=yes,[echo $ac_n "cross compiling; assumed OK... $ac_c"])
       CFLAGS="$ac_save_CFLAGS"
       LIBS="$ac_save_LIBS"
  fi

  if test "x$no_vorbis" = x ; then
     AC_MSG_RESULT(yes)
     ifelse([$1], , :, [$1])     
  else
     AC_MSG_RESULT(no)
     if test -f conf.vorbistest ; then
       :
     else
       echo "*** Could not run Vorbis test program, checking why..."
       CFLAGS="$CFLAGS $VORBIS_CFLAGS"
       LIBS="$LIBS $VORBIS_LIBS $OGG_LIBS"
       AC_TRY_LINK([
#include <stdio.h>
#include <vorbis/codec.h>
],     [ return 0; ],
       [ echo "*** The test program compiled, but did not run. This usually means"
       echo "*** that the run-time linker is not finding Vorbis or finding the wrong"
       echo "*** version of Vorbis. If it is not finding Vorbis, you'll need to set your"
       echo "*** LD_LIBRARY_PATH environment variable, or edit /etc/ld.so.conf to point"
       echo "*** to the installed location  Also, make sure you have run ldconfig if that"
       echo "*** is required on your system"
       echo "***"
       echo "*** If you have an old version installed, it is best to remove it, although"
       echo "*** you may also be able to get things to work by modifying LD_LIBRARY_PATH"],
       [ echo "*** The test program failed to compile or link. See the file config.log for the"
       echo "*** exact error that occured. This usually means Vorbis was incorrectly installed"
       echo "*** or that you have moved Vorbis since it was installed." ])
       CFLAGS="$ac_save_CFLAGS"
       LIBS="$ac_save_LIBS"
     fi
     VORBIS_CFLAGS=""
     VORBIS_LIBS=""
     VORBISFILE_LIBS=""
     VORBISENC_LIBS=""
     ifelse([$2], , :, [$2])
  fi
  AC_SUBST(VORBIS_CFLAGS)
  AC_SUBST(VORBIS_LIBS)
  AC_SUBST(VORBISFILE_LIBS)
  AC_SUBST(VORBISENC_LIBS)
  rm -f conf.vorbistest
])
# Configure paths for SMPEG
# Nicolas Vignal 11/19/2000
# stolen from Sam Lantinga
# stolen from Manish Singh
# stolen back from Frank Belew
# stolen from Manish Singh
# Shamelessly stolen from Owen Taylor

dnl AM_PATH_SMPEG([MINIMUM-VERSION, [ACTION-IF-FOUND [, 
ACTION-IF-NOT-FOUND]]])
dnl Test for SMPEG, and define SMPEG_CFLAGS and SMPEG_LIBS
dnl
AC_DEFUN(AM_PATH_SMPEG,
[dnl
dnl Get the cflags and libraries from the smpeg-config script
dnl
AC_ARG_WITH(smpeg-prefix,[  --with-smpeg-prefix=PFX   Prefix where SMPEG is 
installed (optional)],
            smpeg_prefix="$withval", smpeg_prefix="")
AC_ARG_WITH(smpeg-exec-prefix,[  --with-smpeg-exec-prefix=PFX Exec prefix 
where SMPEG is installed (optional)],
            smpeg_exec_prefix="$withval", smpeg_exec_prefix="")
AC_ARG_ENABLE(smpegtest, [  --disable-smpegtest       Do not try to compile 
and run a test SMPEG program],
                    , enable_smpegtest=yes)

  if test x$smpeg_exec_prefix != x ; then
     smpeg_args="$smpeg_args --exec-prefix=$smpeg_exec_prefix"
     if test x${SMPEG_CONFIG+set} != xset ; then
        SMPEG_CONFIG=$smpeg_exec_prefix/bin/smpeg-config
     fi
  fi
  if test x$smpeg_prefix != x ; then
     smpeg_args="$smpeg_args --prefix=$smpeg_prefix"
     if test x${SMPEG_CONFIG+set} != xset ; then
        SMPEG_CONFIG=$smpeg_prefix/bin/smpeg-config
     fi
  fi

  AC_PATH_PROG(SMPEG_CONFIG, smpeg-config, no)
  min_smpeg_version=ifelse([$1], ,0.2.7,$1)
  AC_MSG_CHECKING(for SMPEG - version >= $min_smpeg_version)
  no_smpeg=""
  if test "$SMPEG_CONFIG" = "no" ; then
    no_smpeg=yes
  else
    SMPEG_CFLAGS=`$SMPEG_CONFIG $smpegconf_args --cflags`
    SMPEG_LIBS=`$SMPEG_CONFIG $smpegconf_args --libs`

    smpeg_major_version=`$SMPEG_CONFIG $smpeg_args --version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\1/'`
    smpeg_minor_version=`$SMPEG_CONFIG $smpeg_args --version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\2/'`
    smpeg_micro_version=`$SMPEG_CONFIG $smpeg_config_args --version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\3/'`
    if test "x$enable_smpegtest" = "xyes" ; then
      ac_save_CFLAGS="$CFLAGS"
      ac_save_LIBS="$LIBS"
      CFLAGS="$CFLAGS $SMPEG_CFLAGS $SDL_CFLAGS"
      LIBS="$LIBS $SMPEG_LIBS $SDL_LIBS"
dnl
dnl Now check if the installed SMPEG is sufficiently new. (Also sanity
dnl checks the results of smpeg-config to some extent
dnl
      rm -f conf.smpegtest
      AC_TRY_RUN([
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "smpeg.h"

char*
my_strdup (char *str)
{
  char *new_str;

  if (str)
    {
      new_str = (char *)malloc ((strlen (str) + 1) * sizeof(char));
      strcpy (new_str, str);
    }
  else
    new_str = NULL;

  return new_str;
}

int main (int argc, char *argv[])
{
  int major, minor, micro;
  char *tmp_version;

  /* This hangs on some systems (?)
  system ("touch conf.smpegtest");
  */
  { FILE *fp = fopen("conf.smpegtest", "a"); if ( fp ) fclose(fp); }

  /* HP/UX 9 (%@#!) writes to sscanf strings */
  tmp_version = my_strdup("$min_smpeg_version");
  if (sscanf(tmp_version, "%d.%d.%d", &major, &minor, &micro) != 3) {
     printf("%s, bad version string\n", "$min_smpeg_version");
     exit(1);
   }

   if (($smpeg_major_version > major) ||
      (($smpeg_major_version == major) && ($smpeg_minor_version > minor)) ||
      (($smpeg_major_version == major) && ($smpeg_minor_version == minor) 
&& ($smpeg_micro_version >= micro)))
    {
      return 0;
    }
  else
    {
      printf("\n*** 'smpeg-config --version' returned %d.%d.%d, but the 
minimum version\n", $smpeg_major_version, $smpeg_minor_version, 
$smpeg_micro_version);
      printf("*** of SMPEG required is %d.%d.%d. If smpeg-config is 
correct, then it is\n", major, minor, micro);
      printf("*** best to upgrade to the required version.\n");
      printf("*** If smpeg-config was wrong, set the environment variable 
SMPEG_CONFIG\n");
      printf("*** to point to the correct copy of smpeg-config, and remove 
the file\n");
      printf("*** config.cache before re-running configure\n");
      return 1;
    }
}

],, no_smpeg=yes,[echo $ac_n "cross compiling; assumed OK... $ac_c"])
       CFLAGS="$ac_save_CFLAGS"
       LIBS="$ac_save_LIBS"
     fi
  fi
  if test "x$no_smpeg" = x ; then
     AC_MSG_RESULT(yes)
     ifelse([$2], , :, [$2])
  else
     AC_MSG_RESULT(no)
     if test "$SMPEG_CONFIG" = "no" ; then
       echo "*** The smpeg-config script installed by SMPEG could not be 
found"
       echo "*** If SMPEG was installed in PREFIX, make sure PREFIX/bin is 
in"
       echo "*** your path, or set the SMPEG_CONFIG environment variable to 
the"
       echo "*** full path to smpeg-config."
     else
       if test -f conf.smpegtest ; then
        :
       else
          echo "*** Could not run SMPEG test program, checking why..."
          CFLAGS="$CFLAGS $SMPEG_CFLAGS $SDL_CFLAGS"
          LIBS="$LIBS $SMPEG_LIBS $SDL_LIBS"
          AC_TRY_LINK([
#include <stdio.h>
#include "smpeg.h"
],      [ return 0; ],
        [ echo "*** The test program compiled, but did not run. This 
usually means"
          echo "*** that the run-time linker is not finding SMPEG or 
finding the wrong"
          echo "*** version of SMPEG. If it is not finding SMPEG, you'll 
need to set your"
          echo "*** LD_LIBRARY_PATH environment variable, or edit 
/etc/ld.so.conf to point"
          echo "*** to the installed location  Also, make sure you have run 
ldconfig if that"
          echo "*** is required on your system"
          echo "***"
          echo "*** If you have an old version installed, it is best to 
remove it, although"
          echo "*** you may also be able to get things to work by modifying 
LD_LIBRARY_PATH"],
        [ echo "*** The test program failed to compile or link. See the 
file config.log for the"
          echo "*** exact error that occured. This usually means SMPEG was 
incorrectly installed"
          echo "*** or that you have moved SMPEG since it was installed. In 
the latter case, you"
          echo "*** may want to edit the smpeg-config script: 
$SMPEG_CONFIG" ])
          CFLAGS="$ac_save_CFLAGS"
          LIBS="$ac_save_LIBS"
       fi
     fi
     SMPEG_CFLAGS=""
     SMPEG_LIBS=""
     ifelse([$3], , :, [$3])
  fi
  AC_SUBST(SMPEG_CFLAGS)
  AC_SUBST(SMPEG_LIBS)
  rm -f conf.smpegtest
])
