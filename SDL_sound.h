/*
 * SDL_sound -- An abstract sound format decoding API.
 * Copyright (C) 2001  Ryan C. Gordon.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/**
 * @overview
 *
 * The basic gist of SDL_sound is that you use an SDL_RWops to get sound data
 *  into this library, and SDL_sound will take that data, in one of several
 *  popular formats, and decode it into raw waveform data in the format of
 *  your choice. This gives you a nice abstraction for getting sound into your
 *  game or application; just feed it to SDL_sound, and it will handle
 *  decoding and converting, so you can just pass it to your SDL audio
 *  callback (or whatever). Since it gets data from an SDL_RWops, you can get
 *  the initial sound data from any number of sources: file, memory buffer,
 *  network connection, etc.
 *
 * As the name implies, this library depends on SDL: Simple Directmedia Layer,
 *  which is a powerful, free, and cross-platform multimedia library. It can
 *  be found at http://www.libsdl.org/
 *
 * Support is in place or planned for the following sound formats:
 *   - .WAV  (Microsoft WAVfile RIFF data, internal.)
 *   - .VOC  (Creative Labs' Voice format, internal.)
 *   - .MP3  (MPEG-1 Layer 3 support, via the SMPEG library.)
 *   - .MID  (MIDI music converted to Waveform data, via Timidity.)
 *   - .MOD  (MOD files, via MikMod.)
 *   - .OGG  (Ogg files, via Ogg Vorbis libraries.)
 *   - .RAW  (Raw sound data in any format, internal.)
 *   - .CDA  (CD audio read into a sound buffer, internal.)
 *   - .AU
 *   - .AIFF
 *
 *   (...and more to come...)
 *
 * Please see the file COPYING in the source's root directory.
 *
 *  This file written by Ryan C. Gordon. (icculus@clutteredmind.org)
 */

#ifndef _INCLUDE_SDL_SOUND_H_
#define _INCLUDE_SDL_SOUND_H_

#include "SDL.h"
#include "SDL_endian.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef SDL_SOUND_DLL_EXPORTS
#  undef DECLSPEC
#  define DECLSPEC __declspec(dllexport)
#endif


#define SOUND_VER_MAJOR 0
#define SOUND_VER_MINOR 1
#define SOUND_VER_PATCH 4


/**
 * These are flags that are used in a Sound_Sample to show various states.
 *
 *   To use: "if (sample->flags & SOUND_SAMPLEFLAG_ERROR) { dosomething(); }"
 *
 *  @param SOUND_SAMPLEFLAG_NONE     null flag.
 *  @param SOUND_SAMPLEFLAG_NEEDSEEK SDL_RWops must be able to seek.
 *  @param SOUND_SAMPLEFLAG_EOF      end of input stream.
 *  @param SOUND_SAMPLEFLAG_ERROR    unrecoverable error.
 *  @param SOUND_SAMPLEFLAG_EAGAIN   function would block, or temp error.
 */
typedef enum __SOUND_SAMPLEFLAGS__
{
    SOUND_SAMPLEFLAG_NONE      = 0,

        /* these are set at sample creation time... */
    SOUND_SAMPLEFLAG_NEEDSEEK  = 1,

        /* these are set during decoding... */
    SOUND_SAMPLEFLAG_EOF       = 1 << 29,
    SOUND_SAMPLEFLAG_ERROR     = 1 << 30,
    SOUND_SAMPLEFLAG_EAGAIN    = 1 << 31
} Sound_SampleFlags;


/**
 * These are the basics of a decoded sample's data structure: data format
 *  (see AUDIO_U8 and friends in SDL_audio.h), number of channels, and sample
 *  rate. If you need more explanation than that, you should stop developing
 *  sound code right now.
 *
 *   @param format Equivalent of SDL_AudioSpec.format.
 *   @param channels Number of sound channels. 1 == mono, 2 == stereo.
 *   @param rate Sample rate; frequency of sample points per second (44100,
 *                22050, 8000, etc.)
 */
typedef struct __SOUND_AUDIOINFO__
{
    Uint16 format;
    Uint8 channels;
    Uint32 rate;
} Sound_AudioInfo;


/**
 * Each decoder sets up one of these structs, which can be retrieved via
 *  the Sound_AvailableDecoders() function. EVERY FIELD IN THIS IS READ-ONLY.
 *
 *   @param extensions File extensions, list ends with NULL. Read it like this:
 *           const char **ext;
 *           for (ext = info->extensions; *ext != NULL; ext++)
 *                printf("   File extension \"%s\"\n", *ext);
 *   @param description Human readable description of decoder.
 *   @param author "Name Of Author <email@emailhost.dom>"
 *   @param url URL specific to this decoder.
 */
