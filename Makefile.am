lib_LTLIBRARIES = libSDL_sound.la

SUBDIRS = decoders test

libSDL_soundincludedir = $(includedir)/SDL
libSDL_soundinclude_HEADERS =	\
	SDL_sound.h

libSDL_sound_la_SOURCES =	\
	SDL_sound.c	\
	SDL_sound_internal.h	\
	extra_rwops.c	\
	extra_rwops.h	\
	audio_convert.c

libSDL_sound_la_LDFLAGS = 		\
	-release $(LT_RELEASE)	\
	-version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE)
libSDL_sound_la_LIBADD = decoders/libdecoders.la


EXTRA_DIST =	\
	CREDITS \
	LICENSE \
	CHANGELOG
        
