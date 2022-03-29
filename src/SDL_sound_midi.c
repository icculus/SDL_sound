/**
 * SDL_sound; An abstract sound format decoding API.
 *
 * Please see the file LICENSE.txt in the source's root directory.
 *
 *  This file written by Ryan C. Gordon.
 */

/*
 * MIDI decoder for SDL_sound.
 * !!! FIXME write more
 *
 * Built from scratch from these documents:
 *   https://www.personal.kent.edu/~sbirch/Music_Production/MP-II/MIDI/midi_file_format.htm
 */

#define __SDL_SOUND_INTERNAL__
#include "SDL_sound_internal.h"

#if SOUND_SUPPORTS_MIDI

#define MIDI_CHUNK_HEADER_ID 0x4D546864  /* "MThd", in bigendian. */
#define MIDI_CHUNK_TRACK_ID  0x4D54726B  /* "MTrk", in bigendian. */


/* !!! FIXME: mostly a copy/paste from .wav decoder */
/* Better than SDL_ReadBE16, since you can detect i/o errors... */
static SDL_INLINE SDL_bool read_be16(SDL_RWops *rw, Uint16 *ui16)
{
    int rc = SDL_RWread(rw, ui16, sizeof (Uint16), 1);
    BAIL_IF_MACRO(rc != 1, ERR_IO_ERROR, SDL_FALSE);
    *ui16 = SDL_SwapBE16(*ui16);
    return SDL_TRUE;
} /* read_be16 */


/* Better than SDL_ReadBE32, since you can detect i/o errors... */
static SDL_INLINE SDL_bool read_be32(SDL_RWops *rw, Uint32 *ui32)
{
    int rc = SDL_RWread(rw, ui32, sizeof (Uint32), 1);
    BAIL_IF_MACRO(rc != 1, ERR_IO_ERROR, SDL_FALSE);
    *ui32 = SDL_SwapBE32(*ui32);
    return SDL_TRUE;
} /* read_be32 */

static SDL_INLINE SDL_bool read_be16s(SDL_RWops *rw, Sint16 *si16)
{
    return read_be16(rw, (Uint16 *) si16);
} /* read_be16s */

static SDL_INLINE SDL_bool read_be32s(SDL_RWops *rw, Sint32 *si32)
{
    return read_be32(rw, (Uint32 *) si32);
} /* read_be32s */


/* This is just cleaner on the caller's end... */
static SDL_INLINE SDL_bool read_uint8(SDL_RWops *rw, Uint8 *ui8)
{
    int rc = SDL_RWread(rw, ui8, sizeof (Uint8), 1);
    BAIL_IF_MACRO(rc != 1, ERR_IO_ERROR, SDL_FALSE);
    return SDL_TRUE;
} /* read_uint8 */

/* "variable-length quantities" ... kind of like UTF-8. */
static SDL_bool read_vlq(SDL_RWops *rw, Uint32 *_ui32)
{
    Uint32 val = 0;
    Uint8 ui8;

    BAIL_IF_MACRO(SDL_RWread(rw, &ui8, sizeof (Uint8), 1) != 1, ERR_IO_ERROR, SDL_FALSE);
    val = (Uint32) ui8;
    if ((ui8 & 0x80) == 0) { *_ui32 = val; return SDL_TRUE; }
    BAIL_IF_MACRO(SDL_RWread(rw, &ui8, sizeof (Uint8), 1) != 1, ERR_IO_ERROR, SDL_FALSE);
    val = (val << 7) | ((Uint32) ui8);
    if ((ui8 & 0x80) == 0) { *_ui32 = val; return SDL_TRUE; }
    BAIL_IF_MACRO(SDL_RWread(rw, &ui8, sizeof (Uint8), 1) != 1, ERR_IO_ERROR, SDL_FALSE);
    val = (val << 7) | ((Uint32) ui8);
    if ((ui8 & 0x80) == 0) { *_ui32 = val; return SDL_TRUE; }
    BAIL_IF_MACRO(SDL_RWread(rw, &ui8, sizeof (Uint8), 1) != 1, ERR_IO_ERROR, SDL_FALSE);
    val = (val << 7) | ((Uint32) ui8);
    if ((ui8 & 0x80) == 0) { *_ui32 = val; return SDL_TRUE; }

    *_ui32 = 0;
    return SDL_FALSE;  // these are capped at 0x0FFFFFFF (four blocks of seven bits each). If there wasn't a terminating 0 bit, this is corrupted.
}