typedef struct __SOUND_DECODERINFO__
{
    const char **extensions;
    const char *description;    
    const char *author;
    const char *url;
} Sound_DecoderInfo;



/**
 * The Sound_Sample structure is the heart of SDL_sound. This holds
 *  information about a source of sound data as it is being decoded.
 *  EVERY FIELD IN THIS IS READ-ONLY. Please use the API functions to
 *  change them.
 *
 *  @param opaque Internal use only. Don't touch.
 *  @param decoder Decoder used for this sample.
 *  @param desired Desired audio format for conversion.
 *  @param actual Actual audio format of sample.
 *  @param buffer Decoded sound data lands in here.
 *  @param buffer_size Current size of (buffer), in bytes (Uint8).
 *  @param flags Flags relating to this sample.
 */
typedef struct __SOUND_SAMPLE__
{
    void *opaque;
    const Sound_DecoderInfo *decoder;
    Sound_AudioInfo desired;
    Sound_AudioInfo actual;
    void *buffer;
    Uint32 buffer_size;
    Sound_SampleFlags flags;
} Sound_Sample;


/**
 * Just what it says: a major.minor.patch style version number...
 *
 *   @param major The major version number.
 *   @param minor The minor version number.
 *   @param patch The patchlevel version number.
 */
typedef struct __SOUND_VERSION__
{
    int major;
    int minor;
    int patch;
} Sound_Version;



/* functions and macros... */

#define SOUND_VERSION(x) { \
                           (x)->major = SOUND_VER_MAJOR; \
                           (x)->minor = SOUND_VER_MINOR; \
                           (x)->patch = SOUND_VER_PATCH; \
                         }

/**
 * Get the version of SDL_sound that is linked against your program. If you
 *  are using a shared library (DLL) version of SDL_sound, then it is possible
 *  that it will be different than the version you compiled against.
 *
 * This is a real function; the macro SOUND_VERSION tells you what version
 *  of SDL_sound you compiled against:
 *
 * Sound_Version compiled;
 * Sound_Version linked;
 *
 * SOUND_VERSION(&compiled);
 * Sound_GetLinkedVersion(&linked);
 * printf("We compiled against SDL_sound version %d.%d.%d ...\n",
 *           compiled.major, compiled.minor, compiled.patch);
 * printf("But we linked against SDL_sound version %d.%d.%d.\n",
 *           linked.major, linked.minor, linked.patch);
 *
 * This function may be called safely at any time, even before Sound_Init().
 *
 * @param ver Sound_Version structure to fill with shared library's version.
 */
extern DECLSPEC void Sound_GetLinkedVersion(Sound_Version *ver);


/**
 * Initialize SDL_sound. This must be called before any other SDL_sound
 *  function (except perhaps Sound_GetLinkedVersion()). You should call
 *  SDL_Init() before calling this. Sound_Init() will attempt to call
 *  SDL_Init(SDL_INIT_AUDIO), just in case. This is a safe behaviour, but it
 *  may not configure SDL to your liking by itself.
 *
 *  @returns nonzero on success, zero on error. Specifics of the
 *           error can be gleaned from Sound_GetError().
 */
extern DECLSPEC int Sound_Init(void);


/**
 * Shutdown SDL_sound. This closes any SDL_RWops that were being used as
 *  sound sources, and frees any resources in use by SDL_sound.
 *
 * All Sound_Sample pointers you had prior to this call are INVALIDATED.
 *
 * Once successfully deinitialized, Sound_Init() can be called again to
 *  restart the subsystem. All default API states are restored at this
 *  point.
 *
 * You should call this BEFORE SDL_Quit(). This will NOT call SDL_Quit()
 *  for you!
 *
 *  @returns nonzero on success, zero on error. Specifics of the error
 *           can be gleaned from Sound_GetError(). If failure, state of
 *           SDL_sound is undefined, and probably badly screwed up.
 */
extern DECLSPEC int Sound_Quit(void);


/**
 * Get a list of sound formats supported by this implementation of SDL_sound.
 *  This is for informational purposes only. Note that the extension listed is
 *  merely convention: if we list "MP3", you can open an MPEG-1 Layer 3 audio
 *  file with an extension of "XYZ", if you like. The file extensions are
 *  informational, and only required as a hint to choosing the correct
 *  decoder, since the sound data may not be coming from a file at all, thanks
 *  to the abstraction that an SDL_RWops provides.
 *
 * The returned value is an array of pointers to Sound_DecoderInfo structures,
 *  with a NULL entry to signify the end of the list:
 *
 * Sound_DecoderInfo **i;
 *
 * for (i = Sound_AvailableDecoders(); *i != NULL; i++)
 * {
 *     printf("Supported sound format: [%s], which is [%s].\n",
 *              i->extension, i->description);
 *     // ...and other fields...
 * }
 *
 * The return values are pointers to static internal memory, and should
 *  be considered READ ONLY, and never freed.
 *
 *  @returns READ ONLY Null-terminated array of READ ONLY structures.
 */
