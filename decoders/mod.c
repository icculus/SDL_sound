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
 * Module player - through MikMod - for SDL_sound.
 *
 * This driver handles anything our subset of MikMod handles. It is based on
 *  SDL_mixer's MikMod music player.
 *
 * Please see the file LICENSE in the source's root directory.
 *
 *  This file written by Torbjörn Andersson (d91tan@Update.UU.SE)
 */

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#ifdef SOUND_SUPPORTS_MOD

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "SDL_sound.h"

#define __SDL_SOUND_INTERNAL__
#include "SDL_sound_internal.h"

#include "mikmod.h"


static int MOD_init(void);
static void MOD_quit(void);
static int MOD_open(Sound_Sample *sample, const char *ext);
static void MOD_close(Sound_Sample *sample);
static Uint32 MOD_read(Sound_Sample *sample);

static const char *extensions_mikmod[] =
{
    "MOD", "IT",  "XM",  "S3M", "MTM", "669", "STM", "ULT",
    "FAR", "MED", "AMF", "DSM", "IMF", "GDM", "STX", "OKT",
    NULL
};

const Sound_DecoderFunctions __Sound_DecoderFunctions_MOD =
{
    {
        extensions_mikmod,
        "Play modules through MikMod",
        "Torbjörn Andersson <d91tan@Update.UU.SE>",
        "http://www.mikmod.org/"
    },

    MOD_init,       /*  init() method */
    MOD_quit,       /*  quit() method */
    MOD_open,       /*  open() method */
    MOD_close,      /* close() method */
    MOD_read        /*  read() method */
};

    /* this is what we store in our internal->decoder_private field... */
typedef struct {
    MODULE *module;
} mod_t;


/* Make MikMod read from a RWops... */

typedef struct MRWOPSREADER {
    MREADER core;
    Sound_Sample *sample;
    int end;
} MRWOPSREADER;

static BOOL _mm_RWopsReader_eof(MREADER *reader)
{
    MRWOPSREADER *rwops_reader = (MRWOPSREADER *) reader;
    Sound_Sample *sample = rwops_reader->sample;
    Sound_SampleInternal *internal = (Sound_SampleInternal *) sample->opaque;
    int pos = SDL_RWtell(internal->rw);

    if (rwops_reader->end == pos)
        return(1);

    return(0);
} /* _mm_RWopsReader_eof */


static BOOL _mm_RWopsReader_read(MREADER *reader, void *ptr, size_t size)
{
    MRWOPSREADER *rwops_reader = (MRWOPSREADER *) reader;
    Sound_Sample *sample = rwops_reader->sample;
    Sound_SampleInternal *internal = (Sound_SampleInternal *) sample->opaque;
    return(SDL_RWread(internal->rw, ptr, size, 1));
} /* _mm_RWopsReader_Read */


static int _mm_RWopsReader_get(MREADER *reader)
{
    char buf;
    MRWOPSREADER *rwops_reader = (MRWOPSREADER *) reader;
    Sound_Sample *sample = rwops_reader->sample;
    Sound_SampleInternal *internal = (Sound_SampleInternal *) sample->opaque;

    if (SDL_RWread(internal->rw, &buf, 1, 1) != 1)
        return(EOF);

    return((int) buf);
} /* _mm_RWopsReader_get */


static BOOL _mm_RWopsReader_seek(MREADER *reader, long offset, int whence)
{
    MRWOPSREADER *rwops_reader = (MRWOPSREADER *) reader;
    Sound_Sample *sample = rwops_reader->sample;
    Sound_SampleInternal *internal = (Sound_SampleInternal *) sample->opaque;

    return(SDL_RWseek(internal->rw, offset, whence));
} /* _mm_RWopsReader_seek */


static long _mm_RWopsReader_tell(MREADER *reader)
{
    MRWOPSREADER *rwops_reader = (MRWOPSREADER *) reader;
    Sound_Sample *sample = rwops_reader->sample;
    Sound_SampleInternal *internal = (Sound_SampleInternal *) sample->opaque;

    return(SDL_RWtell(internal->rw));
} /* _mm_RWopsReader_tell */


