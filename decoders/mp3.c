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

#ifdef SOUND_SUPPORTS_MP3

#define __SDL_SOUND_INTERNAL__
#include "SDL_sound_internal.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "smpeg.h"
#include "SDL_sound.h"
#include "extra_rwops.h"


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
        SNDDBG(("MP3: Compiled against SMPEG v%d.%d.%d.\n",
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

    output_version();

       /*
        * If I understand things correctly, MP3 files don't really have any
        * magic header we can check for. The MP3 player is expected to just
        * pick the first thing that looks like a valid frame and start
        * playing from there.
        *
        * So here's what we do: If the caller insists that this is really
        * MP3 we'll take his word for it. Otherwise, use the same test as
        * SDL_mixer does and check if the stream starts with something that
        * looks like a frame.
        *
        * A frame begins with 11 bits of frame sync (all bits must be set),
        * followed by a two-bit MPEG Audio version ID:
        *
        *   00 - MPEG Version 2.5 (later extension of MPEG 2)
        *   01 - reserved
        *   10 - MPEG Version 2 (ISO/IEC 13818-3)
        *   11 - MPEG Version 1 (ISO/IEC 11172-3)
        *
        * Apparently we don't handle MPEG Version 2.5.
        */
    if (__Sound_strcasecmp(ext, "MP3") != 0)
    {
        Uint8 mp3_magic[2];

        if (SDL_RWread(internal->rw, mp3_magic, sizeof (mp3_magic), 1) != 1)
        {
            Sound_SetError("MP3: Could not read MP3 magic.");
            return(0);
        } /*if */

        if (mp3_magic[0] != 0xFF || (mp3_magic[1] & 0xF0) != 0xF0)
        {
            Sound_SetError("MP3: Not an MP3 stream.");
            return(0);
        } /* if */

            /* !!! FIXME: If the seek fails, we'll probably miss a frame */
        SDL_RWseek(internal->rw, -sizeof (mp3_magic), SEEK_CUR);
    } /* if */

    refCounter = RWops_RWRefCounter_new(internal->rw);
    if (refCounter == NULL)
    {
        SNDDBG(("MP3: Failed to create reference counting RWops.\n"));
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

    SNDDBG(("MP3: Accepting data stream.\n"));
    SNDDBG(("MP3: has_audio == {%s}.\n", smpeg_info.has_audio ? "TRUE" : "FALSE"));
    SNDDBG(("MP3: has_video == {%s}.\n", smpeg_info.has_video ? "TRUE" : "FALSE"));
    SNDDBG(("MP3: width == (%d).\n", smpeg_info.width));
    SNDDBG(("MP3: height == (%d).\n", smpeg_info.height));
    SNDDBG(("MP3: current_frame == (%d).\n", smpeg_info.current_frame));
    SNDDBG(("MP3: current_fps == (%f).\n", smpeg_info.current_fps));
    SNDDBG(("MP3: audio_string == [%s].\n", smpeg_info.audio_string));
    SNDDBG(("MP3: audio_current_frame == (%d).\n", smpeg_info.audio_current_frame));
    SNDDBG(("MP3: current_offset == (%d).\n", smpeg_info.current_offset));
    SNDDBG(("MP3: total_size == (%d).\n", smpeg_info.total_size));
    SNDDBG(("MP3: current_time == (%f).\n", smpeg_info.current_time));
    SNDDBG(("MP3: total_time == (%f).\n", smpeg_info.total_time));

    SMPEG_enablevideo(smpeg, 0);
    SMPEG_enableaudio(smpeg, 1);
    SMPEG_loop(smpeg, 0);

    SMPEG_wantedSpec(smpeg, &spec);

        /*
         * One of the MP3s I tried wouldn't work unless I added this line
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

#endif /* SOUND_SUPPORTS_MP3 */

/* end of mp3.c ... */

