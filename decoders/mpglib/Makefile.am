if USE_LIBMPG123
noinst_LTLIBRARIES = libmpg123.la
endif

INCLUDES = -I$(top_srcdir)
libmpg123_la_CFLAGS = -DLAYER1 -DLAYER2 -DLAYER3

libmpg123_la_SOURCES =	\
	mpglib_common.c     \
	huffman.h           \
	layer1.c            \
	tabinit.c           \
	dct64_i386.c        \
	interface.c         \
	layer2.c            \
	mpg123_sdlsound.h   \
	decode_i386.c       \
	l2tables.h          \
	layer3.c            \
	mpglib_sdlsound.h

EXTRA_DIST = CHANGES README README-sdlsound TODO main.c

