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
 * MIDI decoder for SDL_sound, using TiMidity.
 *
 * This is a rough, proof-of-concept implementation. A number of things need
 *  to be fixed or at the very least looked at:
 *
 * - I have only tested this with TiMidity++. Does TiMidity use the same
 *   command-line options?
 * - No attempt is made to ensure that the input really is MIDI. Eek. Does
 *   anyone here know how to recognize MIDI?
 * - Error handling is a joke.
 * - Did I make any stupid errors in the process communication?
 * - TiMidity++ spews a number of messages (to stderr?) which should be
 *   silenced eventually.
 * - This has to be the least portable decoder that has ever been written.
 * - Etc.
 *
 * Oh, and the whole thing should be rewritten as a generic "send data to
 *  external decoder" thing so this version should either be scrapped or
 *  cannibalized for ideas. :-)
 *
 * Please see the file COPYING in the source's root directory.
 *
 *  This file written by Torbjörn Andersson. (d91tan@Update.UU.SE)
 */

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#ifdef SOUND_SUPPORTS_MIDI

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "SDL_sound.h"

#define __SDL_SOUND_INTERNAL__
#include "SDL_sound_internal.h"


static int MIDI_init(void);
static void MIDI_quit(void);
static int MIDI_open(Sound_Sample *sample, const char *ext);
static void MIDI_close(Sound_Sample *sample);
static Uint32 MIDI_read(Sound_Sample *sample);

static const char *extensions_midi[] = { "MIDI", NULL };
const Sound_DecoderFunctions __Sound_DecoderFunctions_MIDI =
{
    {
        extensions_midi,
        "MIDI music through the TiMidity MIDI to WAVE converter",
        "Torbjörn Andersson <d91tan@Update.UU.SE>",
        "http://www.goice.co.jp/member/mo/timidity/"
    },

    MIDI_init,       /* init() method       */
    MIDI_quit,       /* quit() method       */
    MIDI_open,       /* open() method       */
    MIDI_close,      /* close() method       */
    MIDI_read        /* read() method       */
};

    /* this is what we store in our internal->decoder_private field... */
typedef struct
{
    int fd_audio;
} midi_t;


static void sig_pipe(int signo)
{
    signal(signo, sig_pipe);
} /* sig_pipe */


static int MIDI_init(void)
{
    if (signal(SIGPIPE, sig_pipe) == SIG_ERR)
    {
        Sound_SetError("MIDI: Could not set up SIGPIPE signal handler.");
        return(0);
    } /* if */
    return(1);
} /* MIDI_init */


static void MIDI_quit(void)
{
    /* it's a no-op. */
} /* MIDI_quit */


static int MIDI_open(Sound_Sample *sample, const char *ext)
{
    Sound_SampleInternal *internal = (Sound_SampleInternal *) sample->opaque;
    SDL_RWops *rw = internal->rw;
    midi_t *m;
    Uint8 buf[BUFSIZ];
    ssize_t retval;
    int fd1[2];
    int fd2[2];
    pid_t pid;

        /*
         * Use the desired format if available, or pick sensible defaults.
         * TiMidity does all the conversions we need.
         */
    if (sample->desired.rate == 0)
    {
        sample->actual.channels = 2;
        sample->actual.rate = 44100;
        sample->actual.format = AUDIO_S16SYS;
    } /* if */
    else
        memcpy(&sample->actual, &sample->desired, sizeof (Sound_AudioInfo));
    sample->flags = SOUND_SAMPLEFLAG_NONE;

    if (pipe(fd1) < 0 || pipe(fd2) < 0)
    {
        Sound_SetError("MIDI: Pipe error.");
        return(0);
    } /* if */

    pid = fork();
    if (pid < 0)
    {
        Sound_SetError("MIDI: Fork error.");
        return(0);
    } /* if */

    if (pid == 0)
    {
        /* child */
        char arg_freq[10];   /* !!! FIXME: Large enough? */
        char arg_format[10];
        char *arg_list[] =
            { "timidity", "-s", NULL, NULL, "-o", "-", "-", NULL };

        arg_list[2] = arg_freq;
        arg_list[3] = arg_format;

        close(fd1[1]);
        close(fd2[0]);

            /* !!! FIXME: Errors should be propagated back to the parent */

        if (fd1[0] != STDIN_FILENO)
        {
            if (dup2(fd1[0], STDIN_FILENO) != STDIN_FILENO)
                return(0);
            close(fd1[0]);
        } /* if */

        if (fd2[1] != STDOUT_FILENO)
        {
            if (dup2(fd2[1], STDOUT_FILENO) != STDOUT_FILENO)
                return(0);
            close(fd2[1]);
        } /* if */

            /* Construct command-line arguments matching the audio format */
        sprintf(arg_freq, "%d", sample->actual.rate);
        sprintf(arg_format, "-Or%c%c%cl",
                (sample->actual.format & 0x0008) ? '8' : '1',
                (sample->actual.format & 0x8000) ? 's' : 'u',
                (sample->actual.channels == 1) ? 'M' : 'S');
        if ((sample->actual.format & 0x0010) &&
            (sample->actual.format & 0x7fff) != (AUDIO_S16SYS & 0x7fff))
            strcat(arg_format, "x");

        if (execvp("timidity", arg_list) < 0)
            return(0);

        SNDDBG(("MIDI: Is this line ever reached?"));
        return(1);
    } /* if */

    /* parent */
    close(fd1[0]);
    close(fd2[1]);

        /* Copy the entire file to the child process */
    for (;;)
    {
        retval = SDL_RWread(internal->rw, buf, 1, BUFSIZ);
        if (retval == 0)
            break;

        if (write(fd1[1], buf, retval) != retval)
        {
            Sound_SetError("MIDI: Could not send data to child process.");
            return(0);
        }
    } /* for */

    close(fd1[1]);

    /* !!! FIXME: We still need some way of recognizing valid MIDI data */

    m = (midi_t *) malloc(sizeof(midi_t));
    BAIL_IF_MACRO(m == NULL, ERR_OUT_OF_MEMORY, 0);
    internal->decoder_private = (void *) m;

        /* This is where we'll read the converted audio data. */
    m->fd_audio = fd2[0];

    SNDDBG(("MIDI: Accepting data stream.\n"));
    return(1); /* we'll handle this data. */
} /* MIDI_open */


static void MIDI_close(Sound_Sample *sample)
{
    Sound_SampleInternal *internal = (Sound_SampleInternal *) sample->opaque;

    close(((midi_t *) internal->decoder_private)->fd_audio);
    free(internal->decoder_private);
} /* MIDI_close */


static Uint32 MIDI_read(Sound_Sample *sample)
{
    ssize_t retval;
    Sound_SampleInternal *internal = (Sound_SampleInternal *) sample->opaque;
    midi_t *m = (midi_t *) internal->decoder_private;

        /*
         * We don't actually do any decoding, so we read the converted data
         *  directly into the internal buffer.
         */
    retval = read(m->fd_audio, internal->buffer, internal->buffer_size);

        /* Make sure the read went smoothly... */
    if (retval == 0)
        sample->flags |= SOUND_SAMPLEFLAG_EOF;

    else if (retval == -1)
        sample->flags |= SOUND_SAMPLEFLAG_ERROR;

        /* (next call this EAGAIN may turn into an EOF or error.) */
    else if (retval < internal->buffer_size)
        sample->flags |= SOUND_SAMPLEFLAG_EAGAIN;

    return(retval);
} /* MIDI_read */

#endif /* SOUND_SUPPORTS_MIDI */

/* end of midi.c ... */

