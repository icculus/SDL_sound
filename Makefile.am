lib_LTLIBRARIES = libSDL_sound.la

SUBDIRS = decoders test

include_HEADERS = SDL_sound.h

noinst_HEADERS = SDL_sound_internal.h

libSDL_sound_la_SOURCES =	\
	SDL_sound.c	\
	extra_rwops.c	\
	extra_rwops.h

libSDL_sound_la_LDFLAGS = 		\
	-release $(LT_RELEASE)	\
	-version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE)
libSDL_sound_la_LIBADD = decoders/libdecoders.la