static MREADER *_mm_new_rwops_reader(Sound_Sample *sample)
{
    Sound_SampleInternal *internal = (Sound_SampleInternal *) sample->opaque;

    MRWOPSREADER *reader = (MRWOPSREADER *) malloc(sizeof (MRWOPSREADER));
    if (reader != NULL)
    {
        int here;
        reader->core.Eof  = _mm_RWopsReader_eof;
        reader->core.Read = _mm_RWopsReader_read;
        reader->core.Get  = _mm_RWopsReader_get;
        reader->core.Seek = _mm_RWopsReader_seek;
        reader->core.Tell = _mm_RWopsReader_tell;
        reader->sample = sample;

        /* RWops does not explicitly support an eof check, so we shall find
           the end manually - this requires seek support for the RWop */
        here = SDL_RWtell(internal->rw);
        reader->end = SDL_RWseek(internal->rw, 0, SEEK_END);
        SDL_RWseek(internal->rw, here, SEEK_SET);   /* Move back */
        /* !!! FIXME: What happens if the seek fails? */
    } /* if */

    return((MREADER *) reader);
} /* _mm_new_rwops_reader */


static void _mm_delete_rwops_reader(MREADER *reader)
{
        /* SDL_sound will delete the RWops and sample at a higher level... */
    if (reader != NULL)
        free(reader);
} /* _mm_delete_rwops_reader */



static int MOD_init(void)
{
    MikMod_RegisterDriver(&drv_nos);
    MikMod_RegisterAllLoaders();

        /*
         * Both DMODE_SOFT_MUSIC and DMODE_16BITS should be set by default,
         * so this is just for clarity. I haven't experimented with any of
         * the other flags. There are a few which are said to give better
         * sound quality.
         */
    md_mode |= (DMODE_SOFT_MUSIC | DMODE_16BITS);
    md_mixfreq = 0;

    BAIL_IF_MACRO(MikMod_Init(""), MikMod_strerror(MikMod_errno), 0);

    return(1);  /* success. */
} /* MOD_init */


static void MOD_quit(void)
{
    MikMod_Exit();
    md_mixfreq = 0;
} /* MOD_quit */


static int MOD_open(Sound_Sample *sample, const char *ext)
{
    Sound_SampleInternal *internal = (Sound_SampleInternal *) sample->opaque;
    MREADER *reader;
    mod_t *m;

    m = (mod_t *) malloc(sizeof(mod_t));
    BAIL_IF_MACRO(m == NULL, ERR_OUT_OF_MEMORY, 0);
    reader = _mm_new_rwops_reader(sample);
    BAIL_IF_MACRO(reader == NULL, ERR_OUT_OF_MEMORY, 0);
    m->module = Player_LoadGeneric(reader, 64, 0);
    _mm_delete_rwops_reader(reader);
    BAIL_IF_MACRO(m->module == NULL, "MOD: Not a module file.", 0);

    if (md_mixfreq == 0)
        md_mixfreq = (!sample->desired.rate) ? 44100 : sample->desired.rate;

    sample->actual.channels = 2;
    sample->actual.rate = md_mixfreq;
    sample->actual.format = AUDIO_S16SYS;
    internal->decoder_private = (void *) m;

    Player_Start(m->module);
    Player_SetPosition(0);

    /* !!! FIXME: A little late to be giving this information... */
    sample->flags = SOUND_SAMPLEFLAG_NEEDSEEK;

    SNDDBG(("MOD: Accepting data stream\n"));
    return(1); /* we'll handle this data. */
} /* MOD_open */


static void MOD_close(Sound_Sample *sample)
{
    Sound_SampleInternal *internal = (Sound_SampleInternal *) sample->opaque;
    mod_t *m = (mod_t *) internal->decoder_private;

    Player_Free(m->module);
    free(m);
} /* MOD_close */


static Uint32 MOD_read(Sound_Sample *sample)
{
    Sound_SampleInternal *internal = (Sound_SampleInternal *) sample->opaque;
    mod_t *m = (mod_t *) internal->decoder_private;

        /* Switch to the current module, stopping any previous one. */
    Player_Start(m->module);
    if (!Player_Active())
    {
        sample->flags |= SOUND_SAMPLEFLAG_EOF;
        return(0);
    } /* if */
    return((Uint32) VC_WriteBytes(internal->buffer, internal->buffer_size));
} /* MOD_read */

#endif /* SOUND_SUPPORTS_MOD */


/* end of mod.c ... */

