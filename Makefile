#-----------------------------------------------------------------------------#
# SDL_sound -- An abstract sound format decoding API.
# Copyright (C) 2001  Ryan C. Gordon.
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#
#  (Please see the file LICENSE in the source's root directory.)
#
#   This file written by Ryan C. Gordon.
#-----------------------------------------------------------------------------#


#-----------------------------------------------------------------------------#
# Makefile for building SDL_sound on Unix-like systems. Follow the
#  instructions for editing this file, then run "make" on the command line.
#-----------------------------------------------------------------------------#


#-----------------------------------------------------------------------------#
# Set to your liking.
#-----------------------------------------------------------------------------#
CC = gcc
LINKER = gcc


#-----------------------------------------------------------------------------#
# If this makefile fails to detect Cygwin correctly, or you want to force
#  the build process's behaviour, set it to "true" or "false" (w/o quotes).
#-----------------------------------------------------------------------------#
#cygwin := true
#cygwin := false
cygwin := autodetect

#-----------------------------------------------------------------------------#
# You only need to set SDL_INC_DIR and SDL_LIB_DIR if you are using cygwin.
#  SDL_INC_DIR is where SDL.h and associated headers can be found, and
#  SDL_LIB_DIR is where SDL.lib and SDL.dll are located. These may be set as
#  environment variables, if you'd prefer to not hack the Makefile.
#
# examples:
#   SDL_INC_DIR := C:/2/SDL-1.1.8/include
#   SDL_LIB_DIR := C:/2/SDL-1.1.8/lib
#-----------------------------------------------------------------------------#
ifeq ($(strip $(SDL_INC_DIR)),)
  SDL_INC_DIR := please_set_me_cygwin_users
endif

ifeq ($(strip $(SDL_LIB_DIR)),)
  SDL_LIB_DIR := please_set_me_cygwin_users
endif

#-----------------------------------------------------------------------------#
# Set this to true if you want to create a debug build.
#  (for generating debugging symbols, disabling optimizations, etc.)
#-----------------------------------------------------------------------------#
#debugging := false
debugging := true

#-----------------------------------------------------------------------------#
# Set this to true if you want debugging output. This does a LOT of
#  chattering to stdout, and can be used with out without the (debugging)
#  option above. You do NOT want this in a release build!
#-----------------------------------------------------------------------------#
#debugging_chatter := false
debugging_chatter := true

#-----------------------------------------------------------------------------#
# Set the decoder types you'd like to support.
#  Note that various decoders may need external libraries.
#-----------------------------------------------------------------------------#
use_decoder_raw  := true
use_decoder_mp3  := true
use_decoder_mod  := true
use_decoder_voc  := true
use_decoder_wav  := true
use_decoder_aiff := true
use_decoder_ogg  := true


#-----------------------------------------------------------------------------#
# If use_decoder_mod is set to true, and libmikmod is in a nonstandard place,
#  note that here. It's usually safe to leave these commented if you installed
#  MikMod from a package like an RPM, or from source with the defaults.
#-----------------------------------------------------------------------------#
#INCPATH_MOD := -I/usr/local/include
#LIBPATH_MOD := -L/usr/local/lib


#-----------------------------------------------------------------------------#
# If use_decoder_ogg is set to true, and libvorbis and libvorbisfile are
#  in a nonstandard place, set them here. It's usually safe to leave these
#  commented if you installed Ogg Vorbis from a package like an RPM, or from
#  source with the defaults.
#-----------------------------------------------------------------------------#
#INCPATH_OGG := -I/usr/local/include
#LIBPATH_OGG := -L/usr/local/lib


#-----------------------------------------------------------------------------#
# Set to "true" if you'd like to build a DLL. Set to "false" otherwise.
#-----------------------------------------------------------------------------#
#build_dll := false
build_dll := true

#-----------------------------------------------------------------------------#
# Set one of the below. Currently, none of these are used.
#-----------------------------------------------------------------------------#
#use_asm = -DUSE_I386_ASM
use_asm = -DUSE_PORTABLE_C


#-----------------------------------------------------------------------------#
# Set this to where you want SDL_sound installed. It will put the
#  files in $(install_prefix)/bin, $(install_prefix)/lib, and
#  $(install_prefix)/include  ...
#-----------------------------------------------------------------------------#
install_prefix := /usr/local


#-----------------------------------------------------------------------------#
#-----------------------------------------------------------------------------#
#-----------------------------------------------------------------------------#
#-----------------------------------------------------------------------------#
# Everything below this line is probably okay.
#-----------------------------------------------------------------------------#
#-----------------------------------------------------------------------------#
#-----------------------------------------------------------------------------#
#-----------------------------------------------------------------------------#

