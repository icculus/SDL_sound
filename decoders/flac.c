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
 * Please see the file LICENSE in the source's root directory.
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
    Uint8 metadata_found;
} flac_t;


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
    Uint8 *dst;

#if 0
    SNDDBG(("FLAC: Write callback.\n"));
#endif

    f->frame_size = frame->header.channels * frame->header.blocksize
        * frame->header.bits_per_sample / 8;

    if (f->frame_size > f->sample->buffer_size)
        Sound_SetBufferSize(f->sample, f->frame_size);

    dst = f->sample->buffer;

    if (frame->header.bits_per_sample == 8)
    {
        for (i = 0; i < frame->header.blocksize; i++)
            for (j = 0; j < frame->header.channels; j++)
                *dst++ = buffer[j][i] & 0x000000ff;
    } /* if */
    else
    {
        for (i = 0; i < frame->header.blocksize; i++)
            for (j = 0; j < frame->header.channels; j++)
            {
                *dst++ = (buffer[j][i] & 0x0000ff00) >> 8;
                *dst++ = buffer[j][i] & 0x000000ff;
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

    if (metadata->type == FLAC__METADATA_TYPE_STREAMINFO)
    {
        SNDDBG(("FLAC: Metadata is streaminfo.\n"));
        f->metadata_found = 1;
        f->sample->actual.channels = metadata->data.stream_info.channels;
        f->sample->actual.rate = metadata->data.stream_info.sample_rate;

            /* !!! FIXME: I believe bits_per_sample may be anywhere between
             * 4 and 24. We can only handle 8 and 16 at present.
             */
        switch (metadata->data.stream_info.bits_per_sample)
        {
            case 8:
                f->sample->actual.format = AUDIO_S8;
                break;
            case 16:
                f->sample->actual.format = AUDIO_S16MSB;
                break;
            default:
                Sound_SetError("FLAC: Unsupported sample width.");
                f->sample->actual.format = 0;
                break;
        } /* switch */
    } /* if */
} /* FLAC_metadata_callback */


void FLAC_error_callback(
    const FLAC__StreamDecoder *decoder, FLAC__StreamDecoderErrorStatus status,
    void *client_data)
{
    flac_t *f = (flac_t *) client_data;

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


static int FLAC_open(Sound_Sample *sample, const char *ext)
{
    Sound_SampleInternal *internal = (Sound_SampleInternal *) sample->opaque;
    SDL_RWops *rw = internal->rw;
    FLAC__StreamDecoder *decoder;
    flac_t *f;

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
    
    FLAC__stream_decoder_init(decoder);
    internal->decoder_private = f;

    f->metadata_found = 0;
    f->sample->actual.format = 0;
    FLAC__stream_decoder_process_metadata(decoder);

    if (f->sample->actual.format == 0)
    {
        if (f->metadata_found == 0)
            Sound_SetError("FLAC: No metadata found.");
        FLAC__stream_decoder_finish(decoder);
        FLAC__stream_decoder_delete(decoder);
        free(f);
        return(0);
    } /* if */

    SNDDBG(("FLAC: Accepting data stream.\n"));

    sample->flags = SOUND_SAMPLEFLAG_NONE;
    return(1);
} /* FLAC_open */


static void FLAC_close(Sound_Sample *sample)
{
    Sound_SampleInternal *internal = (Sound_SampleInternal *) sample->opaque;
    flac_t *f = (flac_t *) internal->decoder_private;

    FLAC__stream_decoder_finish(f->decoder);
    FLAC__stream_decoder_delete(f->decoder);
    free(f);
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