static SDL_bool MIDI_init(void)
{
    return SDL_TRUE;  /* always succeeds. */
} /* MIDI_init */


static void MIDI_quit(void)
{
    /* it's a no-op. */
} /* MIDI_quit */



static SDL_bool read_midi_track(SDL_RWops *rw, const Uint32 chunklen)
{
    Uint32 delta_time;
    const Sint64 endpos = SDL_RWtell(rw) + chunklen;
    while (SDL_RWtell(rw) <= endpos)
    {
        Uint8 running_status = 0;
        Uint8 event_type;
        Uint32 event_len = 0;
        if (!read_vlq(rw, &delta_time)) { return SDL_FALSE; }
        if (!read_uint8(rw, &event_type)) { return SDL_FALSE; }
        if ((event_type == 0xF0) || (event_type == 0xF7))  /* sysex event */
        {
            if (!read_vlq(rw, &event_len)) { return SDL_FALSE; }
            SNDDBG(("MIDI: sysex event, %u bytes", (unsigned int) event_len));
            if (event_len) { SDL_RWseek(rw, event_len, RW_SEEK_CUR); }
            running_status = 0;  /* sysex events cancel any running status */
        } /* if */
        else if (event_type == 0xFF)
        {
            Uint8 meta_event_type;
            if (!read_uint8(rw, &meta_event_type)) { return SDL_FALSE; }
            if (!read_vlq(rw, &event_len)) { return SDL_FALSE; }
            SNDDBG(("MIDI: meta event 0x%X, %u bytes", (unsigned int) meta_event_type, (unsigned int) event_len));
            if (event_len) { SDL_RWseek(rw, event_len, RW_SEEK_CUR); }
            running_status = 0;  /* meta events cancel any running status */
        } /* else if */
        else
        {
            if ((event_type & 0x80) == 0)   /* if high bit isn't set, this is data for the current running status, not a new event. */
                event_type = running_status;
            else
                running_status = event_type;   /* if high bit _is_ set, however, this becomes the new running status. */

            const Uint8 channel = event_type & 0xF;
            const Uint8 msg = event_type >> 4;
            Uint8 data1, data2;
            switch (msg)
            {
                case 0x8:   /* note off */
                    if (!read_uint8(rw, &data1)) { return SDL_FALSE; }
                    if (!read_uint8(rw, &data2)) { return SDL_FALSE; }
                    SNDDBG(("MIDI: note off event channel %u, key=%u, velocity=%u", (unsigned int) channel, (unsigned int) data1, (unsigned int) data2));
                    break;

                case 0x9:   /* note on */
                    if (!read_uint8(rw, &data1)) { return SDL_FALSE; }
                    if (!read_uint8(rw, &data2)) { return SDL_FALSE; }
                    SNDDBG(("MIDI: note on event channel %u, key=%u, velocity=%u", (unsigned int) channel, (unsigned int) data1, (unsigned int) data2));
                    break;

                case 0xA:   /* polyphonic key pressure */
                    if (!read_uint8(rw, &data1)) { return SDL_FALSE; }
                    if (!read_uint8(rw, &data2)) { return SDL_FALSE; }
                    SNDDBG(("MIDI: poly key pressure event channel %u, key=%u, pressure=%u", (unsigned int) channel, (unsigned int) data1, (unsigned int) data2));
                    break;

                case 0xB:   /* controller change */
                    if (!read_uint8(rw, &data1)) { return SDL_FALSE; }
                    if (!read_uint8(rw, &data2)) { return SDL_FALSE; }
                    if (data1 < 0x78)  /* controller change event */
                    {
                        SNDDBG(("MIDI: controller change event channel %u, controller=%u, value=%u", (unsigned int) channel, (unsigned int) data1, (unsigned int) data2));
                    } /* if */
                    else  /* channel mode message? */
                    {
                        switch (data1)
                        {
                            case 0x78: SNDDBG(("MIDI: channel mode all sound off event channel %u", (unsigned int) channel)); break;
                            case 0x79: SNDDBG(("MIDI: channel mode reset all controllers event channel %u", (unsigned int) channel)); break;
                            case 0x7A: SNDDBG(("MIDI: channel mode local control event channel %u, value=%u", (unsigned int) channel, (unsigned int) data2)); break;
                            case 0x7B: SNDDBG(("MIDI: channel mode all notes off event channel %u", (unsigned int) channel)); break;
                            case 0x7C: SNDDBG(("MIDI: channel mode omni mode off event channel %u", (unsigned int) channel)); break;
                            case 0x7D: SNDDBG(("MIDI: channel mode omni mode on event channel %u", (unsigned int) channel)); break;
                            case 0x7E: SNDDBG(("MIDI: channel mode mono mode on event channel %u, channel_count=%u", (unsigned int) channel, (unsigned int) data2)); break;
                            case 0x7F: SNDDBG(("MIDI: channel mode poly mode on event channel %u", (unsigned int) channel)); break;
                            default: SNDDBG(("MIDI: channel mode UNKNOWN event channel %u, value=%u", (unsigned int) channel, (unsigned int) data2)); break;
                        } /* switch */
                    } /* else */
                    break;

                case 0xC:   /* program change */
                    if (!read_uint8(rw, &data1)) { return SDL_FALSE; }
                    if (!read_uint8(rw, &data2)) { return SDL_FALSE; }
                    SNDDBG(("MIDI: program change event channel %u, preset=%u", (unsigned int) channel, (unsigned int) data1));
                    break;

                case 0xD:   /* channel key pressure */
                    if (!read_uint8(rw, &data1)) { return SDL_FALSE; }
                    if (!read_uint8(rw, &data2)) { return SDL_FALSE; }
                    SNDDBG(("MIDI: channel key pressure event channel %u, pressure=%u", (unsigned int) channel, (unsigned int) data1));
                    break;

                case 0xE:   /* pitch bend */
                    if (!read_uint8(rw, &data1)) { return SDL_FALSE; }
                    if (!read_uint8(rw, &data2)) { return SDL_FALSE; }
                    SNDDBG(("MIDI: pitch bend event channel %u, coarse=%u, fine=%u", (unsigned int) channel, (unsigned int) data1, (unsigned int) data2));
                    break;

                default:
                    SNDDBG(("MIDI: UNKNOWN event channel %u, data1=%u, data2=%u", (unsigned int) channel, (unsigned int) data1, (unsigned int) data2));
                    break;
            } /* switch */
        } /* else */
    } /* while */

    return SDL_TRUE;
}