#-----------------------------------------------------------------------------#
# CygWin autodetect.
#-----------------------------------------------------------------------------#

ifeq ($(strip $(cygwin)),autodetect)
  ifneq ($(strip $(shell gcc -v 2>&1 |grep "cygwin")),)
    cygwin := true
  else
    cygwin := false
  endif
endif


#-----------------------------------------------------------------------------#
# Platform-specific binary stuff.
#-----------------------------------------------------------------------------#

ifeq ($(strip $(cygwin)),true)
  ifeq ($(strip $(SDL_INC_DIR)),please_set_me_cygwin_users)
    $(error Cygwin users need to set the SDL_INC_DIR envr var.)
  else
    SDL_CFLAGS := -I$(SDL_INC_DIR)
  endif

  ifeq ($(strip $(SDL_LIB_DIR)),please_set_me_cygwin_users)
    $(error Cygwin users need to set the SDL_LIB_DIR envr var.)
  else
    SDL_LDFLAGS := -L$(SDL_LIB_DIR) -lSDL
  endif

  # !!! FIXME
  build_dll := false
  # !!! FIXME

  ASM = nasmw
  EXE_EXT = .exe
  DLL_EXT = .dll
  STATICLIB_EXT = .a
  ASMOBJFMT = win32
  ASMDEFS = -dC_IDENTIFIERS_UNDERSCORED
  CFLAGS += -DC_IDENTIFIERS_UNDERSCORED
else
  ASM = nasm
  EXE_EXT =
  DLL_EXT = .so
  STATICLIB_EXT = .a
  ASMOBJFMT = elf
  SDL_CFLAGS := $(shell sdl-config --cflags)
  SDL_LDFLAGS := $(shell sdl-config --libs)
endif

CFLAGS += $(SDL_CFLAGS)
LDFLAGS += $(SDL_LDFLAGS)

ifeq ($(strip $(build_dll)),true)
LIB_EXT := $(DLL_EXT)
SHAREDFLAGS += -shared
else
LIB_EXT := $(STATICLIB_EXT)
endif

#-----------------------------------------------------------------------------#
# Version crapola.
#-----------------------------------------------------------------------------#
VERMAJOR := $(shell grep "define SOUND_VER_MAJOR" SDL_sound.h | sed "s/\#define SOUND_VER_MAJOR //")
VERMINOR := $(shell grep "define SOUND_VER_MINOR" SDL_sound.h | sed "s/\#define SOUND_VER_MINOR //")
VERPATCH := $(shell grep "define SOUND_VER_PATCH" SDL_sound.h | sed "s/\#define SOUND_VER_PATCH //")

VERMAJOR := $(strip $(VERMAJOR))
VERMINOR := $(strip $(VERMINOR))
VERPATCH := $(strip $(VERPATCH))

VERFULL := $(VERMAJOR).$(VERMINOR).$(VERPATCH)

#-----------------------------------------------------------------------------#
# General compiler, assembler, and linker flags.
#-----------------------------------------------------------------------------#

BINDIR := bin
SRCDIR := .

CFLAGS := -I$(SRCDIR) $(CFLAGS)
CFLAGS += $(use_asm) -D_REENTRANT -fsigned-char -DPLATFORM_UNIX
CFLAGS += -Wall -Werror -fno-exceptions -fno-rtti -ansi -pedantic

LDFLAGS += -lm

ifeq ($(strip $(debugging)),true)
  CFLAGS += -DDEBUG -g -fno-omit-frame-pointer
  LDFLAGS += -g -fno-omit-frame-pointer
else
  CFLAGS += -DNDEBUG -O2 -fomit-frame-pointer
  LDFLAGS += -O2 -fomit-frame-pointer
endif

ifeq ($(strip $(debugging_chatter)),true)
  CFLAGS += -DDEBUG_CHATTER
endif

ASMFLAGS := -f $(ASMOBJFMT) $(ASMDEFS)

#-----------------------------------------------------------------------------#
# Source and target names.
#-----------------------------------------------------------------------------#

PUREBASELIBNAME := SDL_sound
ifeq ($(strip $(cygwin)),true)
BASELIBNAME := $(strip $(PUREBASELIBNAME))
else
BASELIBNAME := lib$(strip $(PUREBASELIBNAME))
endif

MAINLIB := $(BINDIR)/$(strip $(BASELIBNAME))$(strip $(LIB_EXT))

TESTSRCS := test/test_sdlsound.c

MAINSRCS := SDL_sound.c

