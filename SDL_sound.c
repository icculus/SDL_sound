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
 * This file implements the core API, which is relatively simple.
 *   The real meat of SDL_sound is in the decoders directory.
 *
 * Documentation is in SDL_sound.h ... It's verbose, honest.  :)
 *
 * Please see the file LICENSE in the source's root directory.
 *
 *  This file written by Ryan C. Gordon. (icculus@clutteredmind.org)
 */

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>

#include "SDL.h"
#include "SDL_sound.h"

#define __SDL_SOUND_INTERNAL__
#include "SDL_sound_internal.h"


/* The various decoder drivers... */

#if (defined SOUND_SUPPORTS_MP3)
extern const Sound_DecoderFunctions  __Sound_DecoderFunctions_MP3;
#endif

#if (defined SOUND_SUPPORTS_MOD)
extern const Sound_DecoderFunctions  __Sound_DecoderFunctions_MOD;
#endif

#if (defined SOUND_SUPPORTS_WAV)
extern const Sound_DecoderFunctions  __Sound_DecoderFunctions_WAV;
#endif

#if (defined SOUND_SUPPORTS_AIFF)
extern const Sound_DecoderFunctions  __Sound_DecoderFunctions_AIFF;
#endif

#if (defined SOUND_SUPPORTS_OGG)
extern const Sound_DecoderFunctions  __Sound_DecoderFunctions_OGG;
#endif

#if (defined SOUND_SUPPORTS_VOC)
extern const Sound_DecoderFunctions  __Sound_DecoderFunctions_VOC;
#endif

#if (defined SOUND_SUPPORTS_RAW)
extern const Sound_DecoderFunctions  __Sound_DecoderFunctions_RAW;
#endif

#if (defined SOUND_SUPPORTS_SHN)
extern const Sound_DecoderFunctions  __Sound_DecoderFunctions_SHN;
#endif

#if (defined SOUND_SUPPORTS_MIDI)
extern const Sound_DecoderFunctions  __Sound_DecoderFunctions_MIDI;
#endif



typedef struct
{
    int available;
    const Sound_DecoderFunctions *funcs;
} decoder_element;

static decoder_element decoders[] =
{
#if (defined SOUND_SUPPORTS_MP3)
    { 0, &__Sound_DecoderFunctions_MP3 },
#endif

#if (defined SOUND_SUPPORTS_MOD)
    { 0, &__Sound_DecoderFunctions_MOD },
#endif

#if (defined SOUND_SUPPORTS_WAV)
    { 0, &__Sound_DecoderFunctions_WAV },
#endif

#if (defined SOUND_SUPPORTS_AIFF)
    { 0, &__Sound_DecoderFunctions_AIFF },
#endif

#if (defined SOUND_SUPPORTS_OGG)
    { 0, &__Sound_DecoderFunctions_OGG },
#endif

#if (defined SOUND_SUPPORTS_VOC)
    { 0, &__Sound_DecoderFunctions_VOC },
#endif

#if (defined SOUND_SUPPORTS_RAW)
    { 0, &__Sound_DecoderFunctions_RAW },
#endif

#if (defined SOUND_SUPPORTS_SHN)
    { 0, &__Sound_DecoderFunctions_SHN },
#endif

#if (defined SOUND_SUPPORTS_MIDI)
    { 0, &__Sound_DecoderFunctions_MIDI },
#endif

    { 0, NULL }
};



/* General SDL_sound state ... */

static int initialized = 0;
static Sound_Sample *samplesList = NULL;  /* this is a linked list. */
static const Sound_DecoderInfo **available_decoders = NULL;


/* functions ... */

void Sound_GetLinkedVersion(Sound_Version *ver)
{
    if (ver != NULL)
    {
        ver->major = SOUND_VER_MAJOR;
        ver->minor = SOUND_VER_MINOR;
        ver->patch = SOUND_VER_PATCH;
    } /* if */
} /* Sound_GetLinkedVersion */


