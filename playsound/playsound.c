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
#include <string.h>
#include <assert.h>
#include "SDL.h"
#include "SDL_sound.h"

#define PLAYSOUND_VER_MAJOR  0
#define PLAYSOUND_VER_MINOR  1
#define PLAYSOUND_VER_PATCH  3


static void output_versions(const char *argv0)
{
    Sound_Version compiled;
    Sound_Version linked;
    SDL_version sdl_compiled;
    const SDL_version *sdl_linked;

    SOUND_VERSION(&compiled);
    Sound_GetLinkedVersion(&linked);
    SDL_VERSION(&sdl_compiled);
    sdl_linked = SDL_Linked_Version();

    fprintf(stderr,
            "%s version %d.%d.%d.\n"
            " Compiled against SDL_sound version %d.%d.%d,\n"
            " and linked against %d.%d.%d.\n"
            " Compiled against SDL version %d.%d.%d,\n"
            " and linked against %d.%d.%d.\n\n",
             argv0,
             PLAYSOUND_VER_MAJOR, PLAYSOUND_VER_MINOR, PLAYSOUND_VER_PATCH,
             compiled.major, compiled.minor, compiled.patch,
             linked.major, linked.minor, linked.patch,
             sdl_compiled.major, sdl_compiled.minor, sdl_compiled.patch,
             sdl_linked->major, sdl_linked->minor, sdl_linked->patch);
} /* output_versions */


static void output_decoders(void)
{
    const Sound_DecoderInfo **rc = Sound_AvailableDecoders();
    const Sound_DecoderInfo **i;

    fprintf(stderr, "Supported sound formats:\n");
    if (rc == NULL)
        fprintf(stderr, " * Apparently, NONE!\n");
    else
    {
        for (i = rc; *i != NULL; i++)
        {
            fprintf(stderr, " * %s: %s\n    Written by %s.\n    %s\n",
                    (*i)->extension, (*i)->description,
                    (*i)->author, (*i)->url);
        } /* for */
    } /* else */

    fprintf(stderr, "\n");
} /* output_decoders */


static volatile int done_flag = 0;

static Uint8 *decoded_ptr = NULL;
static Uint32 decoded_bytes = 0;

static void audio_callback(void *userdata, Uint8 *stream, int len)
{
    Sound_Sample *sample = (Sound_Sample *) userdata;
    int bw = 0; /* bytes written to stream*/
    int cpysize;

    while (bw < len)
    {
        if (!decoded_bytes)
        {
            if (sample->flags & (SOUND_SAMPLEFLAG_ERROR|SOUND_SAMPLEFLAG_EOF))
            {
                memset(stream + bw, '\0', len - bw);
                done_flag = 1;
                return;
            } /* if */

            decoded_bytes = Sound_Decode(sample);
            decoded_ptr = sample->buffer;
        } /* if */

        cpysize = len - bw;
        if (cpysize > decoded_bytes)
            cpysize = decoded_bytes;

        if (cpysize > 0)
        {
            memcpy(stream + bw, decoded_ptr, cpysize);
            bw += cpysize;
            decoded_ptr += bw;
            decoded_bytes -= bw;
        } /* if */
    } /* while */
} /* audio_callback */


static void output_usage(const char *argv0)
{
    fprintf(stderr,
            "USAGE: %s [...options...] [soundFile1] ... [soundFileN]\n"	
            "\n"
            "   Options:\n"
            "     --rate x      Playback at sample rate of x HZ.\n"
            "     --format fmt  Playback in fmt format (see below).\n"
            "     --channels n  Playback on n channels (1 or 2).\n"
            "     --version     Display version information and exit.\n"
            "     --decoders    List supported sound formats and exit.\n"
            "     --help        Display this information and exit.\n"
            "\n"
            "   Valid arguments to the --format option are:\n"
            "     U8      Unsigned 8-bit.\n"
            "     S8      Signed 8-bit.\n"
            "     U16LSB  Unsigned 16-bit (least significant byte first).\n"
            "     U16MSB  Unsigned 16-bit (most significant byte first).\n"
            "     S16LSB  Signed 16-bit (least significant byte first).\n"
            "     S16MSB  Signed 16-bit (most significant byte first).\n"
            "\n",
            argv0);
} /* output_usage */


static int str_to_fmt(char *str)
{
    if (strcmp(str, "U8") == 0)
        return AUDIO_U8;
    if (strcmp(str, "S8") == 0)
        return AUDIO_S8;
    if (strcmp(str, "U16LSB") == 0)
        return AUDIO_U16LSB;
    if (strcmp(str, "S16LSB") == 0)
        return AUDIO_S16LSB;
    if (strcmp(str, "U16MSB") == 0)
        return AUDIO_U16MSB;
    if (strcmp(str, "S16MSB") == 0)
        return AUDIO_S16MSB;
    return 0;
} /* str_to_fmt */