need_extra_rwops := false
ifeq ($(strip $(use_decoder_raw)),true)
  MAINSRCS += decoders/raw.c
  CFLAGS += -DSOUND_SUPPORTS_RAW
endif

ifeq ($(strip $(use_decoder_mp3)),true)
  MAINSRCS += decoders/mp3.c
  need_extra_rwops := true
  CFLAGS += -DSOUND_SUPPORTS_MP3 $(shell smpeg-config --cflags)
  LDFLAGS += $(shell smpeg-config --libs)
endif

ifeq ($(strip $(use_decoder_mod)),true)
  MAINSRCS += decoders/mod.c
  CFLAGS += -DSOUND_SUPPORTS_MOD
  LDFLAGS += -lmikmod
endif

ifeq ($(strip $(use_decoder_voc)),true)
  MAINSRCS += decoders/voc.c
  CFLAGS += -DSOUND_SUPPORTS_VOC
endif

ifeq ($(strip $(use_decoder_wav)),true)
  MAINSRCS += decoders/wav.c
  CFLAGS += -DSOUND_SUPPORTS_WAV
endif

ifeq ($(strip $(use_decoder_aiff)),true)
  MAINSRCS += decoders/aiff.c
  CFLAGS += -DSOUND_SUPPORTS_AIFF
endif

ifeq ($(strip $(use_decoder_ogg)),true)
  MAINSRCS += decoders/ogg.c
  CFLAGS += -DSOUND_SUPPORTS_OGG $(INCPATH_OGG)
  LDFLAGS += $(LIBPATH_OGG) -lvorbisfile -lvorbis
endif

ifeq ($(strip $(need_extra_rwops)),true)
  MAINSRCS += extra_rwops.c
endif

#ifeq ($(strip $(cygwin)),true)
#  MAINSRCS += platform/win32.c
#  CFLAGS += -DWIN32
#else
#  MAINSRCS += platform/unix.c
#endif

TESTEXE := $(BINDIR)/test_sdlsound$(EXE_EXT)

# Rule for getting list of objects from source
MAINOBJS1 := $(MAINSRCS:.c=.o)
MAINOBJS2 := $(MAINOBJS1:.cpp=.o)
MAINOBJS3 := $(MAINOBJS2:.asm=.o)
MAINOBJS := $(foreach f,$(MAINOBJS3),$(BINDIR)/$(f))
MAINSRCS := $(foreach f,$(MAINSRCS),$(SRCDIR)/$(f))

TESTOBJS1 := $(TESTSRCS:.c=.o)
TESTOBJS2 := $(TESTOBJS1:.cpp=.o)
TESTOBJS3 := $(TESTOBJS2:.asm=.o)
TESTOBJS := $(foreach f,$(TESTOBJS3),$(BINDIR)/$(f))
TESTSRCS := $(foreach f,$(TESTSRCS),$(SRCDIR)/$(f))

