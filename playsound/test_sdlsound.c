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
 * This is a quick and dirty test of SDL_sound.
 *
 * Please see the file LICENSE in the source's root directory.
 *
 *  This file written by Ryan C. Gordon. (icculus@clutteredmind.org)
 */

#include <stdio.h>
#include <assert.h>
#include "SDL.h"
#include "SDL_sound.h"

#define TEST_VERSION_MAJOR  0
#define TEST_VERSION_MINOR  1
#define TEST_VERSION_PATCH  0


static void output_versions(void)
{
    Sound_Version compiled;
    Sound_Version linked;
    SDL_version sdl_compiled;
    const SDL_version *sdl_linked;

    SOUND_VERSION(&compiled);
    Sound_GetLinkedVersion(&linked);
    SDL_VERSION(&sdl_compiled);
    sdl_linked = SDL_Linked_Version();

    printf("test_sdlsound version %d.%d.%d.\n"
           " Compiled against SDL_sound version %d.%d.%d,\n"
           " and linked against %d.%d.%d.\n"
           " Compiled against SDL version %d.%d.%d,\n"
           " and linked against %d.%d.%d.\n\n",
            TEST_VERSION_MAJOR, TEST_VERSION_MINOR, TEST_VERSION_PATCH,
            compiled.major, compiled.minor, compiled.patch,
            linked.major, linked.minor, linked.patch,
            sdl_compiled.major, sdl_compiled.minor, sdl_compiled.patch,
            sdl_linked->major, sdl_linked->minor, sdl_linked->patch);
} /* output_versions */


static void output_decoders(void)
{
    const Sound_DecoderInfo **rc = Sound_AvailableDecoders();
    const Sound_DecoderInfo **i;

    printf("Supported sound formats:\n");
    if (*rc == NULL)
        printf(" * Apparently, NONE!\n");
    else
    {
        for (i = rc; *i != NULL; i++)
        {
            printf(" * %s: %s\n    Written by %s.\n    %s\n",
                    (*i)->extension, (*i)->description,
                    (*i)->author, (*i)->url);
        } /* for */
    } /* else */

    printf("\n");
} /* output_decoders */


static volatile int done_flag = 0;

void test_callback(void *userdata, Uint8 *stream, int len)
{
    static Uint8 overflow[16384]; /* this is a hack. */
    static Uint32 overflowBytes = 0;
    Sound_Sample *sample = (Sound_Sample *) userdata;
    Uint32 bw = 0; /* bytes written to stream*/
    Uint32 rc;  /* return code */

    if (overflowBytes > 0)
    {
        memcpy(stream, overflow, overflowBytes);
        bw = overflowBytes;
        overflowBytes = 0;
    } /* if */

    while ((bw < len) && (!done_flag))
    {
        rc = Sound_Decode(sample);
        if (rc > 0)
        {
            if (bw + rc > len)
            {
                overflowBytes = (bw + rc) - len;
                memcpy(overflow,
                       ((Uint8 *) sample->buffer) + (rc - overflowBytes),
                       overflowBytes);
                rc -= overflowBytes;
            } /* if */

            memcpy(stream + bw, sample->buffer, rc);
            bw += rc;
        } /* if */

        if (sample->flags & SOUND_SAMPLEFLAG_EOF)
            done_flag = 1;

        else if (sample->flags & SOUND_SAMPLEFLAG_ERROR)
        {
            printf("Error condition in decoding!\n");
            done_flag = 1;
        } /* else if */
    } /* while */

    assert(bw <= len);

    if (bw < len)
        memset(stream + bw, '\0', len - bw);
} /* test_callback */


int main(int argc, char **argv)
{
    Sound_AudioInfo sound_desired;
    SDL_AudioSpec sdl_desired;
    Sound_Sample *sample;

    if (SDL_Init(SDL_INIT_AUDIO) == -1)
    {
        printf("SDL_Init(SDL_INIT_AUDIO) failed!\n"
               "  reason: [%s].\n", SDL_GetError());
        return(42);
    } /* if */

    if (!Sound_Init())
    {
        printf("Sound_Init() failed!\n"
               "  reason: [%s].\n", Sound_GetError());
        SDL_Quit();
        return(42);
    } /* if */

    output_versions();
    output_decoders();

    if (argc != 2)
    {
        printf("USAGE: %s <oneSupportedFile>\n", argv[0]);
        Sound_Quit();
        SDL_Quit();
        return(42);
    } /* if */

    sound_desired.rate = 44100;
    sound_desired.channels = 2;
    sound_desired.format = AUDIO_S16SYS;

    sample = Sound_NewSampleFromFile(argv[1], &sound_desired, 4096 * 4);
    if (!sample)
    {
        printf("Sound_NewSampleFromFile(\"%s\") failed!\n"
               "  reason: [%s].\n", argv[1], Sound_GetError());
        Sound_Quit();
        SDL_Quit();
        return(42);
    } /* if */

	sdl_desired.freq = 44100;
	sdl_desired.format = AUDIO_S16SYS;
	sdl_desired.channels = 2;
	sdl_desired.samples = 4096;
	sdl_desired.callback = test_callback;
	sdl_desired.userdata = sample;

#if 1
	if ( SDL_OpenAudio(&sdl_desired, NULL) < 0 )
    {
        printf("SDL_OpenAudio() failed!\n"
               "  reason: [%s].\n", SDL_GetError());
        Sound_Quit();
        SDL_Quit();
        return(42);
    } /* if */

    SDL_PauseAudio(0);
    while (!done_flag)
        SDL_Delay(10);
    SDL_PauseAudio(1);

/*
 * This just decodes the file, and dumps the decoded waveform to
 *  stderr. Use with caution.
 */
#else
    {
        Uint32 rc;
        while ((rc = Sound_Decode(sample)) == sample->buffer_size)
            fwrite(sample->buffer, rc, 1, stderr);
    }
#endif


    Sound_FreeSample(sample);

    Sound_Quit();
    SDL_Quit();
    return(0);
} /* main */

/* end of test_sdlsound.c ... */

