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
 * MPEG-1 Layer 3, or simply, "MP3", decoder for SDL_sound.
 *
 * This driver handles all those highly compressed songs you stole through
 *  Napster.  :)  It depends on the SMPEG library for decoding, which can
 *  be grabbed from: http://www.lokigames.com/development/smpeg.php3
 *
 * Please see the file LICENSE in the source's root directory.
 *
 *  This file written by Ryan C. Gordon. (icculus@clutteredmind.org)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "smpeg.h"
#include "SDL_sound.h"
#include "extra_rwops.h"

#define __SDL_SOUND_INTERNAL__
#include "SDL_sound_internal.h"

#if (!defined SOUND_SUPPORTS_MP3)
#error SOUND_SUPPORTS_MP3 must be defined.
#endif


static int MP3_init(void);
static void MP3_quit(void);
static int MP3_open(Sound_Sample *sample, const char *ext);
static void MP3_close(Sound_Sample *sample);
static Uint32 MP3_read(Sound_Sample *sample);

const Sound_DecoderFunctions __Sound_DecoderFunctions_MP3 =
{
    {
        "MP3",
        "MPEG-1 Layer 3 audio through SMPEG",
        "Ryan C. Gordon <icculus@clutteredmind.org>",
        "http://www.icculus.org/SDL_sound/"
    },

    MP3_init,       /*  init() method */
    MP3_quit,       /*  quit() method */
    MP3_open,       /*  open() method */
    MP3_close,      /* close() method */
    MP3_read        /*  read() method */
};


static int MP3_init(void)
{
    return(1);  /* always succeeds. */
} /* MP3_init */


static void MP3_quit(void)
{
    /* it's a no-op. */
} /* MP3_quit */


static __inline__ void output_version(void)
{
    static int first_time = 1;

    if (first_time)
    {
        SMPEG_version v;
        SMPEG_VERSION(&v);
        _D(("MP3: Compiled against SMPEG v%d.%d.%d.\n",
                    v.major, v.minor, v.patch));
        first_time = 0;
    } /* if */
} /* output_version */


static int MP3_open(Sound_Sample *sample, const char *ext)
{
    SMPEG *smpeg;
    SMPEG_Info smpeg_info;
    SDL_AudioSpec spec;
    Sound_SampleInternal *internal = (Sound_SampleInternal *) sample->opaque;
    SDL_RWops *refCounter;
    Uint8 mp3_magic[2];

    output_version();

       /*
        * SMPEG appears to be far too greedy about what it accepts as input.
        * This test was adapted from SDL_mixer.
        */
    if (SDL_RWread(internal->rw, mp3_magic, sizeof (mp3_magic), 1) != 1)
    {
        Sound_SetError("MP3: Could not read MP3 magic.");
        return(0);
    }
    if (mp3_magic[0] != 0xFF || (mp3_magic[1] & 0xF0) != 0xF0)
    {
        Sound_SetError("MP3: Not an MP3 stream.");
        return(0);
    }
    SDL_RWseek(internal->rw, -sizeof (mp3_magic), SEEK_CUR);

    refCounter = RWops_RWRefCounter_new(internal->rw);
    if (refCounter == NULL)
    {
        _D(("MP3: Failed to create reference counting RWops.\n"));
        return(0);
    } /* if */

    /* replace original RWops. This is safe. Honest. :) */
    internal->rw = refCounter;

    /*
     * increment the refcount, since SMPEG will nuke the RWops if it can't
     *  accept the contained data...
     */
    RWops_RWRefCounter_addRef(refCounter);
    smpeg = SMPEG_new_rwops(refCounter, &smpeg_info, 0);

    if (SMPEG_error(smpeg))
    {
        Sound_SetError(SMPEG_error(smpeg));
        SMPEG_delete(smpeg);
        return(0);
    } /* if */

    if (!smpeg_info.has_audio)
    {
        Sound_SetError("MP3: No audio stream found in data.");
        SMPEG_delete(smpeg);
        return(0);
    } /* if */

    _D(("MP3: Accepting data stream.\n"));
    _D(("MP3: has_audio == {%s}.\n", smpeg_info.has_audio ? "TRUE" : "FALSE"));
    _D(("MP3: has_video == {%s}.\n", smpeg_info.has_video ? "TRUE" : "FALSE"));
    _D(("MP3: width == (%d).\n", smpeg_info.width));
    _D(("MP3: height == (%d).\n", smpeg_info.height));
    _D(("MP3: current_frame == (%d).\n", smpeg_info.current_frame));
    _D(("MP3: current_fps == (%f).\n", smpeg_info.current_fps));
    _D(("MP3: audio_string == [%s].\n", smpeg_info.audio_string));
    _D(("MP3: audio_current_frame == (%d).\n", smpeg_info.audio_current_frame));
    _D(("MP3: current_offset == (%d).\n", smpeg_info.current_offset));
    _D(("MP3: total_size == (%d).\n", smpeg_info.total_size));
    _D(("MP3: current_time == (%f).\n", smpeg_info.current_time));
    _D(("MP3: total_time == (%f).\n", smpeg_info.total_time));

    SMPEG_enablevideo(smpeg, 0);
    SMPEG_enableaudio(smpeg, 1);
    SMPEG_loop(smpeg, 0);

    SMPEG_wantedSpec(smpeg, &spec);
        /*
         * One of the MP3:s I tried wouldn't work unless I added this line
         * to tell SMPEG that yes, it may have the spec it wants.
         */
    SMPEG_actualSpec(smpeg, &spec);
    sample->actual.format = spec.format;
    sample->actual.rate = spec.freq;
    sample->actual.channels = spec.channels;
    sample->flags = SOUND_SAMPLEFLAG_NONE;
    internal->decoder_private = smpeg;

    SMPEG_play(smpeg);
    return(1);
} /* MP3_open */


static void MP3_close(Sound_Sample *sample)
{
    Sound_SampleInternal *internal = (Sound_SampleInternal *) sample->opaque;
    SMPEG_delete((SMPEG *) internal->decoder_private);
} /* MP3_close */


static Uint32 MP3_read(Sound_Sample *sample)
{
    Uint32 retval;
    Sound_SampleInternal *internal = (Sound_SampleInternal *) sample->opaque;
    SMPEG *smpeg = (SMPEG *) internal->decoder_private;

        /*
         * We have to clear the buffer because apparently SMPEG_playAudio()
         * will mix the decoded audio with whatever's already in it. Nasty.
         */
    memset(internal->buffer, 0, internal->buffer_size);
    retval = SMPEG_playAudio(smpeg, internal->buffer, internal->buffer_size);
    if (retval < internal->buffer_size)
    {
        char *errMsg = SMPEG_error(smpeg);
        if (errMsg == NULL)
            sample->flags |= SOUND_SAMPLEFLAG_EOF;
        else
        {
            Sound_SetError(errMsg);
            sample->flags |= SOUND_SAMPLEFLAG_ERROR;
        } /* else */
    } /* if */

    return(retval);
} /* MP3_read */

/* end of mp3.c ... */