static SDL_bool read_chunk(SDL_RWops *rw, Uint32 *_chunkid, Uint32 *_chunklen, Sint64 *_nextchunkpos)
{
    SDL_assert(_chunkid != NULL);
    SDL_assert(_chunklen != NULL);
    SDL_assert(_nextchunkpos != NULL);
    BAIL_IF_MACRO(SDL_RWseek(rw, *_nextchunkpos, RW_SEEK_SET) != *_nextchunkpos, "MIDI: Couldn't seek to next chunk!", SDL_FALSE);
    if (!read_be32(rw, _chunkid) || !read_be32(rw, _chunklen))
        return SDL_FALSE;
    *_nextchunkpos += *_chunklen + 8;
    SNDDBG(("MIDI: CHUNK '%c%c%c%c', %u bytes",
            (int) ((*_chunkid >> 24) & 0xFF),
            (int) ((*_chunkid >> 16) & 0xFF),
            (int) ((*_chunkid >> 8) & 0xFF),
            (int) (*_chunkid & 0xFF),
            (unsigned int) *_chunklen));
    return SDL_TRUE;
}

static int MIDI_open(Sound_Sample *sample, const char *ext)
{
    Sound_SampleInternal *internal = sample->opaque;
    SDL_RWops *rw = internal->rw;
    Uint32 chunkid, chunklen;
    Sint64 nextchunkpos = SDL_RWtell(rw);
    Uint16 format, num_tracks, division;

    BAIL_IF_MACRO(nextchunkpos == -1, "MIDI: Can't determine initial file position!", SDL_FALSE);

    if (!read_chunk(rw, &chunkid, &chunklen, &nextchunkpos)) { return 0; }
    BAIL_IF_MACRO(chunkid != MIDI_CHUNK_HEADER_ID, "MIDI: unexpected chunk looking for header", 0);   // !!! FIXME: does this _have_ to be the first chunk?
    if (!read_be16(rw, &format) || !read_be16(rw, &num_tracks) || !read_be16(rw, &division)) { return 0; }
    BAIL_IF_MACRO(format > 2, "MIDI: Unknown file format type", 0);

    SNDDBG(("MIDI: format=%u, num_tracks=%u, division=%u", (unsigned int) format, (unsigned int) num_tracks, (unsigned int) division));

    while (read_chunk(rw, &chunkid, &chunklen, &nextchunkpos))
    {
        if (chunkid == MIDI_CHUNK_TRACK_ID)
        {
            SNDDBG(("MIDI: This is a Track chunk"));
            read_midi_track(rw, chunklen);
        } /* if */
        else
        {
            SNDDBG(("MIDI: UNKNOWN CHUNK"));
        } /* else */
    } /* while */


    return 0;

    return 1; /* we'll handle this data. */
} /* MIDI_open */


