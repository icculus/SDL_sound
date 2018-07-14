/**
 * SDL_sound; A sound processing toolkit.
 *
 * Please see the file LICENSE.txt in the source's root directory.
 *
 *  This file written by Ryan C. Gordon.
 */

/**
 * This file implements the mixer itself. Largely, this is handled in the
 *  SDL audio callback.
 */

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "SDL.h"
#include "SDL_thread.h"
#include "SDL_sound.h"

#define __SDL_SOUND_INTERNAL__
#include "SDL_sound_internal.h"


static void conv_mix_buf_u8_mono(void *userdata, Uint8 *_stream, int len)
{
    register Uint8 *stream = _stream;
    register Uint32 i;
    register Uint32 max = len * 2;
    for (i = 0; i < max; i += 2)
    {
        *stream = (Uint8) ((((mixbuf[i]+mixbuf[i+1])*0.5f) * 127.0f) + 128.0f);
        stream++;
    } /* for */
} /* conv_mix_buf_s8s */

static void conv_mix_buf_s8_mono(void *userdata, Uint8 *_stream, int len)
{
    register Uint32 i;
    register Sint8 *stream = (Sint8 *) _stream;
    register Uint32 max = len * 2;
    for (i = 0; i < max; i += 2)
    {
        *stream = (Sint8) (((mixbuf[i] + mixbuf[i+1]) * 0.5f) * 127.0f);
        stream++;
    } /* for */
} /* conv_mix_buf_s8s */

static void conv_mix_buf_s16_lsb_mono(void *userdata, Uint8 *_stream, int len)
{
    register Uint32 i;
    register Sint16 *stream = (Sint16 *) _stream;
    register Sint16 val;
    register Uint32 max = len / 2;
    for (i = 0; i < max; i += 2)
    {
        val = (Sint16) (((mixbuf[i] + mixbuf[i+1]) * 0.5f) * 32767.0f);
        *stream = SDL_SwapLE16(val);
        stream++;
    } /* for */
} /* conv_mix_buf_s16_lsb_mono */

static void conv_mix_buf_s16_msb_mono(void *userdata, Uint8 *_stream, int len)
{
    register Uint32 i;
    register Sint16 *stream = (Sint16 *) _stream;
    register Sint16 val;
    register Uint32 max = len / 2;
    for (i = 0; i < max; i += 2)
    {
        val = (Sint16) (((mixbuf[i] + mixbuf[i+1]) * 0.5f) * 32767.0f);
        *stream = SDL_SwapBE16(val);
        stream++;
    } /* for */
} /* conv_mix_buf_s16_msb_mono */

static void conv_mix_buf_u16_lsb_mono(void *userdata, Uint8 *_stream, int len)
{
    register Uint32 i;
    register Uint16 *stream = (Uint16 *) _stream;
    register Uint16 val;
    register Uint32 max = len / 2;
    for (i = 0; i < max; i += 2)
    {
        val = (Uint16)((((mixbuf[i]+mixbuf[i+1])*0.5f) * 32767.0f) + 32768.0f);
        *stream = SDL_SwapLE16(val);
        stream++;
    } /* for */
} /* conv_mix_buf_s16_lsb_mono */

static void conv_mix_buf_u16_msb_mono(void *userdata, Uint8 *_stream, int len)
{
    register Uint32 i;
    register Uint16 *stream = (Uint16 *) _stream;
    register Uint16 val;
    register Uint32 max = len / 2;
    for (i = 0; i < max; i += 2)
    {
        val = (Uint16)((((mixbuf[i]+mixbuf[i+1])*0.5f) * 32767.0f) + 32768.0f);
        *stream = SDL_SwapBE16(val);
        stream++;
    } /* for */
} /* conv_mix_buf_s16_lsb_mono */

static void conv_mix_buf_u8_stereo(void *userdata, Uint8 *_stream, int len)
{
    register Uint32 i;
    register Uint8 *stream = _stream;
    register Uint32 max = len;
    for (i = 0; i < max; i++)
    {
        *stream = (Uint8) ((mixbuf[i] * 127.0f) + 128.0f);
        stream++;
    } /* for */
} /* conv_mix_buf_s8s */

static void conv_mix_buf_s8_stereo(void *userdata, Uint8 *_stream, int len)
{
    register Uint32 i;
    register Sint8 *stream = (Sint8 *) _stream;
    register Uint32 max = len;
    for (i = 0; i < max; i++)
    {
        *stream = (Sint8) (mixbuf[i] * 127.0f);
        stream++;
    } /* for */
} /* conv_mix_buf_s8s */

static void conv_mix_buf_s16lsb_stereo(void *userdata, Uint8 *_stream, int len)
{
    register Uint32 i;
    register Sint16 *stream = (Sint16 *) _stream;
    register Sint16 val;
    register Uint32 max = len / 2;
    for (i = 0; i < max; i++)
    {
        val = (Sint16) (mixbuf[i] * 32767.0f);
        *stream = SDL_SwapLE16(val);
        stream++;
    } /* for */
} /* conv_mix_buf_s16_lsb_stereo */

static void conv_mix_buf_s16msb_stereo(void *userdata, Uint8 *_stream, int len)
{
    register Uint32 i;
    register Sint16 *stream = (Sint16 *) _stream;
    register Sint16 val;
    register Uint32 max = len / 2;
    for (i = 0; i < max; i++)
    {
        val = (Sint16) (mixbuf[i] * 32767.0f);
        *stream = SDL_SwapBE16(val);
        stream++;
    } /* for */
} /* conv_mix_buf_s16_msb_stereo */

static void conv_mix_buf_u16lsb_stereo(void *userdata, Uint8 *_stream, int len)
{
    register Uint32 i;
    register Uint16 *stream = (Uint16 *) _stream;
    register Uint16 val;
    register Uint32 max = len / 2;
    for (i = 0; i < max; i++)
    {
        val = (Uint16) ((mixbuf[i] * 32767.0f) + 32768.0f);
        *stream = SDL_SwapLE16(val);
        stream++;
    } /* for */
} /* conv_mix_buf_s16_lsb_stereo */

static void conv_mix_buf_u16msb_stereo(void *userdata, Uint8 *_stream, int len)
{
    register Uint32 i;
    register Uint16 *stream = (Uint16 *) _stream;
    register Uint16 val;
    register Uint32 max = len / 2;
    for (i = 0; i < max; i++)
    {
        val = (Uint16) ((mixbuf[i] * 32767.0f) + 32768.0f);
        *stream = SDL_SwapBE16(val);
        stream++;
    } /* for */
} /* conv_mix_buf_s16_msb_stereo */

/* end of converters.c ... */

