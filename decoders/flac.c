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
 * FLAC decoder for SDL_sound.
 *
 * This driver handles FLAC audio, that is to say the Free Lossless Audio
 *  Codec. It depends on libFLAC for decoding, which can be grabbed from:
 *  http://flac.sourceforge.net
 *
 * Please see the file COPYING in the source's root directory.
 *
 *  This file written by Torbjörn Andersson. (d91tan@Update.UU.SE)
 */

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#ifdef SOUND_SUPPORTS_FLAC

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "SDL_sound.h"

#define __SDL_SOUND_INTERNAL__
#include "SDL_sound_internal.h"

#include "FLAC/stream_decoder.h"


static int FLAC_init(void);
static void FLAC_quit(void);
static int FLAC_open(Sound_Sample *sample, const char *ext);
static void FLAC_close(Sound_Sample *sample);
static Uint32 FLAC_read(Sound_Sample *sample);

static const char *extensions_flac[] = { "FLAC", "FLA", NULL };

const Sound_DecoderFunctions __Sound_DecoderFunctions_FLAC =
{
    {
        extensions_flac,
        "Free Lossless Audio Codec",
        "Torbjörn Andersson <d91tan@Update.UU.SE>",
        "http://flac.sourceforge.net/"
    },

    FLAC_init,       /* init() method       */
    FLAC_quit,       /* quit() method       */
    FLAC_open,       /* open() method       */
    FLAC_close,      /* close() method       */
    FLAC_read        /* read() method       */
};

    /* This is what we store in our internal->decoder_private field. */
typedef struct
{
    FLAC__StreamDecoder *decoder;
    SDL_RWops *rw;
    Sound_Sample *sample;
    Uint32 frame_size;
    Uint8 is_flac;
} flac_t;


static void free_flac(flac_t *f)
{
    FLAC__stream_decoder_finish(f->decoder);
    FLAC__stream_decoder_delete(f->decoder);
    free(f);
} /* free_flac */


static FLAC__StreamDecoderReadStatus FLAC_read_callback(
    const FLAC__StreamDecoder *decoder, FLAC__byte buffer[],
    unsigned int *bytes, void *client_data)
{
    flac_t *f = (flac_t *) client_data;
    Uint32 retval;

#if 0
    SNDDBG(("FLAC: Read callback.\n"));
#endif
    
    retval = SDL_RWread(f->rw, (Uint8 *) buffer, 1, *bytes);

    if (retval == 0)
    {
        *bytes = 0;
        f->sample->flags |= SOUND_SAMPLEFLAG_EOF;
        return(FLAC__STREAM_DECODER_READ_END_OF_STREAM);
    } /* if */

    if (retval == -1)
    {
        *bytes = 0;
        f->sample->flags |= SOUND_SAMPLEFLAG_ERROR;
        return(FLAC__STREAM_DECODER_READ_ABORT);
    } /* if */

    if (retval < *bytes)
    {
        *bytes = retval;
        f->sample->flags |= SOUND_SAMPLEFLAG_EAGAIN;
    } /* if */

    return(FLAC__STREAM_DECODER_READ_CONTINUE);
} /* FLAC_read_callback */


static FLAC__StreamDecoderWriteStatus FLAC_write_callback(
    const FLAC__StreamDecoder *decoder, const FLAC__Frame *frame,
    const FLAC__int32 *buffer[], void *client_data)
{
    flac_t *f = (flac_t *) client_data;
    Uint32 i, j;
    Uint32 sample;
    Uint8 *dst;

#if 0
    SNDDBG(("FLAC: Write callback.\n"));
#endif

    f->frame_size = frame->header.channels * frame->header.blocksize
        * frame->header.bits_per_sample / 8;

    if (f->frame_size > f->sample->buffer_size)
        Sound_SetBufferSize(f->sample, f->frame_size);

    dst = f->sample->buffer;

        /* If the sample is neither exactly 8-bit nor 16-bit, it will have to
         * be converted. Unfortunately the buffer is read-only, so we either
         * have to check for each sample, or make a copy of the buffer. I'm
         * not sure which way is best, so I've arbitrarily picked the former.
         */
    if (f->sample->actual.format == AUDIO_S8)
    {
        for (i = 0; i < frame->header.blocksize; i++)
            for (j = 0; j < frame->header.channels; j++)
            {
                sample = buffer[j][i];
                if (frame->header.bits_per_sample < 8)
                    sample <<= (8 - frame->header.bits_per_sample);
                *dst++ = sample & 0x00ff;
            } /* for */
    } /* if */
    else
    {
        for (i = 0; i < frame->header.blocksize; i++)
            for (j = 0; j < frame->header.channels; j++)
            {
                sample = buffer[j][i];
                if (frame->header.bits_per_sample < 16)
                    sample <<= (16 - frame->header.bits_per_sample);
                else if (frame->header.bits_per_sample > 16)
                    sample >>= (frame->header.bits_per_sample - 16);
                *dst++ = (sample & 0xff00) >> 8;
                *dst++ = sample & 0x00ff;
            } /* for */
    } /* else */

    return(FLAC__STREAM_DECODER_WRITE_CONTINUE);
} /* FLAC_write_callback */


void FLAC_metadata_callback(
    const FLAC__StreamDecoder *decoder, const FLAC__StreamMetaData *metadata,
    void *client_data)
{
    flac_t *f = (flac_t *) client_data;
    
    SNDDBG(("FLAC: Metadata callback.\n"));

        /* There are several kinds of metadata, but STREAMINFO is the only
         * one that always has to be there.
         */
    if (metadata->type == FLAC__METADATA_TYPE_STREAMINFO)
    {
        SNDDBG(("FLAC: Metadata is streaminfo.\n"));

        f->is_flac = 1;
        f->sample->actual.channels = metadata->data.stream_info.channels;
        f->sample->actual.rate = metadata->data.stream_info.sample_rate;

        if (metadata->data.stream_info.bits_per_sample > 8)
            f->sample->actual.format = AUDIO_S16MSB;
        else
            f->sample->actual.format = AUDIO_S8;
    } /* if */
} /* FLAC_metadata_callback */