static void MIDI_close(Sound_Sample *sample)
{
    /* we don't allocate anything that we need to free. That's easy, eh? */
} /* MIDI_close */


static Uint32 MIDI_read(Sound_Sample *sample)
{
    Uint32 retval;
    Sound_SampleInternal *internal = (Sound_SampleInternal *) sample->opaque;

        /*
         * We don't actually do any decoding, so we read the raw data
         *  directly into the internal buffer...
         */
    retval = SDL_RWread(internal->rw, internal->buffer,
                        1, internal->buffer_size);

        /* Make sure the read went smoothly... */
    if (retval == 0)
        sample->flags |= SOUND_SAMPLEFLAG_EOF;

    else if (retval == -1) /** FIXME: this error check is broken **/
        sample->flags |= SOUND_SAMPLEFLAG_ERROR;

        /* (next call this EAGAIN may turn into an EOF or error.) */
    else if (retval < internal->buffer_size)
        sample->flags |= SOUND_SAMPLEFLAG_EAGAIN;

    return retval;
} /* MIDI_read */


static int MIDI_rewind(Sound_Sample *sample)
{
    Sound_SampleInternal *internal = (Sound_SampleInternal *) sample->opaque;
    BAIL_IF_MACRO(SDL_RWseek(internal->rw, 0, RW_SEEK_SET) != 0, ERR_IO_ERROR, 0);
    return 1;
} /* MIDI_rewind */


static int MIDI_seek(Sound_Sample *sample, Uint32 ms)
{
    Sound_SampleInternal *internal = (Sound_SampleInternal *) sample->opaque;
    const Sint64 pos = __Sound_convertMsToBytePos(&sample->actual, ms);
    const int err = (SDL_RWseek(internal->rw, pos, RW_SEEK_SET) != pos);
    BAIL_IF_MACRO(err, ERR_IO_ERROR, 0);
    return 1;
} /* MIDI_seek */

static const char *extensions_midi[] = { "MIDI", NULL };
const Sound_DecoderFunctions __Sound_DecoderFunctions_MIDI =
{
    {
        extensions_midi,
        "MIDI audio",
        "Ryan C. Gordon <icculus@icculus.org>",
        "https://icculus.org/SDL_sound/"
    },

    MIDI_init,       /*   init() method */
    MIDI_quit,       /*   quit() method */
    MIDI_open,       /*   open() method */
    MIDI_close,      /*  close() method */
    MIDI_read,       /*   read() method */
    MIDI_rewind,     /* rewind() method */
    MIDI_seek        /*   seek() method */
};

#endif /* SOUND_SUPPORTS_MIDI */

/* end of SDL_sound_midi.c ... */

