noinst_LTLIBRARIES = libtimidity.la

INCLUDES = -I$(top_srcdir)

if USE_TIMIDITY
libtimidity_la_SOURCES =	\
	common.c		\
	common.h		\
	dls1.h			\
	dls2.h			\
	instrum.c		\
	instrum.h		\
	instrum_dls.c		\
	instrum_dls.h		\
	mix.c			\
	mix.h			\
	options.h		\
	output.c		\
	output.h		\
	playmidi.c		\
	playmidi.h		\
	readmidi.c		\
	readmidi.h		\
	resample.c		\
	resample.h		\
	tables.c		\
	tables.h		\
	timidity.c		\
	timidity.h
else
libtimidity_la_SOURCES =
endif

EXTRA_DIST = FAQ README CHANGES TODO