void FLAC_error_callback(
    const FLAC__StreamDecoder *decoder, FLAC__StreamDecoderErrorStatus status,
    void *client_data)
{
    flac_t *f = (flac_t *) client_data;

        /* !!! FIXME: Is every error really fatal? I don't know... */
    Sound_SetError(FLAC__StreamDecoderErrorStatusString[status]);
    f->sample->flags |= SOUND_SAMPLEFLAG_ERROR;
} /* FLAC_error_callback */


static int FLAC_init(void)
{
    return(1);  /* always succeeds. */
} /* FLAC_init */


static void FLAC_quit(void)
{
    /* it's a no-op. */
} /* FLAC_quit */


#define FLAC_MAGIC 0x43614C66  /* "fLaC" in ASCII. */

static int FLAC_open(Sound_Sample *sample, const char *ext)
{
    Sound_SampleInternal *internal = (Sound_SampleInternal *) sample->opaque;
    SDL_RWops *rw = internal->rw;
    FLAC__StreamDecoder *decoder;
    flac_t *f;
    int i;
    int has_extension = 0;

    /*
     * If the extension is "flac", we'll believe that this is really meant
     *  to be a FLAC stream, and will try to grok it from existing metadata.
     *  metadata searching can be a very expensive operation, however, so
     *  unless the user swears that it is a FLAC stream through the extension,
     *  we decide what to do based on the existance of a 32-bit magic number.
     */
    for (i = 0; extensions_flac[i] != NULL; i++)
    {
        if (__Sound_strcasecmp(ext, extensions_flac[i]) == 0)
        {
            has_extension = 1;
            break;
        } /* if */
    } /* for */

    if (!has_extension)
    {
        int rc;
        Uint32 flac_magic = SDL_ReadLE32(rw);
        BAIL_IF_MACRO(flac_magic != FLAC_MAGIC, "FLAC: Not a FLAC stream.", 0);

        /* move back over magic number for metadata scan... */
        rc = SDL_RWseek(internal->rw, -sizeof (flac_magic), SEEK_CUR);
        BAIL_IF_MACRO(rc < 0, ERR_IO_ERROR, 0);
    } /* if */

    f = (flac_t *) malloc(sizeof (flac_t));
    BAIL_IF_MACRO(f == NULL, ERR_OUT_OF_MEMORY, 0);
    
    decoder = FLAC__stream_decoder_new();
    if (decoder == NULL)
    {
        Sound_SetError(ERR_OUT_OF_MEMORY);
        free(f);
        return(0);
    } /* if */       

    FLAC__stream_decoder_set_read_callback(decoder, FLAC_read_callback);
    FLAC__stream_decoder_set_write_callback(decoder, FLAC_write_callback);
    FLAC__stream_decoder_set_metadata_callback(decoder, FLAC_metadata_callback);
    FLAC__stream_decoder_set_error_callback(decoder, FLAC_error_callback);
    FLAC__stream_decoder_set_client_data(decoder, f);

    f->rw = internal->rw;
    f->sample = sample;
    f->decoder = decoder;
    f->sample->actual.format = 0;
    f->is_flac = 0 /* !!! FIXME: should be "has_extension", not "0". */;

    internal->decoder_private = f;
    FLAC__stream_decoder_init(decoder);

        /*
         * If we are not sure this is a FLAC stream, check for the STREAMINFO
         * metadata block. If not, we'd have to peek at the first audio frame
         * and get the sound format from there, but that is not yet
         * implemented.
         */
    if (!f->is_flac)
    {
        FLAC__stream_decoder_process_metadata(decoder);

        /* Still not FLAC? Give up. */
        if (!f->is_flac)
        {
            Sound_SetError("FLAC: No metadata found. Not a FLAC stream?");
            free_flac(f);
            return(0);
        } /* if */
    } /* if */

    SNDDBG(("FLAC: Accepting data stream.\n"));

    sample->flags = SOUND_SAMPLEFLAG_NONE;
    return(1);
} /* FLAC_open */


static void FLAC_close(Sound_Sample *sample)
{
    Sound_SampleInternal *internal = (Sound_SampleInternal *) sample->opaque;
    flac_t *f = (flac_t *) internal->decoder_private;

    free_flac(f);
} /* FLAC_close */


static Uint32 FLAC_read(Sound_Sample *sample)
{
    Sound_SampleInternal *internal = (Sound_SampleInternal *) sample->opaque;
    flac_t *f = (flac_t *) internal->decoder_private;
    Uint32 len;

    if (FLAC__stream_decoder_get_state(f->decoder) == FLAC__STREAM_DECODER_END_OF_STREAM)
    {
        sample->flags |= SOUND_SAMPLEFLAG_EOF;
        return(0);
    } /* if */

    if (!FLAC__stream_decoder_process_one_frame(f->decoder))
    {
        Sound_SetError("FLAC: Couldn't decode frame.");
        sample->flags |= SOUND_SAMPLEFLAG_ERROR;
        return(0);
    } /* if */

        /* An error may have been signalled through the error callback. */    
    if (sample->flags & SOUND_SAMPLEFLAG_ERROR)
        return(0);
    
    return(f->frame_size);
} /* FLAC_read */

#endif /* SOUND_SUPPORTS_FLAC */


/* end of flac.c ... */
