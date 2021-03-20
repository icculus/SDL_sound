/**
 * SDL_sound; An abstract sound format decoding API.
 *
 * Please see the file LICENSE.txt in the source's root directory.
 *
 *  This file written by Torbjörn Andersson.
 */

/*
 * This driver handles MIDI data through a stripped-down version of TiMidity.
 *  See the documentation in the timidity subdirectory.
 */

#define __SDL_SOUND_INTERNAL__
#include "SDL_sound_internal.h"

#if SOUND_SUPPORTS_MIDI

#include "timidity/timidity.h"


/* Config file should contain any other directory that needs
 * to be added to the search path. The library adds the path
 * of the config file to its search path, too. */
#if defined(_WIN32) || defined(__OS2__)
# define TIMIDITY_CFG           "C:\\TIMIDITY\\TIMIDITY.CFG"
#else  /* unix: */
# define TIMIDITY_CFG_ETC       "/etc/timidity.cfg"
# define TIMIDITY_CFG_FREEPATS  "/etc/timidity/freepats.cfg"
#endif

static int MIDI_init(void)
{
    const char *cfg;
    int rc = -1;

    cfg = SDL_getenv("TIMIDITY_CFG");
    if (cfg) {
        rc = Timidity_Init(cfg); /* env or user override: no other tries */
    }
    else {
#if defined(TIMIDITY_CFG)
        if (rc < 0) rc = Timidity_Init(TIMIDITY_CFG);
#endif
#if defined(TIMIDITY_CFG_ETC)
        if (rc < 0) rc = Timidity_Init(TIMIDITY_CFG_ETC);
#endif
#if defined(TIMIDITY_CFG_FREEPATS)
        if (rc < 0) rc = Timidity_Init(TIMIDITY_CFG_FREEPATS);
#endif
        if (rc < 0) rc = Timidity_Init(NULL); /* library's default cfg. */
    }

    BAIL_IF_MACRO(rc < 0, "MIDI: Could not initialise", 0);
    return(1);
} /* MIDI_init */


static void MIDI_quit(void)
{
    Timidity_Exit();
} /* MIDI_quit */


static int MIDI_open(Sound_Sample *sample, const char *ext)
{
    Sound_SampleInternal *internal = (Sound_SampleInternal *) sample->opaque;
    SDL_RWops *rw = internal->rw;
    SDL_AudioSpec spec;
    MidiSong *song;

    spec.channels = 2;
    spec.format = AUDIO_S16SYS;
    spec.freq = 44100;
    spec.samples = 4096;
    
    song = Timidity_LoadSong(rw, &spec);
    BAIL_IF_MACRO(song == NULL, "MIDI: Not a MIDI file.", 0);
    Timidity_SetVolume(song, 100);
    Timidity_Start(song);

    SNDDBG(("MIDI: Accepting data stream.\n"));

    internal->decoder_private = (void *) song;
    internal->total_time = Timidity_GetSongLength(song);

    sample->actual.channels = 2;
    sample->actual.rate = 44100;
    sample->actual.format = AUDIO_S16SYS;
    sample->flags = SOUND_SAMPLEFLAG_CANSEEK;
    
    return(1); /* we'll handle this data. */
} /* MIDI_open */


static void MIDI_close(Sound_Sample *sample)
{
    Sound_SampleInternal *internal = (Sound_SampleInternal *) sample->opaque;
    MidiSong *song = (MidiSong *) internal->decoder_private;

    Timidity_FreeSong(song);
} /* MIDI_close */


static Uint32 MIDI_read(Sound_Sample *sample)
{
    Uint32 retval;
    Sound_SampleInternal *internal = (Sound_SampleInternal *) sample->opaque;
    MidiSong *song = (MidiSong *) internal->decoder_private;

    retval = Timidity_PlaySome(song, internal->buffer, internal->buffer_size);

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


static int MIDI_rewind(Sound_Sample *sample)
{
    Sound_SampleInternal *internal = (Sound_SampleInternal *) sample->opaque;
    MidiSong *song = (MidiSong *) internal->decoder_private;
    
    Timidity_Start(song);
    return(1);
} /* MIDI_rewind */


static int MIDI_seek(Sound_Sample *sample, Uint32 ms)
{
    Sound_SampleInternal *internal = (Sound_SampleInternal *) sample->opaque;
    MidiSong *song = (MidiSong *) internal->decoder_private;

    Timidity_Seek(song, ms);
    return(1);
} /* MIDI_seek */


static const char *extensions_midi[] = { "MIDI", "MID", NULL };
const Sound_DecoderFunctions __Sound_DecoderFunctions_MIDI =
{
    {
        extensions_midi,
        "MIDI decoder, using a subset of TiMidity",
        "Torbjörn Andersson <d91tan@Update.UU.SE>",
        "http://www.icculus.org/SDL_sound/"
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

