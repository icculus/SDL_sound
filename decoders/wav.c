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
 * WAV decoder for SDL_sound.
 *
 * This driver handles Microsoft .WAVs, in as many of the thousands of
 *  variations as we can.
 *
 * Please see the file LICENSE in the source's root directory.
 *
 *  This file written by Ryan C. Gordon. (icculus@clutteredmind.org)
 */

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#ifdef SOUND_SUPPORTS_WAV

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "SDL_sound.h"

#define __SDL_SOUND_INTERNAL__
#include "SDL_sound_internal.h"

static int WAV_init(void);
static void WAV_quit(void);
static int WAV_open(Sound_Sample *sample, const char *ext);
static void WAV_close(Sound_Sample *sample);
static Uint32 WAV_read(Sound_Sample *sample);

const Sound_DecoderFunctions __Sound_DecoderFunctions_WAV =
{
    {
        "WAV",
        "Microsoft WAVE audio format",
        "Ryan C. Gordon <icculus@clutteredmind.org>",
        "http://www.icculus.org/SDL_sound/"
    },

    WAV_init,       /*  init() method */
    WAV_quit,       /*  quit() method */
    WAV_open,       /*  open() method */
    WAV_close,      /* close() method */
    WAV_read        /*  read() method */
};


    /* this is what we store in our internal->decoder_private field... */
typedef struct
{
    Sint32 bytesLeft;
} wav_t;


    /* Chunk management code... */

#define riffID 0x46464952  /* "RIFF", in ascii. */
#define waveID 0x45564157  /* "WAVE", in ascii. */


#define fmtID  0x20746D66  /* "fmt ", in ascii. */

#define FMT_NORMAL 1    /* Uncompressed waveform data. */

typedef struct
{
    Uint32 chunkID;
    Sint32 chunkSize;
    Sint16 wFormatTag;
    Uint16 wChannels;
    Uint32 dwSamplesPerSec;
    Uint32 dwAvgBytesPerSec;
    Uint16 wBlockAlign;
    Uint16 wBitsPerSample;
} fmt_t;


static int WAV_init(void)
{
    return(1);  /* always succeeds. */
} /* WAV_init */


static void WAV_quit(void)
{
    /* it's a no-op. */
} /* WAV_quit */


/*
 * Read in a fmt_t from disk. This makes this process safe regardless of
 *  the processor's byte order or how the fmt_t structure is packed.
 */
static int read_fmt_chunk(SDL_RWops *rw, fmt_t *fmt)
{
    /* skip reading the chunk ID, since it was already read at this point... */
    fmt->chunkID = fmtID;

    if (SDL_RWread(rw, &fmt->chunkSize, sizeof (fmt->chunkSize), 1) != 1)
        return(0);
    fmt->chunkSize = SDL_SwapLE32(fmt->chunkSize);

    if (SDL_RWread(rw, &fmt->wFormatTag, sizeof (fmt->wFormatTag), 1) != 1)
        return(0);
    fmt->wFormatTag = SDL_SwapLE16(fmt->wFormatTag);

    if (SDL_RWread(rw, &fmt->wChannels, sizeof (fmt->wChannels), 1) != 1)
        return(0);
    fmt->wChannels = SDL_SwapLE16(fmt->wChannels);

    if (SDL_RWread(rw, &fmt->dwSamplesPerSec,
                   sizeof (fmt->dwSamplesPerSec), 1) != 1)
        return(0);
    fmt->dwSamplesPerSec = SDL_SwapLE32(fmt->dwSamplesPerSec);

    if (SDL_RWread(rw, &fmt->dwAvgBytesPerSec,
                   sizeof (fmt->dwAvgBytesPerSec), 1) != 1)
        return(0);
    fmt->dwAvgBytesPerSec = SDL_SwapLE32(fmt->dwAvgBytesPerSec);

    if (SDL_RWread(rw, &fmt->wBlockAlign, sizeof (fmt->wBlockAlign), 1) != 1)
        return(0);
    fmt->wBlockAlign = SDL_SwapLE16(fmt->wBlockAlign);

    if (SDL_RWread(rw, &fmt->wBitsPerSample,
                   sizeof (fmt->wBitsPerSample), 1) != 1)
        return(0);
    fmt->wBitsPerSample = SDL_SwapLE16(fmt->wBitsPerSample);

    return(1);
} /* read_fmt_chunk */


#define dataID 0x61746164  /* "data", in ascii. */

typedef struct
{
    Uint32 chunkID;
    Sint32 chunkSize;
    /* Then, (chunkSize) bytes of waveform data... */
} data_t;


/*
 * Read in a fmt_t from disk. This makes this process safe regardless of
 *  the processor's byte order or how the fmt_t structure is packed.
 */