int Sound_Init(void)
{
    size_t i;
    size_t pos = 0;
    size_t total = sizeof (decoders) / sizeof (decoders[0]);
    BAIL_IF_MACRO(initialized, ERR_IS_INITIALIZED, 0);
    samplesList = NULL;

    SDL_Init(SDL_INIT_AUDIO);

    available_decoders = (const Sound_DecoderInfo **)
                            malloc((total) * sizeof (Sound_DecoderInfo *));
    BAIL_IF_MACRO(available_decoders == NULL, ERR_OUT_OF_MEMORY, 0);

    for (i = 0; decoders[i].funcs != NULL; i++)
    {
        decoders[i].available = decoders[i].funcs->init();
        if (decoders[i].available)
        {
            available_decoders[pos] = &(decoders[i].funcs->info);
            pos++;
        } /* if */
    } /* for */

    available_decoders[pos] = NULL;

    initialized = 1;
    return(1);
} /* Sound_Init */


int Sound_Quit(void)
{
    size_t i;

    BAIL_IF_MACRO(!initialized, ERR_NOT_INITIALIZED, 0);

    while (((volatile Sound_Sample *) samplesList) != NULL)
        Sound_FreeSample(samplesList);

    for (i = 0; decoders[i].funcs != NULL; i++)
    {
        if (decoders[i].available)
        {
            decoders[i].funcs->quit();
            decoders[i].available = 0;
        } /* if */
    } /* for */

    if (available_decoders != NULL)
        free((void *) available_decoders);
    available_decoders = NULL;

    initialized = 0;

    return(1);
} /* Sound_Quit */


const Sound_DecoderInfo **Sound_AvailableDecoders(void)
{
    return(available_decoders);  /* READ. ONLY. */
} /* Sound_AvailableDecoders */


const char *Sound_GetError(void)
{
    return(SDL_GetError());
} /* Sound_GetError */


void Sound_ClearError(void)
{
    SDL_ClearError();
} /* Sound_ClearError */


/*
 * This is declared in the internal header.
 */
void Sound_SetError(const char *err)
{
    if (err != NULL)
    {
        SNDDBG(("Sound_SetError(\"%s\");\n", err));
        SDL_SetError(err);
    } /* if */
} /* Sound_SetError */


/*
 * -ansi and -pedantic flags prevent use of strcasecmp() on Linux, and
 *  I honestly don't want to mess around with figuring out if a given
 *  platform has "strcasecmp", "stricmp", or
 *  "compare_two_damned_strings_case_insensitive", which I hear is in the
 *  next release of Carbon.  :)  This is exported so decoders may use it if
 *  they like.
 */
int __Sound_strcasecmp(const char *x, const char *y)
{
    int ux, uy;

    if (x == y)  /* same pointer? Both NULL? */
        return(0);

    if (x == NULL)
        return(-1);

    if (y == NULL)
        return(1);
       
    do
    {
        ux = toupper((int) *x);
        uy = toupper((int) *y);
        if (ux > uy)
            return(1);
        else if (ux < uy)
            return(-1);
        x++;
        y++;
    } while ((ux) && (uy));

    return(0);
} /* __Sound_strcasecmp */


/*
 * Allocate a Sound_Sample, and fill in most of its fields. Those that need
 *  to be filled in later, by a decoder, will be initialized to zero.
 */
static Sound_Sample *alloc_sample(SDL_RWops *rw, Sound_AudioInfo *desired,
                                    Uint32 bufferSize)
{
    Sound_Sample *retval = malloc(sizeof (Sound_Sample));
    Sound_SampleInternal *internal = malloc(sizeof (Sound_SampleInternal));
    if ((retval == NULL) || (internal == NULL))
    {
        Sound_SetError(ERR_OUT_OF_MEMORY);
        if (retval)
            free(retval);
        if (internal)
            free(internal);

        return(NULL);
    } /* if */

    memset(retval, '\0', sizeof (Sound_Sample));
    memset(internal, '\0', sizeof (Sound_SampleInternal));

    assert(bufferSize > 0);
    retval->buffer = malloc(bufferSize);  /* pure ugly. */
    if (!retval->buffer)
    {
        Sound_SetError(ERR_OUT_OF_MEMORY);
        free(internal);
        free(retval);
        return(NULL);
    } /* if */
    memset(retval->buffer, '\0', bufferSize);
    retval->buffer_size = bufferSize;

    if (desired != NULL)
        memcpy(&retval->desired, desired, sizeof (Sound_AudioInfo));

    internal->rw = rw;
    retval->opaque = internal;
    return(retval);
} /* alloc_sample */


