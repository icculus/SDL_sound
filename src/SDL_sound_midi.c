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

typedef struct
{
    const Uint8 *data;  /* points to chunk just past ID and chunk length, start of actual data. Not owned; don't free! */
    Uint32 datalen;
    Uint8 running_status;
    const Uint8 *readpos;
} MidiTrack;

typedef enum
{
    MIDIFMT_SINGLE_TRACK=0,
    MIDIFMT_SIMULTANEOUS_TRACKS=1,
    MIDIFMT_SEQUENTIAL_TRACKS=2,
    MIDIFMT_MAX
} MidiFormat;

typedef struct
{
    MidiTrack *tracks;
    Uint16 num_tracks;
    MidiFormat format;
    Uint16 division;
} midi_t;


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


/* "variable-length quantities" ... kind of like UTF-8. */
static SDL_bool get_vlq(MidiTrack *track, Uint32 *_ui32)
{
    const size_t remain = track->datalen - ((size_t) (track->readpos - track->data));
    const int avail = (remain > 4) ? 4 : (int) remain;
    Uint32 val = 0;
    int i;

    for (i = 0; i < avail; i++)
    {
        const Uint8 ui8 = *(track->readpos++);
        val = (val << 7) | ((Uint32) ui8);  /* we can safely shift on the first byte since we're just shifting zero. */
        if ((ui8 & 0x80) == 0)
        {
            *_ui32 = val;
            return SDL_TRUE;
        } /* if */
    } /* for */

    /* these are capped at 0x0FFFFFFF (four blocks of seven bits each). If there wasn't a terminating 0 bit, this is corrupted. */
    *_ui32 = 0;
    BAIL_MACRO("MIDI: corrupt or truncated file", SDL_FALSE);
    return SDL_FALSE;
} /* get_vlq */

static SDL_INLINE SDL_bool get_uint8(MidiTrack *track, Uint8 *_ui8)
{
    BAIL_IF_MACRO(track->readpos >= (track->data + track->datalen), "MIDI: corrupt or truncated file", SDL_FALSE);
    *_ui8 = *(track->readpos++);
    return SDL_TRUE;
} /* get_uint8 */


static SDL_bool MIDI_init(void)
{
    return SDL_TRUE;  /* always succeeds. */
} /* MIDI_init */


static void MIDI_quit(void)
{
    /* it's a no-op. */
} /* MIDI_quit */