static int read_data_chunk(SDL_RWops *rw, data_t *data)
{
    /* skip reading the chunk ID, since it was already read at this point... */
    data->chunkID = dataID;

    if (SDL_RWread(rw, &data->chunkSize, sizeof (data->chunkSize), 1) != 1)
        return(0);
    data->chunkSize = SDL_SwapLE32(data->chunkSize);

    return(1);
} /* read_data_chunk */


static int find_chunk(SDL_RWops *rw, Uint32 id)
{
    Sint32 siz = 0;
    Uint32 _id = 0;

    while (1)
    {
        BAIL_IF_MACRO(SDL_RWread(rw, &_id, sizeof (_id), 1) != 1, NULL, 0);
        if (SDL_SwapLE32(_id) == id)
            return(1);

        BAIL_IF_MACRO(SDL_RWread(rw, &siz, sizeof (siz), 1) != 1, NULL, 0);
        siz = SDL_SwapLE32(siz);
        assert(siz > 0);
        BAIL_IF_MACRO(SDL_RWseek(rw, siz, SEEK_SET) != siz, NULL, 0);
    } /* while */

    return(0);  /* shouldn't hit this, but just in case... */
} /* find_chunk */


static int WAV_open(Sound_Sample *sample, const char *ext)
{
    Sound_SampleInternal *internal = (Sound_SampleInternal *) sample->opaque;
    SDL_RWops *rw = internal->rw;
    fmt_t f;
    data_t d;
    wav_t *w;

    BAIL_IF_MACRO(SDL_ReadLE32(rw) != riffID, "WAV: Not a RIFF file.", 0);
	SDL_ReadLE32(rw);  /* throw the length away; we get this info later. */
    BAIL_IF_MACRO(SDL_ReadLE32(rw) != waveID, "WAV: Not a WAVE file.", 0);
    BAIL_IF_MACRO(!find_chunk(rw, fmtID), "WAV: No format chunk.", 0);
    BAIL_IF_MACRO(!read_fmt_chunk(rw, &f), "WAV: Can't read format chunk.", 0);

    /* !!! FIXME: This will have to change for compression types... */
    BAIL_IF_MACRO(f.wFormatTag != FMT_NORMAL, "WAV: Unsupported encoding.", 0);

    sample->actual.channels = (Uint8) f.wChannels;
    sample->actual.rate = f.dwSamplesPerSec;

    if (f.wBitsPerSample <= 8)
        sample->actual.format = AUDIO_U8;
    else if (f.wBitsPerSample <= 16)
        sample->actual.format = AUDIO_S16LSB;
    else
        BAIL_MACRO("WAV: Unsupported sample size.", 0);

    BAIL_IF_MACRO(!find_chunk(rw, dataID), "WAV: No data chunk.", 0);
    BAIL_IF_MACRO(!read_data_chunk(rw, &d), "WAV: Can't read data chunk.", 0);

    w = (wav_t *) malloc(sizeof(wav_t));
    BAIL_IF_MACRO(w == NULL, ERR_OUT_OF_MEMORY, 0);
    w->bytesLeft = d.chunkSize;
    internal->decoder_private = (void *) w;

    sample->flags = SOUND_SAMPLEFLAG_NONE;

    SNDDBG(("WAV: Accepting data stream.\n"));
    return(1); /* we'll handle this data. */
} /* WAV_open */


static void WAV_close(Sound_Sample *sample)
{
    Sound_SampleInternal *internal = (Sound_SampleInternal *) sample->opaque;
    free(internal->decoder_private);
} /* WAV_close */


static Uint32 WAV_read(Sound_Sample *sample)
{
    Uint32 retval;
    Sound_SampleInternal *internal = (Sound_SampleInternal *) sample->opaque;
    wav_t *w = (wav_t *) internal->decoder_private;
    Uint32 max = (internal->buffer_size < (Uint32) w->bytesLeft) ?
                    internal->buffer_size : (Uint32) w->bytesLeft;

    assert(max > 0);

        /*
         * We don't actually do any decoding, so we read the wav data
         *  directly into the internal buffer...
         */
    retval = SDL_RWread(internal->rw, internal->buffer, 1, max);

    w->bytesLeft -= retval;

        /* Make sure the read went smoothly... */
    if ((retval == 0) || (w->bytesLeft == 0))
        sample->flags |= SOUND_SAMPLEFLAG_EOF;

    else if (retval == -1)
        sample->flags |= SOUND_SAMPLEFLAG_ERROR;

        /* (next call this EAGAIN may turn into an EOF or error.) */
    else if (retval < internal->buffer_size)
        sample->flags |= SOUND_SAMPLEFLAG_EAGAIN;

    return(retval);
} /* WAV_read */

#endif /* SOUND_SUPPORTS_WAV */

/* end of wav.c ... */