int main(int argc, char **argv)
{
    Sound_AudioInfo sound_desired;
    SDL_AudioSpec sdl_desired;
    SDL_AudioSpec sdl_actual;
    Sound_Sample *sample;
    int use_specific_audiofmt = 0;
    int i;

        /* !!! FIXME: Move this to a parse_cmdline() function... */
    if (argc < 2)
    {
        output_usage(argv[0]);
        return(42);
    } /* if */

    memset(&sound_desired, '\0', sizeof (sound_desired));

    for (i = 0; i < argc; i++)
    {
        if (strncmp(argv[i], "--", 2) != 0)
            continue;

        if (strcmp(argv[i], "--version") == 0)
        {
            output_versions(argv[0]);
            return(42);
        } /* if */

        else if (strcmp(argv[i], "--help") == 0)
        {
            output_usage(argv[0]);
            return(42);
        } /* if */
 
        else if (strcmp(argv[i], "--rate") == 0 && argc > i + 1)
        {
            use_specific_audiofmt = 1;
            sound_desired.rate = atoi(argv[++i]);
            if (sound_desired.rate <= 0)
            {
                fprintf(stderr, "Bad argument to --rate!\n");
                return(42);
            }
        } /* else if */

        else if (strcmp(argv[i], "--format") == 0 && argc > i + 1)
        {
            use_specific_audiofmt = 1;
            sound_desired.format = str_to_fmt(argv[++i]);
            if (sound_desired.format == 0)
            {
                fprintf(stderr, "Bad argument to --format! Try one of:\n"
                                "U8, S8, U16LSB, S16LSB, U16MSB, S16MSB\n");
                return(42);
            }
        } /* else if */

        else if (strcmp(argv[i], "--channels") == 0 && argc > i + 1)
        {
            use_specific_audiofmt = 1;
            sound_desired.channels = atoi(argv[++i]);
            if (sound_desired.channels < 1 || sound_desired.channels > 2)
            {
                fprintf(stderr,
                        "Bad argument to --channels! Try 1 (mono) or 2 "
                        "(stereo).\n");
                return(42);
            }
        } /* else if */

        else if (strcmp(argv[i], "--decoders") == 0)
        {
            if (!Sound_Init())
            {
                fprintf(stderr, "Sound_Init() failed!\n"
                                "  reason: [%s].\n", Sound_GetError());
                SDL_Quit();
                return(42);
            } /* if */

            output_decoders();
            Sound_Quit();
            return(0);
        } /* else if */

        else
        {
            fprintf(stderr, "unknown option: \"%s\"\n", argv[i]);
            return(42);
        } /* else */
    } /* for */

        /* Pick sensible defaults for any value not explicitly specified. */
    if (use_specific_audiofmt)
    {
        if (sound_desired.rate == 0)
            sound_desired.rate = 44100;
        if (sound_desired.format == 0)
            sound_desired.format = AUDIO_S16SYS;
        if (sound_desired.channels == 0)
            sound_desired.channels = 2;
    } /* if */

    if (SDL_Init(SDL_INIT_AUDIO) == -1)
    {
        fprintf(stderr, "SDL_Init(SDL_INIT_AUDIO) failed!\n"
                        "  reason: [%s].\n", SDL_GetError());
        return(42);
    } /* if */

    if (!Sound_Init())
    {
        fprintf(stderr, "Sound_Init() failed!\n"
                        "  reason: [%s].\n", Sound_GetError());
        SDL_Quit();
        return(42);
    } /* if */

    for (i = 1; i < argc; i++)
    {
            /* !!! FIXME: This is ugly! */
        if ( (strcmp(argv[i], "--rate") == 0) ||
             (strcmp(argv[i], "--format") == 0) ||
             (strcmp(argv[i], "--channels") == 0) )
        {
            i++;
            continue;
        } /* if */

        if (strncmp(argv[i], "--", 2) == 0)
            continue;

        sample = Sound_NewSampleFromFile(argv[i],
                     use_specific_audiofmt ? &sound_desired : NULL, 4096 * 4);

        if (!sample)
        {
            fprintf(stderr, "Couldn't load \"%s\"!\n"
                            "  reason: [%s].\n", argv[i], Sound_GetError());
            continue;
        } /* if */

            /*
             * Unless explicitly specified, pick the format from the sound
             * to be played.
             */
        if (use_specific_audiofmt)
        {
            sdl_desired.freq = sound_desired.rate;
            sdl_desired.format = sound_desired.format;
            sdl_desired.channels = sound_desired.channels;
        } /* if */
        else
        {
            sdl_desired.freq = sample->actual.rate;
            sdl_desired.format = sample->actual.format;
            sdl_desired.channels = sample->actual.channels;
        } /* else */

        sdl_desired.samples = 4096;
        sdl_desired.callback = audio_callback;
        sdl_desired.userdata = sample;

        if (SDL_OpenAudio(&sdl_desired, NULL) < 0)
        {
            fprintf(stderr, "Couldn't open audio device!\n"
                            "  reason: [%s].\n", SDL_GetError());
            Sound_Quit();
            SDL_Quit();
            return(42);
        } /* if */

        printf("Now playing [%s]...\n", argv[i]);

        done_flag = 0;
        SDL_PauseAudio(0);
        while (!done_flag)
        {
            SDL_Delay(10);
        } /* while */
        SDL_PauseAudio(1);

        if (sample->flags & SOUND_SAMPLEFLAG_ERROR)
        {
            fprintf(stderr, "Error in decoding sound file!\n"
                            "  reason: [%s].\n", Sound_GetError());
        } /* if */

        SDL_CloseAudio();  /* reopen with next sample's format if possible */
        Sound_FreeSample(sample);
    } /* for */

    Sound_Quit();
    SDL_Quit();
    return(0);
} /* main */

/* end of playsound.c ... */

