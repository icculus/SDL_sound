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
 * AIFF decoder for SDL_sound
 *
 * [Insert something profound about the AIFF file format here.]
 *
 * This code was ripped from a decoder I had written for SDL_mixer, which was
 * based on SDL_mixer's old AIFF music loader. (This loader was unfortunately
 * completely broken, but it was still useful because all the pieces were
 * still there, so to speak.)
 *
 * When rewriting it for SDL_sound, I changed its structure to be more like
 * the WAV loader Ryan wrote. Had they not both been part of the same project
 * it would have been embarrassing how similar they are.
 *
 * It is not the most feature-complete AIFF loader the world has ever seen.
 * For instance, it only makes a token attempt at implementing the AIFF-C
 * standard; basically the parts of it that I can easily understand and test.
 * It's a start, though.
 *
 * Please see the file LICENSE in the source's root directory.
 *
 *  This file was written by Torbjörn Andersson. (d91tan@Update.UU.SE)
 */

#ifdef SOUND_SUPPORTS_AIFF

#define __SDL_SOUND_INTERNAL__
#include "SDL_sound_internal.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "SDL.h"
#include "SDL_endian.h"
#include "SDL_sound.h"

static int AIFF_init(void);
static void AIFF_quit(void);
static int AIFF_open(Sound_Sample *sample, const char *ext);
static void AIFF_close(Sound_Sample *sample);
static Uint32 AIFF_read(Sound_Sample *sample);

const Sound_DecoderFunctions __Sound_DecoderFunctions_AIFF =
{
    {
        "AIFF",
        "Audio Interchange File Format",
        "Torbjörn Andersson <d91tan@Update.UU.SE>",
        "http://www.icculus.org/SDL_sound/"
    },

    AIFF_init,      /*  init() method */
    AIFF_quit,      /*  quit() method */
    AIFF_open,      /*  open() method */
    AIFF_close,     /* close() method */
    AIFF_read       /*  read() method */
};


    /* this is what we store in our internal->decoder_private field... */
typedef struct {
    Sint32 bytesLeft;
} aiff_t;


    /* Chunk management code... */

#define formID 0x4d524f46  /* "FORM", in ascii. */
#define aiffID 0x46464941  /* "AIFF", in ascii. */
#define aifcID 0x43464941  /* "AIFC", in ascii. */
#define ssndID 0x444e5353  /* "SSND", in ascii. */
#define commID 0x4d4d4f43  /* "COMM", in ascii. */

#define noneID 0x454e4f4e  /* "NONE", in ascii. */

typedef struct
{
    Uint32 ckID;
    Uint32 ckDataSize;
    Uint16 numChannels;
    Uint32 numSampleFrames;
    Uint16 sampleSize;
    Uint32 sampleRate;
        /*
         * We don't handle AIFF-C compressed audio yet, but for those
         * interested the allowed compression types are supposed to be
         *
         *   compressionType   compressionName   meaning
         *   ---------------------------------------------------------------
         *   'NONE'            "not compressed"  uncompressed, that is,
         *                                        straight digitized samples
         *   'ACE2'            "ACE 2-to-1"      2-to-1 IIGS ACE (Audio
         *                                        Compression / Expansion)
         *   'ACE8'            "ACE 8-to-3"      8-to-3 IIGS ACE (Audio
         *                                        Compression / Expansion)
         *   'MAC3'            "MACE 3-to-1"     3-to-1 Macintosh Audio
         *                                        Compression / Expansion
         *   'MAC6'            "MACE 6-to-1"     6-to-1 Macintosh Audio
         *                                        Compression / Expansion
         *
         * A pstring is a "Pascal-style string", that is, "one byte followed
         * by test bytes followed when needed by one pad byte. The total
         * number of bytes in a pstring must be even. The pad byte is
         * included when the number of text bytes is even, so the total of
         * text bytes + one count byte + one pad byte will be even. This pad
         * byte is not reflected in the count."
         *
         * As for how these compression algorithms work, your guess is as
         * good as mine.
         */
    Uint32 compressionType;
#if 0
    pstring compressionName;
#endif
} comm_t;



static int AIFF_init(void)
{
    return(1);  /* always succeeds. */
} /* AIFF_init */


static void AIFF_quit(void)
{
    /* it's a no-op. */
} /* AIFF_quit */