#if (defined DEBUG_CHATTER)
static __inline__ const char *fmt_to_str(Uint16 fmt)
{
    switch(fmt)
    {
        case AUDIO_U8:
            return("U8");
        case AUDIO_S8:
            return("S8");
        case AUDIO_U16LSB:
            return("U16LSB");
        case AUDIO_S16LSB:
            return("S16LSB");
        case AUDIO_U16MSB:
            return("U16MSB");
        case AUDIO_S16MSB:
            return("S16MSB");
    } /* switch */

    return("Unknown");
} /* fmt_to_str */
#endif


/*
 * The bulk of the Sound_NewSample() work is done here...
 *  Ask the specified decoder to handle the data in (rw), and if
 *  so, construct the Sound_Sample. Otherwise, try to wind (rw)'s stream
 *  back to where it was, and return false.
 *
 *  !!! FIXME: This is big, ugly, nasty, and smelly.
 */
static int init_sample(const Sound_DecoderFunctions *funcs,
                        Sound_Sample *sample, const char *ext,
                        Sound_AudioInfo *_desired)
{
    Sound_SampleInternal *internal = (Sound_SampleInternal *) sample->opaque;
    Sound_AudioInfo desired;
    int pos = SDL_RWtell(internal->rw);  /* !!! FIXME: Int? Really? */

        /* fill in the funcs for this decoder... */
    sample->decoder = &funcs->info;
    internal->funcs = funcs;
    if (!funcs->open(sample, ext))
    {
        SDL_RWseek(internal->rw, pos, SEEK_SET);  /* set for next try... */
        return(0);
    } /* if */

    /* success; we've got a decoder! */

    /* Now we need to set up the conversion buffer... */

    memcpy(&desired, (_desired != NULL) ? _desired : &sample->actual,
            sizeof (Sound_AudioInfo));

    if (SDL_BuildAudioCVT(&internal->sdlcvt,
                          sample->actual.format,
                          sample->actual.channels,
                          (int) sample->actual.rate,   /* !!! FIXME: Int? Really? */
                          desired.format,
                          desired.channels,
                          (int) desired.rate) == -1) /* !!! FIXME: Int? Really? */
    {
        Sound_SetError(SDL_GetError());
        funcs->close(sample);
        SDL_RWseek(internal->rw, pos, SEEK_SET);  /* set for next try... */
        return(0);
    } /* if */

    if (internal->sdlcvt.len_mult > 1)
    {
        void *rc = realloc(sample->buffer,
                            sample->buffer_size * internal->sdlcvt.len_mult);
        if (rc == NULL)
        {
            funcs->close(sample);
            SDL_RWseek(internal->rw, pos, SEEK_SET);  /* set for next try... */
            return(0);
        } /* if */

        sample->buffer = rc;
    } /* if */

        /* these pointers are all one and the same. */
    memcpy(&sample->desired, &desired, sizeof (Sound_AudioInfo));
    internal->sdlcvt.buf = internal->buffer = sample->buffer;
    internal->buffer_size = sample->buffer_size / internal->sdlcvt.len_mult;
    internal->sdlcvt.len = internal->buffer_size;

    /* Prepend our new Sound_Sample to the samplesList... */
    if (samplesList != NULL)
    {
        internal->next = samplesList;
        if (samplesList != NULL)
            internal->prev = sample;
    } /* if */
    samplesList = sample;

    SNDDBG(("New sample DESIRED format: %s format, %d rate, %d channels.\n",
            fmt_to_str(sample->desired.format),
            sample->desired.rate,
            sample->desired.channels));

    SNDDBG(("New sample ACTUAL format: %s format, %d rate, %d channels.\n",
            fmt_to_str(sample->actual.format),
            sample->actual.rate,
            sample->actual.channels));

    SNDDBG(("On-the-fly conversion: %s.\n",
            internal->sdlcvt.needed ? "ENABLED" : "DISABLED"));

    return(1);
} /* init_sample */


