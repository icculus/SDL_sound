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
 * VOC decoder for SDL_sound.
 *
 * This driver handles Creative Labs VOC audio data...this is a legacy format,
 *  but there's some game ports that could make use of such a decoder. Plus,
 *  VOC is fairly straightforward to decode, so this is a more complex, but
 *  still palatable example of an SDL_sound decoder. Y'know, in case the
 *  RAW decoder didn't do it for you.  :)
 *
 * This code was ripped from a decoder I had written for SDL_mixer, which was
 *  largely ripped from sox v12.17.1's voc.c.
 *
 *    SDL_mixer: http://www.libsdl.org/projects/SDL_mixer/
 *    sox: http://www.freshmeat.net/projects/sox/
 *
 * Please see the file COPYING in the source's root directory.
 *
 *  This file written by Ryan C. Gordon. (icculus@clutteredmind.org)
 */

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#ifdef SOUND_SUPPORTS_VOC

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "SDL_sound.h"

#define __SDL_SOUND_INTERNAL__
#include "SDL_sound_internal.h"

static int VOC_init(void);
static void VOC_quit(void);
static int VOC_open(Sound_Sample *sample, const char *ext);
static void VOC_close(Sound_Sample *sample);
static Uint32 VOC_read(Sound_Sample *sample);
static int VOC_rewind(Sound_Sample *sample);

static const char *extensions_voc[] = { "VOC", NULL };
const Sound_DecoderFunctions __Sound_DecoderFunctions_VOC =
{
    {
        extensions_voc,
        "Creative Labs Voice format",
        "Ryan C. Gordon <icculus@clutteredmind.org>",
        "http://www.icculus.org/SDL_sound/"
    },

    VOC_init,       /*   init() method */
    VOC_quit,       /*   quit() method */
    VOC_open,       /*   open() method */
    VOC_close,      /*  close() method */
    VOC_read,       /*   read() method */
    VOC_rewind      /* rewind() method */
};


/* Private data for VOC file */
typedef struct vocstuff {
    Uint32	rest;           /* bytes remaining in current block */
    Uint32	rate;           /* rate code (byte) of this chunk */
    int 	silent;         /* sound or silence? */
    Uint32	srate;			/* rate code (byte) of silence */
    Uint32	blockseek;		/* start of current output block */
    Uint32	samples;		/* number of samples output */
    Uint32	size;           /* word length of data */
    Uint8 	channels;       /* number of sound channels */
    int     extended;       /* Has an extended block been read? */
    Uint32  bufpos;         /* byte position in internal->buffer. */
} vs_t;

/* Size field */ 
/* SJB: note that the 1st 3 are sometimes used as sizeof(type) */
#define ST_SIZE_BYTE     1
#define ST_SIZE_8BIT     1
#define ST_SIZE_WORD     2
#define ST_SIZE_16BIT    2
#define ST_SIZE_DWORD    4
#define ST_SIZE_32BIT    4
#define ST_SIZE_FLOAT    5
#define ST_SIZE_DOUBLE   6
#define ST_SIZE_IEEE     7   /* IEEE 80-bit floats. */

/* Style field */
#define ST_ENCODING_UNSIGNED  1 /* unsigned linear: Sound Blaster */
#define ST_ENCODING_SIGN2     2 /* signed linear 2's comp: Mac */
#define ST_ENCODING_ULAW      3 /* U-law signed logs: US telephony, SPARC */
#define ST_ENCODING_ALAW      4 /* A-law signed logs: non-US telephony */
#define ST_ENCODING_ADPCM     5 /* Compressed PCM */
#define ST_ENCODING_IMA_ADPCM 6 /* Compressed PCM */
#define ST_ENCODING_GSM       7 /* GSM 6.10 33-byte frame lossy compression */

#define VOC_TERM      0
#define VOC_DATA      1
#define VOC_CONT      2
#define VOC_SILENCE   3
#define VOC_MARKER    4
#define VOC_TEXT      5
#define VOC_LOOP      6
#define VOC_LOOPEND   7
#define VOC_EXTENDED  8
#define VOC_DATA_16   9


static int VOC_init(void)
{
    return(1);  /* always succeeds. */
} /* VOC_init */


static void VOC_quit(void)
{
    /* it's a no-op. */
} /* VOC_quit */


static __inline__ int voc_check_header(SDL_RWops *src)
{
    /* VOC magic header */
    Uint8  signature[20];  /* "Creative Voice File\032" */
    Uint16 datablockofs;

    if (SDL_RWread(src, signature, sizeof (signature), 1) != 1)
        return(0);

    if (memcmp(signature, "Creative Voice File\032", sizeof (signature)) != 0) {
        Sound_SetError("Unrecognized file type (not VOC)");
        return(0);
    }

        /* get the offset where the first datablock is located */
    if (SDL_RWread(src, &datablockofs, sizeof (Uint16), 1) != 1)
        return(0);

    datablockofs = SDL_SwapLE16(datablockofs);

    if (SDL_RWseek(src, datablockofs, SEEK_SET) != datablockofs)
        return(0);

    return(1);  /* success! */
} /* voc_check_header */

