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
 * Please see the file COPYING in the source's root directory.
 *
 *  This file written by Ryan C. Gordon. (icculus@clutteredmind.org)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <signal.h>
#include "SDL.h"
#include "SDL_sound.h"

#define DEFAULT_DECODEBUF 16384
#define DEFAULT_AUDIOBUF  4096

#define PLAYSOUND_VER_MAJOR  0
#define PLAYSOUND_VER_MINOR  1
#define PLAYSOUND_VER_PATCH  5

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

    printf("%s version %d.%d.%d\n"
           "Copyright 2001 Ryan C. Gordon\n"
           "This program is free software, covered by the GNU Lesser General\n"
           "Public License, and you are welcome to change it and/or\n"
           "distribute copies of it under certain conditions. There is\n"
           "absolutely NO WARRANTY for this program.\n"
           "\n"
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
    const char **ext;

    printf("Supported sound formats:\n");
    if (rc == NULL)
        printf(" * Apparently, NONE!\n");
    else
    {
        for (i = rc; *i != NULL; i++)
        {
            printf(" * %s\n", (*i)->description);
            for (ext = (*i)->extensions; *ext != NULL; ext++)
                printf("   File extension \"%s\"\n", *ext);
            printf("   Written by %s.\n   %s\n\n", (*i)->author, (*i)->url);
        } /* for */
    } /* else */

    printf("\n");
} /* output_decoders */


static void output_usage(const char *argv0)
{
    fprintf(stderr,
        "USAGE: %s [...options...] [soundFile1] ... [soundFileN]\n"
        "\n"
        "   Options:\n"
        "     --rate n       Playback at sample rate of n HZ.\n"
        "     --format fmt   Playback in fmt format (see below).\n"
        "     --channels n   Playback on n channels (1 or 2).\n"
        "     --decodebuf n  Buffer n decoded bytes at a time (default %d).\n"
        "     --audiobuf n   Buffer n samples to audio device (default %d).\n"
        "     --volume n     Playback volume multiplier (default 1.0).\n"
        "     --version      Display version information and exit.\n"
        "     --decoders     List supported data formats and exit.\n"
        "     --predecode    Decode entire sample before playback.\n"
        "     --loop         Loop playback until SIGINT.\n"
        "     --credits      Shameless promotion.\n"
        "     --help         Display this information and exit.\n"
        "\n"
        "   Valid arguments to the --format option are:\n"
        "     U8      Unsigned 8-bit.\n"
        "     S8      Signed 8-bit.\n"
        "     U16LSB  Unsigned 16-bit (least significant byte first).\n"
        "     U16MSB  Unsigned 16-bit (most significant byte first).\n"
        "     S16LSB  Signed 16-bit (least significant byte first).\n"
        "     S16MSB  Signed 16-bit (most significant byte first).\n"
        "\n",
        argv0, DEFAULT_DECODEBUF, DEFAULT_AUDIOBUF);
} /* output_usage */


static void output_credits(void)
{
    printf("playsound version %d.%d.%d\n"
           "Copyright 2001 Ryan C. Gordon\n"
           "playsound is free software, covered by the GNU Lesser General\n"
           "Public License, and you are welcome to change it and/or\n"
           "distribute copies of it under certain conditions. There is\n"
           "absolutely NO WARRANTY for playsound.\n"
           "\n"
           "    Written by Ryan C. Gordon, Torbjörn Andersson, Max Horn,\n"
           "     Tsuyoshi Iguchi, Tyler Montbriand, and a cast of thousands.\n"
           "\n"
           "    Website and source code: http://icculus.org/SDL_sound/\n"
           "\n",
            PLAYSOUND_VER_MAJOR, PLAYSOUND_VER_MINOR, PLAYSOUND_VER_PATCH);
} /* output_credits */



static volatile int done_flag = 0;


void sigint_catcher(int signum)
{
    static Uint32 last_sigint = 0;
    Uint32 ticks = SDL_GetTicks();

    assert(signum == SIGINT);

    if ((last_sigint != 0) && (ticks - last_sigint < 500))
    {
        SDL_PauseAudio(1);
        SDL_CloseAudio();
        Sound_Quit();
        SDL_Quit();
        exit(1);
    } /* if */

    else
    {
        last_sigint = ticks;
        done_flag = 1;
    } /* else */
} /* sigint_catcher */