Sound_Sample *Sound_NewSample(SDL_RWops *rw, const char *ext,
                              Sound_AudioInfo *desired, Uint32 bSize)
{
    size_t i;
    Sound_Sample *retval;

    /* sanity checks. */
    BAIL_IF_MACRO(!initialized, ERR_NOT_INITIALIZED, NULL);
    BAIL_IF_MACRO(rw == NULL, ERR_INVALID_ARGUMENT, NULL);

    retval = alloc_sample(rw, desired, bSize);
    if (!retval)
        return(NULL);  /* alloc_sample() sets error message... */

    if (ext != NULL)
    {
        for (i = 0; decoders[i].funcs != NULL; i++)
        {
            if (decoders[i].available)
            {
                const char *decoderExt = decoders[i].funcs->info.extension;
                if (__Sound_strcasecmp(decoderExt, ext) == 0)
                {
                    if (init_sample(decoders[i].funcs, retval, ext, desired))
                        return(retval);
                } /* if */
            } /* if */
        } /* for */
    } /* if */

    /* no direct extension match? Try everything we've got... */
    for (i = 0; decoders[i].funcs != NULL; i++)
    {
        if (decoders[i].available)
        {
            if (init_sample(decoders[i].funcs, retval, ext, desired))
                return(retval);
        } /* if */
    } /* for */

    /* nothing could handle the sound data... */
    free(retval->opaque);
    if (retval->buffer != NULL)
        free(retval->buffer);
    free(retval);
    SDL_RWclose(rw);
    Sound_SetError(ERR_UNSUPPORTED_FORMAT);
    return(NULL);
} /* Sound_NewSample */


Sound_Sample *Sound_NewSampleFromFile(const char *filename,
                                      Sound_AudioInfo *desired,
                                      Uint32 bufferSize)
{
    const char *ext;
    SDL_RWops *rw;

    BAIL_IF_MACRO(!initialized, ERR_NOT_INITIALIZED, NULL);
    BAIL_IF_MACRO(filename == NULL, ERR_INVALID_ARGUMENT, NULL);

    ext = strrchr(filename, '.');
    rw = SDL_RWFromFile(filename, "rb");
    BAIL_IF_MACRO(rw == NULL, SDL_GetError(), NULL);

    if (ext != NULL)
        ext++;

    return(Sound_NewSample(rw, ext, desired, bufferSize));
} /* Sound_NewSampleFromFile */


void Sound_FreeSample(Sound_Sample *sample)
{
    Sound_SampleInternal *internal;

    if (!initialized)
    {
        Sound_SetError(ERR_NOT_INITIALIZED);
        return;
    } /* if */

    if (sample == NULL)
    {
        Sound_SetError(ERR_INVALID_ARGUMENT);
        return;
    } /* if */

    internal = (Sound_SampleInternal *) sample->opaque;

    internal->funcs->close(sample);

    /* update the samplesList... */
    if (internal->prev != NULL)
    {
        Sound_SampleInternal *prevInternal;
        prevInternal = (Sound_SampleInternal *) internal->prev->opaque;
        prevInternal->next = internal->next;
    } /* if */
    else
    {
        assert(samplesList == sample);
        samplesList = internal->next;
    } /* else */

    if (internal->next != NULL)
    {
        Sound_SampleInternal *nextInternal;
        nextInternal = (Sound_SampleInternal *) internal->next->opaque;
        nextInternal->prev = internal->prev;
    } /* if */

    /* nuke it... */
    if (internal->rw != NULL)  /* this condition is a "just in case" thing. */
        SDL_RWclose(internal->rw);
    assert(internal->buffer != NULL);
    if (internal->buffer != sample->buffer)
        free(internal->buffer);
    free(internal);
    if (sample->buffer != NULL)
        free(sample->buffer);
    free(sample);
} /* Sound_FreeSample */


