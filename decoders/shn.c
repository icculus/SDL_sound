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
 * Shorten decoder for SDL_sound.
 *
 * This driver handles Shorten-compressed waveforms. Despite the fact that
 *  SHNs are generally used in online trading communities, they tend to be
 *  much bigger than MP3s. If an MP3 crunches the waveform to 10-20 percent
 *  of its original size, SHNs only go to about 50-60%. Why do the Phish fans
 *  of the world use this format then? Rabid music traders appreciate the
 *  sound quality; SHNs, unlike MP3s, do not throw away any part of the
 *  waveform. Yes, there are people that notice this, and further more, they
 *  demand it...and if they can't get a good transfer of those larger files
 *  over the 'net, they haven't underestimated the bandwidth of CDs travelling
 *  the world through the postal system.
 *
 * Shorten homepage: http://www.softsound.com/Shorten.html
 *
 * The Shorten format was gleaned from the shorten codebase, by Tony
 *  Robinson and SoftSound Limited, but none of their code was used here.
 *
 * Please see the file LICENSE in the source's root directory.
 *
 *  This file written by Ryan C. Gordon. (icculus@clutteredmind.org)
 */

#if (defined SOUND_SUPPORTS_SHN)

#define __SDL_SOUND_INTERNAL__
#include "SDL_sound_internal.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "SDL_sound.h"


static int SHN_init(void);
static void SHN_quit(void);
static int SHN_open(Sound_Sample *sample, const char *ext);
static void SHN_close(Sound_Sample *sample);
static Uint32 SHN_read(Sound_Sample *sample);

const Sound_DecoderFunctions __Sound_DecoderFunctions_SHN =
{
    {
        "SHN",
        "Shorten-compressed audio data",
        "Ryan C. Gordon <icculus@clutteredmind.org>",
        "http://www.icculus.org/SDL_sound/"
    },

    SHN_init,       /* init() method       */
    SHN_quit,       /* quit() method       */
    SHN_open,       /* open() method       */
    SHN_close,      /* close() method       */
    SHN_read        /* read() method       */
};


static int SHN_init(void)
{
    return(1);  /* initialization always successful. */
} /* SHN_init */


static void SHN_quit(void)
{
    /* it's a no-op. */
} /* SHN_quit */


static Uint32 mask_table[] =
{
    0x00000000, 0x00000001, 0x00000003, 0x00000007, 0x0000000F, 0x0000001F,
    0x0000003F, 0x0000007F, 0x000000FF, 0x000001FF, 0x000003FF, 0x000007FF,
    0x00000FFF, 0x00001FFF, 0x00003FFF, 0x00007FFF, 0x0000FFFF, 0x0001FFFF,
    0x0003FFFF, 0x0007FFFF, 0x000FFFFF, 0x001FFFFF, 0x003FFFFF, 0x007FFFFF,
    0x00FFFFFF, 0x01FFFFFF, 0x03FFFFFF, 0x07FFFFFF, 0x0FFFFFFF, 0x1FFFFFFF,
    0x3FFFFFFF, 0x7FFFFFFF, 0xFFFFFFFF
};


#define MAGIC_NUM 0x676B6A61   /* looks like "ajkg" as chars. */

/*
 * Look through the whole file for a SHN magic number. This is costly, so
 *  it should only be done if the user SWEARS they have a Shorten stream...
 */
static inline int extended_shn_magic_search(Sound_Sample *sample)
{
    SDL_RWops *rw = ((Sound_SampleInternal *) sample->opaque)->rw;
    Uint32 word = 0;
    Uint8 ch;

    while (1)
    {
        BAIL_IF_MACRO(SDL_RWread(rw, &ch, sizeof (ch), 1) != 1, NULL, -1);
        word = ((word << 8) & 0xFFFFFF00) | ch;
        if (SDL_SwapLE32(word) == MAGIC_NUM)
        {
            BAIL_IF_MACRO(SDL_RWread(rw, &ch, sizeof (ch), 1) != 1, NULL, -1);
            return((int) ch);
        } /* if */
    } /* while */

    return((int) ch);
} /* extended_shn_magic_search */


/* look for the magic number in the RWops and see what kind of file this is. */
static inline int determine_shn_version(Sound_Sample *sample)
{
    SDL_RWops *rw = ((Sound_SampleInternal *) sample->opaque)->rw;
    Uint32 magic;
    Uint8 ch;

    /*
     * Apparently the magic number can start at any byte offset in the file,
     *  and we should just discard prior data, but I'm going to restrict it
     *  to offset zero for now, so we don't chug down every file that might
     *  happen to pass through here. If the extension is explicitly "SHN", we
     *  check the whole stream, though.
     */

    if (__Sound_strcasecmp(ext, "shn") == 0)
        return(extended_shn_magic_search(sample);

    BAIL_IF_MACRO(SDL_RWread(rw, &magic, sizeof (magic), 1) != 1, NULL, -1);
    BAIL_IF_MACRO(SDL_SwapLE32(magic) != MAGIC_NUM, "SHN: Not a SHN file", -1);
    BAIL_IF_MACRO(SDL_RWread(rw, &ch, sizeof (ch), 1) != 1, NULL, -1);
    BAIL_IF_MACRO(ch > 3, "SHN: Unsupported file version", -1);

    return((int) ch);
} /* determine_shn_version */


static int SHN_open(Sound_Sample *sample, const char *ext)
{
    Sound_SampleInternal *internal = (Sound_SampleInternal *) sample->opaque;
    SDL_RWops *rw = internal->rw;
    int shn_version = determine_shn_version(sample, ext);
    int mean = MEAN_VERSION2;

    BAIL_IF_MACRO(shn_version == -1, NULL, 0);
    if (shn_version < 2)  /* downgrade? */
        mean = MEAN_VERSION0;



    SNDDBG(("SHN: Accepting data stream.\n"));
    set up sample->actual;
    sample->flags = SOUND_SAMPLEFLAG_NONE;
    return(1); /* we'll handle this data. */
} /* SHN_open */


static void SHN_close(Sound_Sample *sample)
{
    Sound_SampleInternal *internal = (Sound_SampleInternal *) sample->opaque;
    clean up anything you put into internal->decoder_private;
} /* SHN_close */


static Uint32 SHN_read(Sound_Sample *sample)
{
    Uint32 retval;
    Sound_SampleInternal *internal = (Sound_SampleInternal *) sample->opaque;

        /*
         * We don't actually do any decoding, so we read the fmt data
         *  directly into the internal buffer...
         */
    retval = SDL_RWread(internal->rw, internal->buffer,
                        1, internal->buffer_size);

    (or whatever. Do some decoding here...)

        /* Make sure the read went smoothly... */
    if (retval == 0)
        sample->flags |= SOUND_SAMPLEFLAG_EOF;

    else if (retval == -1)
        sample->flags |= SOUND_SAMPLEFLAG_ERROR;

        /* (next call this EAGAIN may turn into an EOF or error.) */
    else if (retval < internal->buffer_size)
        sample->flags |= SOUND_SAMPLEFLAG_EAGAIN;

    (or whatever. retval == number of bytes you put in internal->buffer).

    return(retval);
} /* SHN_read */

#endif  /* defined SOUND_SUPPORTS_SHN */

/* end of shn.c ... */