/* 
 * Sample rate is encoded as an "80 bit IEEE Standard 754 floating point
 * number (Standard Apple Numeric Environment [SANE] data type Extended)".
 * Whose bright idea was that?
 *
 * This function was adapted from libsndfile, and while I do know a little
 * bit about the IEEE floating point standard I don't pretend to fully
 * understand this.
 */

static Uint32 SANE_to_Uint32 (Uint8 *sanebuf)
{
    /* Is the frequency outside of what we can represent with Uint32? */
    if ( (sanebuf[0] & 0x80)
      || (sanebuf[0] <= 0x3F)
      || (sanebuf[0] > 0x40)
      || (sanebuf[0] == 0x40 && sanebuf[1] > 0x1C) )
        return 0;

    return ((sanebuf[2] << 23) | (sanebuf[3] << 15) | (sanebuf[4] << 7)
        | (sanebuf[5] >> 1)) >> (29 - sanebuf[1]);
} /* SANE_to_Uint32 */


/*
 * Read in a comm_t from disk. This makes this process safe regardless of
 *  the processor's byte order or how the comm_t structure is packed.
 */

static int read_comm_chunk(SDL_RWops *rw, comm_t *comm)
{
    Uint8 sampleRate[10];

    /* skip reading the chunk ID, since it was already read at this point... */
    comm->ckID = commID;

    if (SDL_RWread(rw, &comm->ckDataSize, sizeof (comm->ckDataSize), 1) != 1)
        return(0);
    comm->ckDataSize = SDL_SwapBE32(comm->ckDataSize);

    if (SDL_RWread(rw, &comm->numChannels, sizeof (comm->numChannels), 1) != 1)
        return(0);
    comm->numChannels = SDL_SwapBE16(comm->numChannels);

    if (SDL_RWread(rw, &comm->numSampleFrames,
                   sizeof (comm->numSampleFrames), 1) != 1)
        return(0);
    comm->numSampleFrames = SDL_SwapBE32(comm->numSampleFrames);

    if (SDL_RWread(rw, &comm->sampleSize, sizeof (comm->sampleSize), 1) != 1)
        return(0);
    comm->sampleSize = SDL_SwapBE16(comm->sampleSize);

    if (SDL_RWread(rw, sampleRate, sizeof (sampleRate), 1) != 1)
        return(0);
    comm->sampleRate = SANE_to_Uint32(sampleRate);

    if (comm->ckDataSize > sizeof(comm->numChannels)
                         + sizeof(comm->numSampleFrames)
                         + sizeof(comm->sampleSize)
                         + sizeof(sampleRate))
    {
        if (SDL_RWread(rw, &comm->compressionType,
                       sizeof (comm->compressionType), 1) != 1)
            return(0);
        comm->compressionType = SDL_SwapBE32(comm->compressionType);
    } /* if */
    else
        comm->compressionType = noneID;

    return(1);
} /* read_comm_chunk */

typedef struct
{
    Uint32 ckID;
    Uint32 ckDataSize;
    Uint32 offset;
    Uint32 blockSize;
    /*
     * Then, comm->numSampleFrames sample frames. (It's better to get the
     * length from numSampleFrames than from ckDataSize.)
     */
} ssnd_t;


static int read_ssnd_chunk(SDL_RWops *rw, ssnd_t *ssnd)
{
    /* skip reading the chunk ID, since it was already read at this point... */
    ssnd->ckID = ssndID;

    if (SDL_RWread(rw, &ssnd->ckDataSize, sizeof (ssnd->ckDataSize), 1) != 1)
        return(0);
    ssnd->ckDataSize = SDL_SwapBE32(ssnd->ckDataSize);

    if (SDL_RWread(rw, &ssnd->offset, sizeof (ssnd->offset), 1) != 1)
        return(0);
    ssnd->offset = SDL_SwapBE32(ssnd->offset);

    if (SDL_RWread(rw, &ssnd->blockSize, sizeof (ssnd->blockSize), 1) != 1)
        return(0);
    ssnd->blockSize = SDL_SwapBE32(ssnd->blockSize);

    /* Leave the SDL_RWops position indicator at the start of the samples */
    /* !!! FIXME: Int? Really? */
    if (SDL_RWseek(rw, (int) ssnd->offset, SEEK_CUR) == -1)
        return(0);

    return(1);
} /* read_ssnd_chunk */


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
        siz = SDL_SwapBE32(siz);
        assert(siz > 0);
        BAIL_IF_MACRO(SDL_RWseek(rw, siz, SEEK_CUR) == -1, NULL, 0);
    } /* while */

    return(0);  /* shouldn't hit this, but just in case... */
} /* find_chunk */