int Sound_SetBufferSize(Sound_Sample *sample, Uint32 newSize)
{
    void *newBuf = NULL;
    Sound_SampleInternal *internal = NULL;

    BAIL_IF_MACRO(!initialized, ERR_NOT_INITIALIZED, 0);
    BAIL_IF_MACRO(sample == NULL, ERR_INVALID_ARGUMENT, 0);
    internal = ((Sound_SampleInternal *) sample->opaque);
    newBuf = realloc(sample->buffer, newSize * internal->sdlcvt.len_mult);
    BAIL_IF_MACRO(newBuf == NULL, ERR_OUT_OF_MEMORY, 0);

    internal->sdlcvt.buf = internal->buffer = sample->buffer = newBuf;
    sample->buffer_size = newSize;
    internal->buffer_size = newSize / internal->sdlcvt.len_mult;
    internal->sdlcvt.len = internal->buffer_size;

    return(1);
} /* Sound_SetBufferSize */


Uint32 Sound_Decode(Sound_Sample *sample)
{
    Sound_SampleInternal *internal = NULL;
    Uint32 retval = 0;

        /* a boatload of sanity checks... */
    BAIL_IF_MACRO(!initialized, ERR_NOT_INITIALIZED, 0);
    BAIL_IF_MACRO(sample == NULL, ERR_INVALID_ARGUMENT, 0);
    BAIL_IF_MACRO(sample->flags & SOUND_SAMPLEFLAG_ERROR, ERR_PREV_ERROR, 0);
    BAIL_IF_MACRO(sample->flags & SOUND_SAMPLEFLAG_EOF, ERR_PREV_EOF, 0);

    internal = (Sound_SampleInternal *) sample->opaque;

    assert(sample->buffer != NULL);
    assert(sample->buffer_size > 0);
    assert(internal->buffer != NULL);
    assert(internal->buffer_size > 0);

        /* reset EAGAIN. Decoder can flip it back on if it needs to. */
    sample->flags &= !SOUND_SAMPLEFLAG_EAGAIN;
    retval = internal->funcs->read(sample);

    if (internal->sdlcvt.needed)
    {
        internal->sdlcvt.len = retval;
        SDL_ConvertAudio(&internal->sdlcvt);
        retval *= internal->sdlcvt.len_mult;
    } /* if */

    return(retval);
} /* Sound_Decode */


Uint32 Sound_DecodeAll(Sound_Sample *sample)
{
    Sound_SampleInternal *internal = NULL;
    void *buf = NULL;
    Uint32 newBufSize = 0;

    BAIL_IF_MACRO(!initialized, ERR_NOT_INITIALIZED, 0);

    internal = (Sound_SampleInternal *) sample->opaque;

    while ( ((sample->flags & SOUND_SAMPLEFLAG_EOF) == 0) &&
            ((sample->flags & SOUND_SAMPLEFLAG_ERROR) == 0) )
    {
        void *ptr = realloc(buf, newBufSize + sample->buffer_size);
        if (ptr == NULL)
        {
            sample->flags |= SOUND_SAMPLEFLAG_ERROR;
            Sound_SetError(ERR_OUT_OF_MEMORY);
        } /* if */
        else
        {
            Uint32 br = Sound_Decode(sample);
            memcpy( ((char *) buf) + newBufSize, sample->buffer, br );
            newBufSize += br;
        } /* else */
    } /* while */

    free(sample->buffer);
    sample->buffer = internal->buffer = internal->sdlcvt.buf = buf;
    sample->buffer_size = newBufSize;
    internal->buffer_size = newBufSize / internal->sdlcvt.len_mult;
    internal->sdlcvt.len_mult = internal->buffer_size;

    return(newBufSize);
} /* Sound_DecodeAll */


/* end of SDL_sound.c ... */