/* !!! FIXME : Add a flag to vs_t that distinguishes EOF and Error conditions. */

/* Read next block header, save info, leave position at start of data */
static int voc_get_block(Sound_Sample *sample)
{
    Sound_SampleInternal *internal = (Sound_SampleInternal *) sample->opaque;
    SDL_RWops *src = internal->rw;
    vs_t *v = (vs_t *) internal->decoder_private;
    Uint8 bits24[3];
    Uint8 uc, block;
    Uint32 sblen;
    Uint16 new_rate_short;
    Uint32 new_rate_long;
    Uint8 trash[6];
    Uint16 period;
    int i;

    v->silent = 0;
    while (v->rest == 0)
    {
        if (SDL_RWread(src, &block, sizeof (block), 1) != 1)
            return 1;  /* assume that's the end of the file. */

        if (block == VOC_TERM)
            return 1;

        if (SDL_RWread(src, bits24, sizeof (bits24), 1) != 1)
            return 1;  /* assume that's the end of the file. */
        
        /* Size is an 24-bit value. Ugh. */
        sblen = ( (bits24[0]) | (bits24[1] << 8) | (bits24[2] << 16) );

        switch(block)
        {
            case VOC_DATA:
                if (SDL_RWread(src, &uc, sizeof (uc), 1) != 1)
                    return 0;

                /* When DATA block preceeded by an EXTENDED     */
                /* block, the DATA blocks rate value is invalid */
                if (!v->extended)
                {
                    if (uc == 0)
                    {
                        Sound_SetError("VOC Sample rate is zero?");
                        return 0;
                    }

                    if ((v->rate != -1) && (uc != v->rate))
                    {
                        Sound_SetError("VOC sample rate codes differ");
                        return 0;
                    }

                    v->rate = uc;
                    sample->actual.rate = 1000000.0/(256 - v->rate);
                    v->channels = 1;
                }

                if (SDL_RWread(src, &uc, sizeof (uc), 1) != 1)
                    return 0;

                if (uc != 0)
                {
                    Sound_SetError("VOC decoder only interprets 8-bit data");
                    return 0;
                }

                v->extended = 0;
                v->rest = sblen - 2;
                v->size = ST_SIZE_BYTE;
                return 1;

            case VOC_DATA_16:
                if (SDL_RWread(src, &new_rate_long, sizeof (new_rate_long), 1) != 1)
                    return 0;
                new_rate_long = SDL_SwapLE32(new_rate_long);
                if (new_rate_long == 0)
                {
                    Sound_SetError("VOC Sample rate is zero?");
                    return 0;
                }
                if ((v->rate != -1) && (new_rate_long != v->rate))
                {
                    Sound_SetError("VOC sample rate codes differ");
                    return 0;
                }
                v->rate = new_rate_long;
                sample->actual.rate = new_rate_long;

                if (SDL_RWread(src, &uc, sizeof (uc), 1) != 1)
                    return 0;

                switch (uc)
                {
                    case 8:  v->size = ST_SIZE_BYTE; break;
                    case 16: v->size = ST_SIZE_WORD; break;
                    default:
                        Sound_SetError("VOC with unknown data size");
                        return 0;
                }

                if (SDL_RWread(src, &v->channels, sizeof (Uint8), 1) != 1)
                    return 0;

                if (SDL_RWread(src, trash, sizeof (Uint8), 6) != 6)
                    return 0;

                v->rest = sblen - 12;
                return 1;

            case VOC_CONT:
                v->rest = sblen;
                return 1;

            case VOC_SILENCE:
                if (SDL_RWread(src, &period, sizeof (period), 1) != 1)
                    return 0;
                period = SDL_SwapLE16(period);

                if (SDL_RWread(src, &uc, sizeof (uc), 1) != 1)
                    return 0;
                if (uc == 0)
                {
                    Sound_SetError("VOC silence sample rate is zero");
                    return 0;
                }

                /*
                 * Some silence-packed files have gratuitously
                 * different sample rate codes in silence.
                 * Adjust period.
                 */
                if ((v->rate != -1) && (uc != v->rate))
                    period = (period * (256 - uc))/(256 - v->rate);
                else
                    v->rate = uc;
                v->rest = period;
                v->silent = 1;
                return 1;

            case VOC_LOOP:
            case VOC_LOOPEND:
                for(i = 0; i < sblen; i++)   /* skip repeat loops. */
                {
                    if (SDL_RWread(src, trash, sizeof (Uint8), 1) != 1)
                        return 0;
                }
                break;

            case VOC_EXTENDED:
                /* An Extended block is followed by a data block */
                /* Set this byte so we know to use the rate      */
                /* value from the extended block and not the     */
                /* data block.                     */
                v->extended = 1;
                if (SDL_RWread(src, &new_rate_short, sizeof (new_rate_short), 1) != 1)
                    return 0;
                new_rate_short = SDL_SwapLE16(new_rate_short);
                if (new_rate_short == 0)
                {
                   Sound_SetError("VOC sample rate is zero");
                   return 0;
                }
                if ((v->rate != -1) && (new_rate_short != v->rate))
                {
                   Sound_SetError("VOC sample rate codes differ");
                   return 0;
                }
                v->rate = new_rate_short;

                if (SDL_RWread(src, &uc, sizeof (uc), 1) != 1)
                    return 0;

                if (uc != 0)
                {
                    Sound_SetError("VOC decoder only interprets 8-bit data");
                    return 0;
                }

                if (SDL_RWread(src, &uc, sizeof (uc), 1) != 1)
                    return 0;

                if (uc)
                    sample->actual.channels = 2;  /* Stereo */
                /* Needed number of channels before finishing
                   compute for rate */
                sample->actual.rate =
                     (256000000L/(65536L - v->rate)) / sample->actual.channels;
                /* An extended block must be followed by a data */
                /* block to be valid so loop back to top so it  */
                /* can be grabed.                */
                continue;

            case VOC_MARKER:
                if (SDL_RWread(src, trash, sizeof (Uint8), 2) != 2)
                    return 0;

                /* Falling! Falling! */

            default:  /* text block or other krapola. */
                for(i = 0; i < sblen; i++)
                {
                    if (SDL_RWread(src, &trash, sizeof (Uint8), 1) != 1)
                        return 0;
                }

                if (block == VOC_TEXT)
                    continue;    /* get next block */
        }
    }

    return 1;
}


