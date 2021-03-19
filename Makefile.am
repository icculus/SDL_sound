ACLOCAL_AMFLAGS = -I acinclude
AUTOMAKE_OPTIONS = foreign

SUBDIRS = decoders . examples

if USE_TIMIDITY
TIMIDITY_LIB = decoders/timidity/libtimidity.la
else
TIMIDITY_LIB =
endif

SDLSOUND_HDRS = SDL_sound.h
SDLSOUND_SRCS = SDL_sound.c SDL_sound_internal.h alt_audio_convert.c alt_audio_convert.h audio_convert.c
SDLSOUND_LDFLAGS = -no-undefined -release $(LT_RELEASE) -version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE)
SDLSOUND_LIBADD = decoders/libdecoders.la $(TIMIDITY_LIB)

if USE_SDL2
lib_LTLIBRARIES = libSDL2_sound.la
libSDL2_soundincludedir = $(includedir)/SDL2
libSDL2_soundinclude_HEADERS = $(SDLSOUND_HDRS)
libSDL2_sound_la_SOURCES = $(SDLSOUND_SRCS)
libSDL2_sound_la_LDFLAGS = $(SDLSOUND_LDFLAGS)
libSDL2_sound_la_LIBADD = $(SDLSOUND_LIBADD)
else
lib_LTLIBRARIES = libSDL_sound.la
libSDL_soundincludedir = $(includedir)/SDL
libSDL_soundinclude_HEADERS = $(SDLSOUND_HDRS)
libSDL_sound_la_SOURCES = $(SDLSOUND_SRCS)
libSDL_sound_la_LDFLAGS = $(SDLSOUND_LDFLAGS)
libSDL_sound_la_LIBADD = $(SDLSOUND_LIBADD)
endif

EXTRA_DIST =	\
	CREDITS \
	COPYING \
	CHANGELOG \
	CWProject.sit \
	PBProjects.tar.gz \
	Doxyfile \
	Makefile.os2 \
	VisualC

dist-hook:
	mkdir $(distdir)/docs
	echo "Docs are generated with the program "Doxygen" (http://www.doxygen.org/)," >> $(distdir)/docs/README
	echo " or can be read online at http://icculus.org/SDL_sound/docs/" >> $(distdir)/docs/README
	echo >> $(distdir)/docs/README
	rm -rf `find $(distdir) -type d -name ".hg"`
