bin_PROGRAMS = playsound playsound_simple

AM_CPPFLAGS = -I$(top_srcdir)

if USE_SDL2
LIBSDL_SOUND=libSDL2_sound.la
else
LIBSDL_SOUND=libSDL_sound.la
endif

playsound_LDADD = ../$(LIBSDL_SOUND)
playsound_SOURCES = playsound.c

playsound_simple_LDADD = ../$(LIBSDL_SOUND)
playsound_simple_SOURCES = playsound_simple.c