static SDL_bool read_midi_track(MidiTrack *track)
{
    const Uint8 *endpos = track->data + track->datalen;
    while (track->readpos < endpos)
    {
        Uint32 delta_time;
        Uint8 event_type;
        Uint32 event_len = 0;

        if (!get_vlq(track, &delta_time)) { return SDL_FALSE; }
        if (!get_uint8(track, &event_type)) { return SDL_FALSE; }

        if ((event_type == 0xF0) || (event_type == 0xF7))  /* sysex event */
        {
            if (!get_vlq(track, &event_len)) { return SDL_FALSE; }
            SNDDBG(("MIDI: sysex event, %u bytes", (unsigned int) event_len));
            if (event_len) { track->readpos += event_len; }
            track->running_status = 0;  /* sysex events cancel any running status */
        } /* if */
        else if (event_type == 0xFF)
        {
            Uint8 meta_event_type;
            if (!get_uint8(track, &meta_event_type)) { return SDL_FALSE; }
            if (!get_vlq(track, &event_len)) { return SDL_FALSE; }
            SNDDBG(("MIDI: meta event 0x%X, %u bytes", (unsigned int) meta_event_type, (unsigned int) event_len));
            if (event_len) { track->readpos += event_len; }
            track->running_status = 0;  /* meta events cancel any running status */
        } /* else if */
        else
        {
            Uint8 channel, msg, data1, data2;
            SDL_bool is_running_status = SDL_FALSE;
            if ((event_type & 0x80) == 0)   /* if high bit isn't set, this is data for the current running status, not a new event. */
            {
                event_type = track->running_status;
                track->readpos--;  /* re-read the data byte below. */
                is_running_status = SDL_TRUE;
            } /* if */
            else
            {
                track->running_status = event_type;   /* if high bit _is_ set, however, this becomes the new running status. */
            } /* else */

            channel = event_type & 0xF;
            msg = event_type >> 4;

            switch (msg)
            {
                case 0x8:   /* note off */
                    if (!get_uint8(track, &data1)) { return SDL_FALSE; }
                    if (!get_uint8(track, &data2)) { return SDL_FALSE; }
                    SNDDBG(("MIDI: note off event channel %u, key=%u, velocity=%u", (unsigned int) channel, (unsigned int) data1, (unsigned int) data2));
                    break;

                case 0x9:   /* note on */
                    if (!get_uint8(track, &data1)) { return SDL_FALSE; }
                    if (!get_uint8(track, &data2)) { return SDL_FALSE; }
                    SNDDBG(("MIDI: note on event channel %u, key=%u, velocity=%u", (unsigned int) channel, (unsigned int) data1, (unsigned int) data2));
                    break;

                case 0xA:   /* polyphonic key pressure */
                    if (!get_uint8(track, &data1)) { return SDL_FALSE; }
                    if (!get_uint8(track, &data2)) { return SDL_FALSE; }
                    SNDDBG(("MIDI: poly key pressure event channel %u, key=%u, pressure=%u", (unsigned int) channel, (unsigned int) data1, (unsigned int) data2));
                    break;

                case 0xB:   /* controller change */
                    if (!get_uint8(track, &data1)) { return SDL_FALSE; }
                    if (!get_uint8(track, &data2)) { return SDL_FALSE; }
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
                    if (!get_uint8(track, &data1)) { return SDL_FALSE; }
                    if (!get_uint8(track, &data2)) { return SDL_FALSE; }
                    SNDDBG(("MIDI: program change event channel %u, preset=%u", (unsigned int) channel, (unsigned int) data1));
                    break;

                case 0xD:   /* channel key pressure */
                    if (!get_uint8(track, &data1)) { return SDL_FALSE; }
                    if (!get_uint8(track, &data2)) { return SDL_FALSE; }
                    SNDDBG(("MIDI: channel key pressure event channel %u, pressure=%u", (unsigned int) channel, (unsigned int) data1));
                    break;

                case 0xE:   /* pitch bend */
                    if (!get_uint8(track, &data1)) { return SDL_FALSE; }
                    if (!get_uint8(track, &data2)) { return SDL_FALSE; }
                    SNDDBG(("MIDI: pitch bend event channel %u, coarse=%u, fine=%u", (unsigned int) channel, (unsigned int) data1, (unsigned int) data2));
                    break;

                default:
                    SNDDBG(("MIDI: UNKNOWN event 0x%X channel %u, data1=%u, data2=%u", (unsigned int) msg, (unsigned int) channel, (unsigned int) data1, (unsigned int) data2));
                    if (is_running_status)  /* dump the byte we just pushed back so we make progress. */
                    {
                        if (!get_uint8(track, &data1)) { return SDL_FALSE; }
                    } /* if */
                    break;
            } /* switch */
        } /* else */
    } /* while */

    return SDL_TRUE;
} /* read_midi_track */

static Uint8 *read_midi_chunk(SDL_RWops *rw, Uint32 *_chunkid, Uint32 *_chunklen)
{
    Uint8 *retval = NULL;
    Sint64 rc;

    SDL_assert(_chunkid != NULL);
    SDL_assert(_chunklen != NULL);

    if (!read_be32(rw, _chunkid) || !read_be32(rw, _chunklen))
        return NULL;

    retval = (Uint8 *) SDL_malloc(*_chunklen);
    BAIL_IF_MACRO(retval == NULL, "MIDI: Out of memory", NULL);
    if ((rc = SDL_RWread(rw, retval, *_chunklen, 1)) != 1)
    {
        SDL_free(retval);
        BAIL_MACRO((rc == -1) ? SDL_GetError() : "MIDI: file is corrupt or truncated", NULL);
    } /* if */

    SNDDBG(("MIDI: CHUNK '%c%c%c%c', %u bytes", (int) ((*_chunkid >> 24) & 0xFF), (int) ((*_chunkid >> 16) & 0xFF), (int) ((*_chunkid >> 8) & 0xFF), (int) (*_chunkid & 0xFF), (unsigned int) *_chunklen));

    return retval;
} /* read_midi_chunk */

static SDL_bool read_midi_header(SDL_RWops *rw, MidiFormat *_format, Uint16 *_num_tracks, Uint16 *_division)
{
    Uint32 chunkid, chunklen;
    Uint8 *chunk = read_midi_chunk(rw, &chunkid, &chunklen);
    Uint16 *ptr = (Uint16 *) chunk;

    if (!chunk)
        return SDL_FALSE;  /* read_midi_chunk sets the error message */

    if (chunkid != MIDI_CHUNK_HEADER_ID)
    {
        SDL_free(chunk);
        BAIL_MACRO("MIDI: unexpected chunk looking for header", SDL_FALSE);
    } /* if */
    else if (chunklen < 6)
    {
        SDL_free(chunk);
        BAIL_MACRO("MIDI: header chunk is too small. Corrupt file?", SDL_FALSE);
    } /* else if */

    *_format = (MidiFormat) SDL_SwapBE16(ptr[0]);
    *_num_tracks = SDL_SwapBE16(ptr[1]);
    *_division = SDL_SwapBE16(ptr[2]);
    SDL_free(chunk);

    SNDDBG(("MIDI: format=%u, num_tracks=%u, division=%u", (unsigned int) *_format, (unsigned int) *_num_tracks, (unsigned int) *_division));

    BAIL_IF_MACRO(*_format >= MIDIFMT_MAX, "MIDI: Unknown file format type", SDL_FALSE);
    BAIL_IF_MACRO(*_num_tracks == 0, "MIDI: no tracks to play", SDL_FALSE);

    return SDL_TRUE;
} /* read_midi_header */

