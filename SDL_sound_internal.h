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

/*
 * Internal function/structure declaration. Do NOT include in your
 *  application.
 *
 * Please see the file LICENSE in the source's root directory.
 *
 *  This file written by Ryan C. Gordon. (icculus@clutteredmind.org)
 */

#ifndef _INCLUDE_SDL_SOUND_INTERNAL_H_
#define _INCLUDE_SDL_SOUND_INTERNAL_H_

#ifndef __SDL_SOUND_INTERNAL__
#error Do not include this header from your applications.
#endif

#include "SDL.h"

#if (defined DEBUG_CHATTER)
#define _D(x) printf x
#else
#define _D(x)
#endif

typedef struct __SOUND_DECODERFUNCTIONS__
{
        /* This is a block of info about your decoder. See SDL_sound.h. */
    const Sound_DecoderInfo info;

        /*
         * Returns non-zero if (sample) has a valid fileformat that this
         *  driver can handle. Zero if this driver can NOT handle the data.
         *
         * Extension, which may be NULL, is just a hint as to the form of
         *  data that is being passed in. Most decoders should determine if
         *  they can handle the data by the data itself, but others, like
         *  the raw data handler, need this hint to know if they should
         *  accept the data in the first place.
         *
         * (sample)'s (opaque) field should be cast to a Sound_SampleInternal
         *  pointer:
         *
         *   Sound_SampleInternal *internal;
         *   internal = (Sound_SampleInternal *) sample->opaque;
         *
         * Certain fields of sample will be filled in for the decoder before
         *  this call, and others should be filled in by the decoder. Some
         *  fields are offlimits, and should NOT be modified. The list:
         *
         * in Sound_SampleInternal section:
         *    Sound_Sample *next;  (offlimits)
         *    Sound_Sample *prev;  (offlimits)
         *    SDL_RWops *rw;       (can use, but do NOT close it)
         *    const Sound_DecoderFunctions *funcs; (that's this structure)
         *    SDL_AudioCVT sdlcvt; (offlimits)
         *    void *buffer;        (offlimits until read() method)
         *    Uint32 buffer_size;  (offlimits until read() method)
         *    void *decoder_private; (read and write access)
         *
         * in rest of Sound_Sample:
         *    void *opaque;        (this was internal section, above)
         *    const Sound_DecoderInfo *decoder;  (read only)
         *    Sound_AudioInfo desired; (read only, usually not needed here)
         *    Sound_AudioInfo actual;  (please fill this in)
         *    void *buffer;            (offlimits)
         *    Uint32 buffer_size;      (offlimits)
         *    Sound_SampleFlags flags; (set appropriately)
         */
    int (*open)(Sound_Sample *sample, const char *ext);

        /*
         * Clean up. SDL_sound is done with this sample, so the decoder should
         *  clean up any resources it allocated. Anything that wasn't
         *  explicitly allocated by the decoder should be LEFT ALONE, since
         *  the higher-level SDL_sound layer will clean up its own mess.
         */
    void (*close)(Sound_Sample *sample);

        /*
         * Get more data from (sample). The decoder should get a pointer to
         *  the internal structure...
         *
         *   Sound_SampleInternal *internal;
         *   internal = (Sound_SampleInternal *) sample->opaque;
         *
         *  ...and then start decoding. Fill in up to internal->buffer_size
         *  bytes of decoded sound in the space pointed to by
         *  internal->buffer. The encoded data is read in from internal->rw.
         *  Data should be decoded in the format specified during the
         *  decoder's open() method in the sample->actual field. The
         *  conversion to the desired format is done at a higher level.
         *
         * The return value is the number of bytes decoded into
         *  internal->buffer, which can be no more than internal->buffer_size,
         *  but can be less. If it is less, you should set a state flag:
         *
         *   If there's just no more data (end of file, etc), then do:
         *      sample->flags |= SOUND_SAMPLEFLAG_EOF;
         *
         *   If there's an unrecoverable error, then do:
         *      Sound_SetError(ERR_EXPLAIN_WHAT_WENT_WRONG);
         *      sample->flags |= SOUND_SAMPLEFLAG_ERROR;
         *
         *   If there's more data, but you'd have to block for considerable
         *    amounts of time to get at it, or there's a recoverable error,
         *    then do:
         *      Sound_SetError(ERR_EXPLAIN_WHAT_WENT_WRONG);
         *      sample->flags |= SOUND_SAMPLEFLAG_EAGAIN;
         *
         * SDL_sound will not call your read() method for any samples with
         *  SOUND_SAMPLEFLAG_EOF or SOUND_SAMPLEFLAG_ERROR set. The
         *  SOUND_SAMPLEFLAG_EAGAIN flag is reset before each call to this
         *  method.
         */
    Uint32 (*read)(Sound_Sample *sample);
} Sound_DecoderFunctions;


/* (for now. --ryan.) */
#define MULTIPLE_STREAMS_PER_RWOPS 1



typedef struct __SOUND_SAMPLEINTERNAL__
{
    Sound_Sample *next;
    Sound_Sample *prev;
    SDL_RWops *rw;
#if (defined MULTIPLE_STREAMS_PER_RWOPS)
    int pos; /* !!! FIXME: Int? Really? */
#endif
    const Sound_DecoderFunctions *funcs;
    SDL_AudioCVT sdlcvt;
    void *buffer;
    Uint32 buffer_size;
    void *decoder_private;
} Sound_SampleInternal;



/* error messages... */
#define ERR_IS_INITIALIZED       "Already initialized"
#define ERR_NOT_INITIALIZED      "Not initialized"
#define ERR_INVALID_ARGUMENT     "Invalid argument"
#define ERR_OUT_OF_MEMORY        "Out of memory"
#define ERR_NOT_SUPPORTED        "Operation not supported"
#define ERR_UNSUPPORTED_FORMAT   "Sound format unsupported"
#define ERR_NOT_A_HANDLE         "Not a file handle"
#define ERR_NO_SUCH_FILE         "No such file"
#define ERR_PAST_EOF             "Past end of file"
#define ERR_IO_ERROR             "I/O error"
#define ERR_COMPRESSION          "(De)compression error"
#define ERR_PREV_ERROR           "Previous decoding already caused an error"
#define ERR_PREV_EOF             "Previous decoding already triggered EOF"

/*
 * Call this to set the message returned by Sound_GetError().
 *  Please only use the ERR_* constants above, or add new constants to the
 *  above group, but I want these all in one place.
 *
 * Calling this with a NULL argument is a safe no-op.
 */
void Sound_SetError(const char *err);


/*
 * Use this if you need a cross-platform stricmp().
 */
int __Sound_strcasecmp(const char *x, const char *y);


/* These get used all over for lessening code clutter. */
#define BAIL_MACRO(e, r) { Sound_SetError(e); return r; }
#define BAIL_IF_MACRO(c, e, r) if (c) { Sound_SetError(e); return r; }




/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
/*------------                                              ----------------*/
/*------------  You MUST implement the following functions  ----------------*/
/*------------        if porting to a new platform.         ----------------*/
/*------------     (see platform/unix.c for an example)     ----------------*/
/*------------                                              ----------------*/
/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/


/* (None, right now.)  */


#ifdef __cplusplus
extern "C" {
#endif

#endif /* defined _INCLUDE_SDL_SOUND_INTERNAL_H_ */

/* end of SDL_sound_internal.h ... */