static Uint8 *decoded_ptr = NULL;
static Uint32 decoded_bytes = 0;
static int predecode = 0;
static int looping = 0;
static int wants_volume_change = 0;
static float volume = 1.0;

/*
 * This updates (decoded_bytes) and (decoder_ptr) with more audio data,
 *  taking into account looping and/or predecoding.
 */
static int read_more_data(Sound_Sample *sample)
{
    if (done_flag)              /* probably a sigint; stop trying to read. */
        decoded_bytes = 0;

    if (decoded_bytes > 0)      /* don't need more data; just return. */
        return(decoded_bytes);

        /* need more. See if there's more to be read... */
    if (!(sample->flags & (SOUND_SAMPLEFLAG_ERROR | SOUND_SAMPLEFLAG_EOF)))
    {
        decoded_bytes = Sound_Decode(sample);
        if (sample->flags & SOUND_SAMPLEFLAG_ERROR)
        {
            fprintf(stderr, "Error in decoding sound file!\n"
                            "  reason: [%s].\n", Sound_GetError());
        } /* if */

        decoded_ptr = sample->buffer;
        return(read_more_data(sample));  /* handle loops conditions. */
    } /* if */

    /* No more to be read from stream, but we may want to loop the sample. */

    if (!looping)
        return(0);

    /* we just need to point predecoded samples to the start of the buffer. */
    if (predecode)
    {
        decoded_bytes = sample->buffer_size;
        decoded_ptr = sample->buffer;
        return(decoded_bytes);
    } /* if */
    else
    {
        Sound_Rewind(sample);  /* error is checked in recursion. */
        return(read_more_data(sample));
    } /* else */

    assert(0);  /* shouldn't ever hit this point. */
    return(0);
} /* read_more_data */


static void memcpy_with_volume(Sound_Sample *sample,
                               Uint8 *dst, Uint8 *src, int len)
{
    int i;
    Uint16 *u16src = NULL;
    Uint16 *u16dst = NULL;
    Sint16 *s16src = NULL;
    Sint16 *s16dst = NULL;

    if (!wants_volume_change)
    {
        memcpy(dst, src, len);
        return;
    }

    /* !!! FIXME: This would be more efficient with a lookup table. */
    switch (sample->desired.format)
    {
        case AUDIO_U8:
            for (i = 0; i < len; i++, src++, dst++)
                *dst = (Uint8) (((float) (*src)) * volume);
            break;

        case AUDIO_S8:
            for (i = 0; i < len; i++, src++, dst++)
                *dst = (Sint8) (((float) (*src)) * volume);
            break;

        case AUDIO_U16LSB:
            u16src = (Uint16 *) src;
            u16dst = (Uint16 *) dst;
            for (i = 0; i < len; i += sizeof (Uint16), u16src++, u16dst++)
            {
                *u16dst = (Uint16) (((float) (SDL_SwapLE16(*u16src))) * volume);
                *u16dst = SDL_SwapLE16(*u16dst);
            }
            break;

        case AUDIO_S16LSB:
            s16src = (Sint16 *) src;
            s16dst = (Sint16 *) dst;
            for (i = 0; i < len; i += sizeof (Sint16), s16src++, s16dst++)
            {
                *s16dst = (Sint16) (((float) (SDL_SwapLE16(*s16src))) * volume);
                *s16dst = SDL_SwapLE16(*s16dst);
            }
            break;

        case AUDIO_U16MSB:
            u16src = (Uint16 *) src;
            u16dst = (Uint16 *) dst;
            for (i = 0; i < len; i += sizeof (Uint16), u16src++, u16dst++)
            {
                *u16dst = (Uint16) (((float) (SDL_SwapBE16(*u16src))) * volume);
                *u16dst = SDL_SwapBE16(*u16dst);
            }
            break;

        case AUDIO_S16MSB:
            s16src = (Sint16 *) src;
            s16dst = (Sint16 *) dst;
            for (i = 0; i < len; i += sizeof (Sint16), s16src++, s16dst++)
            {
                *s16dst = (Sint16) (((float) (SDL_SwapBE16(*s16src))) * volume);
                *s16dst = SDL_SwapBE16(*s16dst);
            }
            break;
    }
}