CLEANUP = $(wildcard *.exe) $(wildcard *.obj) \
          $(wildcard $(BINDIR)/*.exe) $(wildcard $(BINDIR)/*.obj) \
          $(wildcard *~) $(wildcard *.err) \
          $(wildcard .\#*) core


#-----------------------------------------------------------------------------#
# Rules.
#-----------------------------------------------------------------------------#

# Rules for turning source files into .o files
$(BINDIR)/%.o: $(SRCDIR)/%.cpp
	$(CC) -c -o $@ $< $(CFLAGS)

$(BINDIR)/%.o: $(SRCDIR)/%.c
	$(CC) -c -o $@ $< $(CFLAGS)

$(BINDIR)/%.o: $(SRCDIR)/%.asm
	$(ASM) $(ASMFLAGS) -o $@ $<

.PHONY: all clean distclean listobjs install showcfg showflags

all: $(BINDIR) $(EXTRABUILD) $(MAINLIB) $(TESTEXE)

$(MAINLIB) : $(BINDIR) $(MAINOBJS)
	$(LINKER) -o $(MAINLIB) $(SHAREDFLAGS) $(MAINOBJS) $(LDFLAGS)

$(TESTEXE) : $(MAINLIB) $(TESTOBJS)
	$(LINKER) -o $(TESTEXE) $(TESTLDFLAGS) $(TESTOBJS) -L$(BINDIR) -l$(strip $(PUREBASELIBNAME)) $(LDFLAGS)


install: all
	rm -f $(install_prefix)/lib/$(strip $(BASELIBNAME))$(strip $(LIB_EXT)).$(strip $(VERMAJOR)).$(strip $(VERMINOR)).*
	mkdir -p $(install_prefix)/bin
	mkdir -p $(install_prefix)/lib
	mkdir -p $(install_prefix)/include
	cp $(SRCDIR)/SDL_sound.h $(install_prefix)/include
	cp $(TESTEXE) $(install_prefix)/bin
ifeq ($(strip $(cygwin)),true)
	cp $(MAINLIB) $(install_prefix)/lib/$(strip $(BASELIBNAME))$(strip $(LIB_EXT))
else
	cp $(MAINLIB) $(install_prefix)/lib/$(strip $(BASELIBNAME))$(strip $(LIB_EXT)).$(strip $(VERFULL))
	ln -sf $(strip $(BASELIBNAME))$(strip $(LIB_EXT)).$(strip $(VERFULL)) $(install_prefix)/lib/$(strip $(BASELIBNAME))$(strip $(LIB_EXT))
	ln -sf $(strip $(BASELIBNAME))$(strip $(LIB_EXT)).$(strip $(VERFULL)) $(install_prefix)/lib/$(strip $(BASELIBNAME))$(strip $(LIB_EXT)).$(strip $(VERMAJOR))
	chmod 0755 $(install_prefix)/lib/$(strip $(BASELIBNAME))$(strip $(LIB_EXT)).$(strip $(VERFULL))
	chmod 0644 $(install_prefix)/include/SDL_sound.h
endif

$(BINDIR):
	mkdir -p $(BINDIR)
	mkdir -p $(BINDIR)/decoders
	mkdir -p $(BINDIR)/platform
	mkdir -p $(BINDIR)/test

distclean: clean

clean:
	rm -f $(CLEANUP)
	rm -rf $(BINDIR)

listobjs:
	@echo SOURCES:
	@echo $(MAINSRCS)
	@echo
	@echo OBJECTS:
	@echo $(MAINOBJS)
	@echo
	@echo BINARIES:
	@echo $(MAINLIB)

showcfg:
	@echo "Compiler              : $(CC)"
	@echo "Using CygWin          : $(cygwin)"
	@echo "Debugging             : $(debugging)"
	@echo "Debugging chatter     : $(debugging_chatter)"
	@echo "ASM flag              : $(use_asm)"
	@echo "SDL_sound version     : $(VERFULL)"
	@echo "Building DLLs         : $(build_dll)"
	@echo "Install prefix        : $(install_prefix)"
	@echo "Supports .WAV         : $(use_decoder_wav)"
	@echo "Supports .AIFF        : $(use_decoder_aiff)"
	@echo "Supports .RAW         : $(use_decoder_raw)"
	@echo "Supports .MP3         : $(use_decoder_mp3)"
	@echo "Supports .MOD         : $(use_decoder_mod)"
	@echo "Supports .VOC         : $(use_decoder_voc)"
	@echo "Supports .OGG         : $(use_decoder_ogg)"

showflags:
	@echo 'CFLAGS  : $(CFLAGS)'
	@echo 'LDFLAGS : $(LDFLAGS)'


#-----------------------------------------------------------------------------#
# This section is pretty much just for Ryan's use to make distributions.
#  You Probably Should Not Touch.
#-----------------------------------------------------------------------------#

# These are the files needed in a binary distribution, regardless of what
#  platform is being used.
BINSCOMMON := LICENSE CHANGELOG SDL_sound.h

.PHONY: package msbins win32bins nocygwin
package: clean
	cd .. ; mv SDL_sound SDL_sound-$(VERFULL) ; tar -cyvvf ./SDL_sound-$(VERFULL).tar.bz2 --exclude="*CVS*" SDL_sound-$(VERFULL) ; mv SDL_sound-$(VERFULL) SDL_sound


ifeq ($(strip $(cygwin)),true)
msbins: win32bins

win32bins: clean all
	cp $(SDL_LIB_DIR)/SDL.dll .
	echo -e "\r\n\r\n\r\nHEY YOU.\r\n\r\n\r\nTake a look at README-win32bins.txt FIRST.\r\n\r\n\r\n--ryan. (icculus@clutteredmind.org)\r\n\r\n" |zip -9rz ../SDL_sound-win32bins-$(shell date +%m%d%Y).zip $(MAINLIB) SDL.dll $(EXTRAPACKAGELIBS) README-win32bins.txt

else

msbins: nocygwin
win32bins: nocygwin

nocygwin:
	@echo This must be done on a Windows box in the Cygwin environment.

endif

#-----------------------------------------------------------------------------#
# That's all, folks.
#-----------------------------------------------------------------------------#

# end of Makefile ...