extern DECLSPEC const Sound_DecoderInfo **Sound_AvailableDecoders(void);


/**
 * Get the last SDL_sound error message as a null-terminated string.
 *  This will be NULL if there's been no error since the last call to this
 *  function. The pointer returned by this call points to an internal buffer.
 *  Each thread has a unique error state associated with it, but each time
 *  a new error message is set, it will overwrite the previous one associated
 *  with that thread. It is safe to call this function at anytime, even
 *  before Sound_Init().
 *
 *  @returns READ ONLY string of last error message.
 */
extern DECLSPEC const char *Sound_GetError(void);


/**
 * Clear the current error message, so the next call to Sound_GetError() will
 *  return NULL.
 */
extern DECLSPEC void Sound_ClearError(void);


/**
 * Start decoding a new sound sample. The data is read via an SDL_RWops
 *  structure (see SDL_rwops.h in the SDL include directory), so it may be
 *  coming from memory, disk, network stream, etc. The (ext) parameter is
 *  merely a hint to determining the correct decoder; if you specify, for
 *  example, "mp3" for an extension, and one of the decoders lists that
 *  as a handled extension, then that decoder is given first shot at trying
 *  to claim the data for decoding. If none of the extensions match (or the
 *  extension is NULL), then every decoder examines the data to determine if
 *  it can handle it, until one accepts it. In such a case your SDL_RWops will
 *  need to be capable of rewinding to the start of the stream.
 * If no decoders can handle the data, a NULL value is returned, and a human
 *  readable error message can be fetched from Sound_GetError().
 * Optionally, a desired audio format can be specified. If the incoming data
 *  is in a different format, SDL_sound will convert it to the desired format
 *  on the fly. Note that this can be an expensive operation, so it may be
 *  wise to convert data before you need to play it back, if possible, or
 *  make sure your data is initially in the format that you need it in.
 *  If you don't want to convert the data, you can specify NULL for a desired
 *  format. The incoming format of the data, preconversion, can be found
 *  in the Sound_Sample structure.
 * Note that the raw sound data "decoder" needs you to specify both the
 *  extension "RAW" and a "desired" format, or it will refuse to handle
 *  the data. This is to prevent it from catching all formats unsupported
 *  by the other decoders.
 * Finally, specify an initial buffer size; this is the number of bytes that
 *  will be allocated to store each read from the sound buffer. The more you
 *  can safely allocate, the more decoding can be done in one block, but the
 *  more resources you have to use up, and the longer each decoding call will
 *  take. Note that different data formats require more or less space to
 *  store. This buffer can be resized via Sound_SetBufferSize() ...
 * The buffer size specified must be a multiple of the size of a single
 *  sample point. So, if you want 16-bit, stereo samples, then your sample
 *  point size is (2 channels * 16 bits), or 32 bits per sample, which is four
 *  bytes. In such a case, you could specify 128 or 132 bytes for a buffer,
 *  but not 129, 130, or 131 (although in reality, you'll want to specify a
 *  MUCH larger buffer).
 * When you are done with this Sound_Sample pointer, you can dispose of it
 *  via Sound_FreeSample().
 * You do not have to keep a reference to (rw) around. If this function
 *  suceeds, it stores (rw) internally (and disposes of it during the call
 *  to Sound_FreeSample()). If this function fails, it will dispose of the
 *  SDL_RWops for you.
 *
 *    @param rw SDL_RWops with sound data.
 *    @param ext File extension normally associated with a data format.
 *               Can usually be NULL.
 *    @param desired Format to convert sound data into. Can usually be NULL,
 *                   if you don't need conversion.
 *   @returns Sound_Sample pointer, which is used as a handle to several other
 *            SDL_sound APIs. NULL on error. If error, use
 *            Sound_GetError() to see what went wrong.
 */
extern DECLSPEC Sound_Sample *Sound_NewSample(SDL_RWops *rw, const char *ext,
                                              Sound_AudioInfo *desired,
                                              Uint32 bufferSize);