static void audio_callback(void *userdata, Uint8 *stream, int len)
{
    Sound_Sample *sample = (Sound_Sample *) userdata;
    int bw = 0; /* bytes written to stream this time through the callback */

    while (bw < len)
    {
        int cpysize;  /* bytes to copy on this iteration of the loop. */

        if (!read_more_data(sample)) /* read more data, if needed. */
        {
            /* ...there isn't any more data to read! */
            memset(stream + bw, '\0', len - bw);
            done_flag = 1;
            return;
        } /* if */

        /* decoded_bytes and decoder_ptr are updated as necessary... */

        cpysize = len - bw;
        if (cpysize > decoded_bytes)
            cpysize = decoded_bytes;

        if (cpysize > 0)
        {
            memcpy_with_volume(sample, stream + bw, decoded_ptr, cpysize);
            bw += cpysize;
            decoded_ptr += cpysize;
            decoded_bytes -= cpysize;
        } /* if */
    } /* while */
} /* audio_callback */


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
    Uint32 audio_buffersize = DEFAULT_AUDIOBUF;
    Uint32 decode_buffersize = DEFAULT_DECODEBUF;
    SDL_AudioSpec sdl_desired;
    SDL_AudioSpec sdl_actual;
    Sound_Sample *sample;
    int use_specific_audiofmt = 0;
    int i;
    int delay;

    setbuf(stdout, NULL);
    setbuf(stderr, NULL);

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

        if (strcmp(argv[i], "--credits") == 0)
        {
            output_credits();
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

        else if (strcmp(argv[i], "--audiobuf") == 0 && argc > i + 1)
        {
            audio_buffersize = atoi(argv[++i]);
        } /* else if */

        else if (strcmp(argv[i], "--decodebuf") == 0 && argc > i + 1)
        {
            decode_buffersize = atoi(argv[++i]);
        } /* else if */

        else if (strcmp(argv[i], "--volume") == 0 && argc > i + 1)
        {
            volume = atof(argv[++i]);
            if (volume != 1.0)
                wants_volume_change = 1;
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

        else if (strcmp(argv[i], "--predecode") == 0)
        {
            predecode = 1;
        } /* else if */

        else if (strcmp(argv[i], "--loop") == 0)
        {
            looping = 1;
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

    signal(SIGINT, sigint_catcher);

    for (i = 1; i < argc; i++)
    {
            /* !!! FIXME: This is ugly! */
        if ( (strcmp(argv[i], "--rate") == 0) ||
             (strcmp(argv[i], "--format") == 0) ||
             (strcmp(argv[i], "--channels") == 0) ||
             (strcmp(argv[i], "--audiobuf") == 0) ||
             (strcmp(argv[i], "--decodebuf") == 0) ||
             (strcmp(argv[i], "--volume") == 0) )
        {
            i++;
            continue;
        } /* if */

        if (strncmp(argv[i], "--", 2) == 0)
            continue;

        sample = Sound_NewSampleFromFile(argv[i],
                     use_specific_audiofmt ? &sound_desired : NULL,
                     decode_buffersize);

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

        sdl_desired.samples = audio_buffersize;
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

        if (predecode)
        {
            printf("  predecoding...", argv[i]);
            decoded_bytes = Sound_DecodeAll(sample);
            decoded_ptr = sample->buffer;
            if (sample->flags & SOUND_SAMPLEFLAG_ERROR)
            {
                fprintf(stderr,
                        "Couldn't fully decode \"%s\"!\n"
                        "  reason: [%s].\n"
                        "  (playing first %lu bytes of decoded data...)\n",
                        argv[i], Sound_GetError(), decoded_bytes);
            } /* if */
            else
            {
                printf("done.\n");
            } /* else */
        } /* if */

        done_flag = 0;
        SDL_PauseAudio(0);
        while (!done_flag)
        {
            SDL_Delay(10);
        } /* while */
        SDL_PauseAudio(1);

            /*
             * Sleep two buffers' worth of audio before closing, in order
             *  to allow the playback to finish. This isn't always enough;
             *   perhaps SDL needs a way to explicitly wait for device drain?
             */
        delay = 2 * 1000 * sdl_desired.samples / sdl_desired.freq;
        SDL_Delay(delay);

        SDL_CloseAudio();  /* reopen with next sample's format if possible */
        Sound_FreeSample(sample);
    } /* for */

    Sound_Quit();
    SDL_Quit();
    return(0);
} /* main */

/* end of playsound.c ... */

