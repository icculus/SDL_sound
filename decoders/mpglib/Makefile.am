noinst_LTLIBRARIES = libmpglib.la

INCLUDES = -I$(top_srcdir)
libmpglib_la_CFLAGS = -DLAYER1 -DLAYER2 -DLAYER3

if USE_MPGLIB
libmpglib_la_SOURCES =	\
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
else
libmpglib_la_SOURCES =
endif

#EXTRA_DIST = FAQ README CHANGES TODO
