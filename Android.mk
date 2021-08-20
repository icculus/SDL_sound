LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := SDL2_sound

SUPPORT_MIDI ?= false


SUPPORT_MODPLUG ?= true


LOCAL_C_INCLUDES := $(LOCAL_PATH)/src
LOCAL_CFLAGS :=

LOCAL_SRC_FILES := $(LOCAL_PATH)/src/SDL_sound.c \
				$(LOCAL_PATH)/src/SDL_sound_aiff.c \
				$(LOCAL_PATH)/src/SDL_sound_au.c \
				$(LOCAL_PATH)/src/SDL_sound_coreaudio.c \
				$(LOCAL_PATH)/src/SDL_sound_flac.c \
				$(LOCAL_PATH)/src/SDL_sound_mp3.c \
				$(LOCAL_PATH)/src/SDL_sound_midi.c \
				$(LOCAL_PATH)/src/SDL_sound_modplug.c \
				$(LOCAL_PATH)/src/SDL_sound_raw.c \
				$(LOCAL_PATH)/src/SDL_sound_shn.c \
				$(LOCAL_PATH)/src/SDL_sound_voc.c \
				$(LOCAL_PATH)/src/SDL_sound_vorbis.c \
				$(LOCAL_PATH)/src/SDL_sound_wav.c 



ifeq ($(SUPPORT_MIDI), true)
	LOCAL_SRC_FILES += $(wildcard $(LOCAL_PATH)/src/timidity/*.c)
else
	LOCAL_CFLAGS += -DSOUND_SUPPORTS_MIDI=0
endif


ifeq ($(SUPPORT_MODPLUG), true)
	LOCAL_SRC_FILES += $(wildcard $(LOCAL_PATH)/src/libmodplug/*.c)
else
	LOCAL_CFLAGS += -DSOUND_SUPPORTS_MODPLUG=0
endif

LOCAL_SHARED_LIBRARIES := SDL2

LOCAL_EXPORT_C_INCLUDES += $(LOCAL_C_INCLUDES)

include $(BUILD_SHARED_LIBRARY)