static int voc_read_waveform(Sound_Sample *sample)
{
    Sound_SampleInternal *internal = (Sound_SampleInternal *) sample->opaque;
    SDL_RWops *src = internal->rw;
    vs_t *v = (vs_t *) internal->decoder_private;
    int done = 0;
    Uint8 silence = 0x80;
    Uint8 *buf = internal->buffer;

    if (v->rest == 0)
    {
        if (!voc_get_block(sample))
            return 0;
    }

    if (v->rest == 0)
        return 0;

    if (v->silent)
    {
        if (v->size == ST_SIZE_WORD)
            silence = 0x00;

        /* Fill in silence */
        memset(buf, silence, v->rest);
        done = v->rest;
        v->rest = 0;
    }

    else
    {
        Uint32 max = (v->rest < internal->buffer_size) ?
                        v->rest : internal->buffer_size;
        done = SDL_RWread(src, buf + v->bufpos, 1, max);
        v->rest -= done;
        v->bufpos += done;
    }

    return done;
} /* voc_read_waveform */


static int VOC_open(Sound_Sample *sample, const char *ext)
{
    Sound_SampleInternal *internal = (Sound_SampleInternal *) sample->opaque;
    vs_t *v = NULL;

    if (!voc_check_header(internal->rw))
        return(0);

    v = (vs_t *) malloc(sizeof (vs_t));
    BAIL_IF_MACRO(v == NULL, ERR_OUT_OF_MEMORY, 0);
    memset(v, '\0', sizeof (vs_t));
    internal->decoder_private = v;

    v->rate = -1;
    if (!voc_get_block(sample))
    {
        free(v);
        return(0);
    } /* if */

    if (v->rate == -1)
    {
        Sound_SetError("VOC data had no sound!");
        free(v);
        return(0);
    } /* if */

    SNDDBG(("VOC: Accepting data stream.\n"));
    sample->actual.format = (v->size == ST_SIZE_WORD) ? AUDIO_S16LSB:AUDIO_U8;
    sample->actual.channels = v->channels;
    sample->flags = SOUND_SAMPLEFLAG_NONE;
    return(1);
} /* VOC_open */


static void VOC_close(Sound_Sample *sample)
{
    Sound_SampleInternal *internal = (Sound_SampleInternal *) sample->opaque;
    free(internal->decoder_private);
} /* VOC_close */


static Uint32 VOC_read(Sound_Sample *sample)
{
    Sound_SampleInternal *internal = (Sound_SampleInternal *) sample->opaque;
    vs_t *v = (vs_t *) internal->decoder_private;

    v->bufpos = 0;
    while (v->bufpos < internal->buffer_size)
    {
        Uint32 rc = voc_read_waveform(sample);
        if (rc == 0)  /* !!! FIXME: Could be an error... */
        {
            sample->flags |= SOUND_SAMPLEFLAG_EOF;
            break;
        } /* if */

        if (!voc_get_block(sample))
        {
            sample->flags |= SOUND_SAMPLEFLAG_ERROR;
            break;
        } /* if */
    } /* while */

    return(v->bufpos);
} /* VOC_read */


static int VOC_rewind(Sound_Sample *sample)
{
    /* !!! FIXME. */
    SNDDBG(("VOC_rewind(): Write me!\n"));
    assert(0);
    return(0);
} /* VOC_rewind */

#endif /* SOUND_SUPPORTS_VOC */

/* end of voc.c ... */
