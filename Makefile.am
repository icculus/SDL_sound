lib_LTLIBRARIES = libSDL_sound.la

SUBDIRS = decoders . playsound

libSDL_soundincludedir = $(includedir)/SDL
libSDL_soundinclude_HEADERS =	\
	SDL_sound.h

if USE_ALTCVT
CVT_SOURCE = alt_audio_convert.c
else
CVT_SOURCE = audio_convert.c
endif

libSDL_sound_la_SOURCES =	\
	SDL_sound.c	\
	SDL_sound_internal.h	\
    $(CVT_SOURCE)   \
	extra_rwops.c	\
	extra_rwops.h

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
	CWProject.sit \
	PBProjects.tar.gz \
	borland.zip \
	Doxyfile \
    docs

dist-hook:
	rm -rf `find $(distdir)/docs -name CVS`
