bin_PROGRAMS = playsound

INCLUDES = -I$(top_srcdir)

if USE_PHYSICSFS
PHYSFS_CFLG = -DSUPPORT_PHYSFS=1
PHYSFS_SRCS = physfsrwops.c
PHYSFS_LIBS = -lphysfs
else
PHYSFS_CFLG =
PHYSFS_SRCS =
PHYSFS_LIBS =
endif

playsound_CFLAGS = $(PHYSFS_CFLG)
playsound_LDADD = ../libSDL_sound.la $(PHYSFS_LIBS)
playsound_SOURCES = playsound.c $(PHYSFS_SRCS)

