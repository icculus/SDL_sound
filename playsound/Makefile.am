bin_PROGRAMS = playsound

INCLUDES = -I$(top_srcdir)

if USE_PHYSICSFS
PHYSFS_CFLG = -DSUPPORT_PHYSFS=1
PHYSFS_SRCS = physfsrwops.c physfsrwops.h
PHYSFS_LIBS = -lphysfs
EXTRAPHYSFS_SRCS =
else
PHYSFS_CFLG =
PHYSFS_SRCS =
PHYSFS_LIBS =
EXTRAPHYSFS_SRCS = physfsrwops.c physfsrwops.h
endif

playsound_CFLAGS = $(PHYSFS_CFLG)
playsound_LDADD = ../libSDL_sound.la $(PHYSFS_LIBS)
playsound_SOURCES = playsound.c $(PHYSFS_SRCS)

EXTRA_DIST = $(EXTRAPHYSFS_SRCS)