static int AIFF_open(Sound_Sample *sample, const char *ext)
{
    Sound_SampleInternal *internal = (Sound_SampleInternal *) sample->opaque;
    SDL_RWops *rw = internal->rw;
    Uint32 chunk_id;
    int bytes_per_sample;
    long pos;
    comm_t c;
    ssnd_t s;
    aiff_t *a;

    BAIL_IF_MACRO(SDL_ReadLE32(rw) != formID, "AIFF: Not a FORM file.", 0);
        SDL_ReadBE32(rw);  /* throw the length away; we don't need it. */

    chunk_id = SDL_ReadLE32(rw);
    BAIL_IF_MACRO(chunk_id != aiffID && chunk_id != aifcID,
        "AIFF: Not an AIFF or AIFC file.", 0);

    /* Chunks may appear in any order, so we establish base camp here. */
    pos = SDL_RWtell(rw);

    BAIL_IF_MACRO(!find_chunk(rw, commID), "AIFF: No common chunk.", 0);
    BAIL_IF_MACRO(!read_comm_chunk(rw, &c),
                  "AIFF: Can't read common chunk.", 0);

    /* !!! FIXME: This will have to change for compression types... */
    BAIL_IF_MACRO(c.compressionType != noneID,
                  "AIFF: Unsupported encoding.", 0);

    BAIL_IF_MACRO(c.sampleRate == 0, "AIFF: Unsupported sample rate.", 0);

    sample->actual.channels = (Uint8) c.numChannels;
    sample->actual.rate = c.sampleRate;

    if (c.sampleSize <= 8)
    {
        sample->actual.format = AUDIO_S8;
        bytes_per_sample = 1;
    } /* if */
    else if (c.sampleSize <= 16)
    {
        sample->actual.format = AUDIO_S16MSB;
        bytes_per_sample = 2;
    } /* if */
    else
        BAIL_MACRO("AIFF: Unsupported sample size.", 0);

    SDL_RWseek(rw, pos, SEEK_SET);

    BAIL_IF_MACRO(!find_chunk(rw, ssndID), "AIFF: No sound data chunk.", 0);
    BAIL_IF_MACRO(!read_ssnd_chunk(rw, &s),
                  "AIFF: Can't read sound data chunk.", 0);

    a = (aiff_t *) malloc(sizeof(aiff_t));
    BAIL_IF_MACRO(a == NULL, ERR_OUT_OF_MEMORY, 0);
    a->bytesLeft = bytes_per_sample * c.numSampleFrames;
    internal->decoder_private = (void *) a;

    sample->flags = SOUND_SAMPLEFLAG_NONE;

    SNDDBG(("AIFF: Accepting data stream.\n"));
    return(1); /* we'll handle this data. */
} /* AIFF_open */


static void AIFF_close(Sound_Sample *sample)
{
    Sound_SampleInternal *internal = (Sound_SampleInternal *) sample->opaque;
    free(internal->decoder_private);
} /* WAV_close */


static Uint32 AIFF_read(Sound_Sample *sample)
{
    Uint32 retval;
    Sound_SampleInternal *internal = (Sound_SampleInternal *) sample->opaque;
    aiff_t *a = (aiff_t *) internal->decoder_private;
    Uint32 max = (internal->buffer_size < (Uint32) a->bytesLeft) ?
                    internal->buffer_size : (Uint32) a->bytesLeft;

    assert(max > 0);

        /*
         * We don't actually do any decoding, so we read the AIFF data
         *  directly into the internal buffer...
         */
    retval = SDL_RWread(internal->rw, internal->buffer, 1, max);

    a->bytesLeft -= retval;

        /* Make sure the read went smoothly... */
    if ((retval == 0) || (a->bytesLeft == 0))
        sample->flags |= SOUND_SAMPLEFLAG_EOF;

    else if (retval == -1)
        sample->flags |= SOUND_SAMPLEFLAG_ERROR;

        /* (next call this EAGAIN may turn into an EOF or error.) */
    else if (retval < internal->buffer_size)
        sample->flags |= SOUND_SAMPLEFLAG_EAGAIN;

    return(retval);
} /* AIFF_read */

#endif /* SOUND_SUPPORTS_AIFF */


/* end of aiff.c ... */
