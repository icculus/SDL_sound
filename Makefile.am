lib_LTLIBRARIES = libSDL_sound.la

SUBDIRS = decoders . playsound

libSDL_soundincludedir = $(includedir)/SDL
libSDL_soundinclude_HEADERS =	\
	SDL_sound.h

libSDL_sound_la_SOURCES =	\
	SDL_sound.c	\
	SDL_sound_internal.h	\
	extra_rwops.c	\
	extra_rwops.h	\
	audio_convert.c

if USE_TIMIDITY
TIMIDITY_LIB = decoders/timidity/libtimidity.la
else
TIMIDITY_LIB =
endif

if USE_MPGLIB
MPGLIB_LIB = decoders/mpglib/libmpglib.la
else
MPGLIB_LIB =
endif

libSDL_sound_la_LDFLAGS = 		\
	-release $(LT_RELEASE)	\
	-version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE)
libSDL_sound_la_LIBADD =	\
	decoders/libdecoders.la	\
	$(TIMIDITY_LIB) $(MPGLIB_LIB)

EXTRA_DIST =	\
	CREDITS \
	COPYING \
	CHANGELOG \
	CWProject.sit