static SDL_bool read_all_midi_chunks(SDL_RWops *rw, midi_t *m)
{
    /* MIDI files are pretty small and we usually need to be able to process multiple chunks at once, so load them all in up front. */
    int num_tracks = 0;
    Uint32 chunkid, chunklen;
    Uint8 *chunk;

    while ((chunk = read_midi_chunk(rw, &chunkid, &chunklen)) != NULL)
    {
        if (chunkid == MIDI_CHUNK_TRACK_ID)
        {
            SNDDBG(("MIDI: This is a Track chunk"));
            m->tracks[num_tracks].data = chunk;
            m->tracks[num_tracks].datalen = chunklen;
            m->tracks[num_tracks].readpos = chunk;
            read_midi_track(&m->tracks[num_tracks]);
            num_tracks++;
            if (num_tracks >= m->num_tracks)
                return SDL_TRUE;  /* we're done! */
        } /* if */
        else
        {
            SNDDBG(("MIDI: UNKNOWN CHUNK"));
            SDL_free(chunk);  /* ignore unknown chunks, carry on. */
        } /* else */
    } /* while */

    return SDL_FALSE;  /* we hit an unexpected EOF or i/o error */
} /* read_all_midi_chunks */

static int MIDI_open(Sound_Sample *sample, const char *ext)
{
    Sound_SampleInternal *internal = sample->opaque;
    SDL_RWops *rw = internal->rw;
    MidiFormat format;
    Uint16 num_tracks, division;
    Uint32 chunkid = 0;
    midi_t *m = NULL;

    if (!read_be32(rw, &chunkid)) {
        return 0;  /* read_be32 sets the error message */
    } else if (chunkid != MIDI_CHUNK_HEADER_ID) {
        BAIL_MACRO("MIDI: not a MIDI file", 0);
    } else if (SDL_RWseek(rw, -(sizeof (Uint32)), RW_SEEK_CUR) == -1) {
        BAIL_MACRO("MIDI: i/o error", 0);
    } else if (!read_midi_header(rw, &format, &num_tracks, &division)) {
        return 0;  /* read_midi_header sets the error message */
    }

    SNDDBG(("MIDI: Accepting data stream."));

    m = (midi_t *) SDL_calloc(1, sizeof (midi_t));
    BAIL_IF_MACRO(m == NULL, "MIDI: out of memory", 0);

    m->tracks = (MidiTrack *) SDL_calloc(num_tracks, sizeof (MidiTrack));
    if (m->tracks == NULL)
    {
        SDL_free(m);
        BAIL_MACRO("MIDI: out of memory", 0);
    } /* if */

    m->format = format;
    m->num_tracks = num_tracks;
    m->division = division;

    if (!read_all_midi_chunks(rw, m))
    {
        SDL_free(m->tracks);
        SDL_free(m);
        return 0;  /* read_all_midi_chunks sets the error message */
    } /* if */

    // !!! FIXME: sample->flags = SOUND_SAMPLEFLAG_CANSEEK;

    /* !!! FIXME: I guess? */
    sample->actual.channels = 1;
    sample->actual.rate = 44100;
    sample->actual.format = AUDIO_F32SYS;

    internal->total_time = -1;  /* !!! FIXME? */
    internal->decoder_private = m;


    return 0;

//    return 1; /* we'll handle this data. */
} /* MIDI_open */


static void MIDI_close(Sound_Sample *sample)
{
    Sound_SampleInternal *internal = sample->opaque;
    midi_t *m = (midi_t *) internal->decoder_private;
    if (m)
    {
        if (m->tracks)
        {
            Uint16 i;
            for (i = 0; i < m->num_tracks; i++)
                SDL_free((void *) m->tracks[i].data);
            SDL_free(m->tracks);
        } /* if */

        SDL_free(m);
    } /* if */
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

static const char *extensions_midi[] = { "MID", "MIDI", NULL };
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