/**
 * This is identical to Sound_NewSample(), but it creates an SDL_RWops for you
 *  from the file located in (filename). Note that (filename) is specified in
 *  platform-dependent notation. ("C:\\music\\mysong.mp3" on windows, and
 *  "/home/icculus/music/mysong.mp3" or whatever on Unix, etc.)
 * Sound_NewSample()'s "ext" parameter is gleaned from the contents of
 *  (filename).
 *
 *    @param filename file containing sound data.
 *    @param desired Format to convert sound data into. Can usually be NULL,
 *                   if you don't need conversion.
 *    @param bufferSize size, in bytes, of initial read buffer.
 *   @returns Sound_Sample pointer, which is used as a handle to several other
 *            SDL_sound APIs. NULL on error. If error, use
 *            Sound_GetError() to see what went wrong.
 */
extern DECLSPEC Sound_Sample *Sound_NewSampleFromFile(const char *filename,
                                                      Sound_AudioInfo *desired,
                                                      Uint32 bufferSize);

/**
 * Dispose of a Sound_Sample pointer that was returned from Sound_NewSample().
 *  This will also close/dispose of the SDL_RWops that was used at creation
 *  time, so there's no need to keep a reference to that around.
 * The Sound_Sample pointer is invalid after this call, and will almost
 *  certainly result in a crash if you attempt to keep using it.
 *
 *    @param sample The Sound_Sample to delete.
 */
extern DECLSPEC void Sound_FreeSample(Sound_Sample *sample);


/**
 * Change the current buffer size for a sample. If the buffer size could
 *  be changed, then the sample->buffer and sample->buffer_size fields will
 *  reflect that. If they could not be changed, then your original sample
 *  state is preserved. If the buffer is shrinking, the data at the end of
 *  buffer is truncated. If the buffer is growing, the contents of the new
 *  space at the end is undefined until you decode more into it or initialize
 *  it yourself.
 *
 * The buffer size specified must be a multiple of the size of a single
 *  sample point. So, if you want 16-bit, stereo samples, then your sample
 *  point size is (2 channels * 16 bits), or 32 bits per sample, which is four
 *  bytes. In such a case, you could specify 128 or 132 bytes for a buffer,
 *  but not 129, 130, or 131 (although in reality, you'll want to specify a
 *  MUCH larger buffer).
 *
 *    @param sample The Sound_Sample whose buffer to modify.
 *    @param new_size The desired size, in bytes, of the new buffer.
 *  @returns non-zero if buffer size changed, zero on failure.
 */
extern DECLSPEC int Sound_SetBufferSize(Sound_Sample *sample, Uint32 new_size);


/**
 * Decode more of the sound data in a Sound_Sample. It will decode at most
 *  sample->buffer_size bytes into sample->buffer in the desired format, and
 *  return the number of decoded bytes.
 * If sample->buffer_size bytes could not be decoded, then please refer to
 *  sample->flags to determine if this was an End-of-stream or error condition.
 *
 *    @param sample Do more decoding to this Sound_Sample.
 *  @returns number of bytes decoded into sample->buffer. If it is less than
 *           sample->buffer_size, then you should check sample->flags to see
 *           what the current state of the sample is (EOF, error, read again).
 */
extern DECLSPEC Uint32 Sound_Decode(Sound_Sample *sample);


/**
 * Decode the remainder of the sound data in a Sound_Sample. This will
 *  dynamically allocate memory for the ENTIRE remaining sample.
 *  sample->buffer_size and sample->buffer will be updated to reflect the
 *  new buffer. Please refer to sample->flags to determine if the decoding
 *  finished due to an End-of-stream or error condition.
 *
 * Be aware that sound data can take a large amount of memory, and that
 *  this function may block for quite awhile while processing. Also note
 *  that a streaming source (for example, from a SDL_RWops that is getting
 *  fed from an Internet radio feed that doesn't end) may fill all available
 *  memory before giving up...be sure to use this on finite sound sources
 *  only!
 *
 * When decoding the sample in its entirety, the work is done one buffer at a
 *  time. That is, sound is decoded in sample->buffer_size blocks, and
 *  appended to a continually-growing buffer until the decoding completes.
 *  That means that this function will need enough RAM to hold approximately
 *  sample->buffer_size bytes plus the complete decoded sample at most. The
 *  larger your buffer size, the less overhead this function needs, but beware
 *  the possibility of paging to disk. Best to make this user-configurable if
 *  the sample isn't specific and small.
 *
 *    @param sample Do all decoding for this Sound_Sample.
 *   @returns number of bytes decoded into sample->buffer. You should check
 *           sample->flags to see what the current state of the sample is
 *           (EOF, error, read again).
 */
extern DECLSPEC Uint32 Sound_DecodeAll(Sound_Sample *sample);

#ifdef __cplusplus
}
#endif

#endif  /* !defined _INCLUDE_SDL_SOUND_H_ */

/* end of SDL_sound.h ... */

