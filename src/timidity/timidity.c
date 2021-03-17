/*

    TiMidity -- Experimental MIDI to WAVE converter
    Copyright (C) 1995 Tuukka Toivonen <toivonen@clinet.fi>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the Perl Artistic License, available in COPYING.
*/

#define __SDL_SOUND_INTERNAL__
#include "SDL_sound_internal.h"

#include "timidity.h"

#include "options.h"
#include "common.h"
#include "instrum.h"
#include "playmidi.h"
#include "readmidi.h"
#include "output.h"

#include "tables.h"

ToneBank *master_tonebank[128], *master_drumset[128];

static char def_instr_name[256] = "";

#define MAXWORDS 10

/* Quick-and-dirty fgets() replacement. */

static char *RWgets(SDL_RWops *rw, char *s, int size)
{
    int num_read = 0;
    char *p = s;

    --size;/* so that we nul terminate properly */

    for (; num_read < size; ++p)
    {
	if (SDL_RWread(rw, p, 1, 1) != 1)
	    break;

	num_read++;

	/* Unlike fgets(), don't store newline. Under Windows/DOS we'll
	 * probably get an extra blank line for every line that's being
	 * read, but that should be ok.
	 */
	if (*p == '\n' || *p == '\r')
	{
	    *p = '\0';
	    return s;
	}
    }

    *p = '\0';

    return (num_read != 0) ? s : NULL;
}

#if !(SDL_VERSION_ATLEAST(2,0,12))
/*
 * Adapted from _PDCLIB_strtok() of PDClib library at
 * https://github.com/DevSolar/pdclib.git
 *
 * The code was under CC0 license:
 * https://creativecommons.org/publicdomain/zero/1.0/legalcode :
 *
 *                        No Copyright
 *
 * The person who associated a work with this deed has dedicated the
 * work to the public domain by waiving all of his or her rights to
 * the work worldwide under copyright law, including all related and
 * neighboring rights, to the extent allowed by law.
 *
 * You can copy, modify, distribute and perform the work, even for
 * commercial purposes, all without asking permission. See Other
 * Information below.
 */
static char *SDL_strtokr(char *s1, const char *s2, char **ptr)
{
    const char *p = s2;

    if (!s2 || !ptr || (!s1 && !*ptr)) return NULL;

    if (s1 != NULL) {  /* new string */
        *ptr = s1;
    } else { /* old string continued */
        if (*ptr == NULL) {
        /* No old string, no new string, nothing to do */
            return NULL;
        }
        s1 = *ptr;
    }

    /* skip leading s2 characters */
    while (*p && *s1) {
        if (*s1 == *p) {
        /* found separator; skip and start over */
            ++s1;
            p = s2;
            continue;
        }
        ++p;
    }

    if (! *s1) { /* no more to parse */
        *ptr = s1;
        return NULL;
    }

    /* skipping non-s2 characters */
    *ptr = s1;
    while (**ptr) {
        p = s2;
        while (*p) {
            if (**ptr == *p++) {
            /* found separator; overwrite with '\0', position *ptr, return */
                *((*ptr)++) = '\0';
                return s1;
            }
        }
        ++(*ptr);
    }

    /* parsed to end of string */
    return s1;
}
#endif /* HAVE_SDL_STRTOKR */

static int read_config_file(const char *name)
{
  SDL_RWops *rw;
  char tmp[1024], *w[MAXWORDS], *cp;
  char *endp;
  ToneBank *bank=0;
  int i, j, k, line=0, words;
  static int rcf_count=0;

  if (rcf_count>50)
  {
    SNDDBG(("Probable source loop in configuration files\n"));
    return (-1);
  }

  if (!(rw=open_file(name)))
   return -1;

  while (RWgets(rw, tmp, sizeof(tmp)))
  {
    line++;
    words=0;
    w[0]=SDL_strtokr(tmp, " \t\240", &endp);
    if (!w[0]) continue;

        /* Originally the TiMidity++ extensions were prefixed like this */
    if (SDL_strcmp(w[0], "#extension") == 0)
    {
        w[0]=SDL_strtokr(0, " \t\240", &endp);
        if (!w[0]) continue;
    }

    if (*w[0] == '#')
        continue;

    while (w[words] && *w[words] != '#') {
      if (++words == MAXWORDS) break;
      w[words]=SDL_strtokr(NULL, " \t\240", &endp);
    }

        /*
         * TiMidity++ adds a number of extensions to the config file format.
         * Many of them are completely irrelevant to SDL_sound, but at least
         * we shouldn't choke on them.
         *
         * Unfortunately the documentation for these extensions is often quite
         * vague, gramatically strange or completely absent.
         */
    if (
           !SDL_strcmp(w[0], "comm")      /* "comm" program second        */
        || !SDL_strcmp(w[0], "HTTPproxy") /* "HTTPproxy" hostname:port    */
        || !SDL_strcmp(w[0], "FTPproxy")  /* "FTPproxy" hostname:port     */
        || !SDL_strcmp(w[0], "mailaddr")  /* "mailaddr" your-mail-address */
        || !SDL_strcmp(w[0], "opt")       /* "opt" timidity-options       */
       )
    {
            /*
             * + "comm" sets some kind of comment -- the documentation is too
             *   vague for me to understand at this time.
             * + "HTTPproxy", "FTPproxy" and "mailaddr" are for reading data
             *   over a network, rather than from the file system.
             * + "opt" specifies default options for TiMidity++.
             *
             * These are all quite useless for our version of TiMidity, so
             * they can safely remain no-ops.
             */
    } else if (!SDL_strcmp(w[0], "timeout")) /* "timeout" program second */
    {
            /*
             * Specifies a timeout value of the program. A number of seconds
             * before TiMidity kills the note. This may be useful to implement
             * later, but I don't see any urgent need for it.
             */
        SNDDBG(("FIXME: Implement \"timeout\" in TiMidity config.\n"));
    } else if (!SDL_strcmp(w[0], "copydrumset")  /* "copydrumset" drumset */
               || !SDL_strcmp(w[0], "copybank")) /* "copybank" bank       */
    {
            /*
             * Copies all the settings of the specified drumset or bank to
             * the current drumset or bank. May be useful later, but not a
             * high priority.
             */
        SNDDBG(("FIXME: Implement \"%s\" in TiMidity config.\n", w[0]));
    } else if (!SDL_strcmp(w[0], "undef")) /* "undef" progno */
    {
            /*
             * Undefines the tone "progno" of the current tone bank (or
             * drum set?). Not a high priority.
             */
        SNDDBG(("FIXME: Implement \"undef\" in TiMidity config.\n"));
    } else if (!SDL_strcmp(w[0], "altassign")) /* "altassign" prog1 prog2 ... */
    {
            /*
             * Sets the alternate assign for drum set. Whatever that's
             * supposed to mean.
             */
        SNDDBG(("FIXME: Implement \"altassign\" in TiMidity config.\n"));
    } else if (!SDL_strcmp(w[0], "soundfont")
               || !SDL_strcmp(w[0], "font"))
    {
            /*
             * I can't find any documentation for these, but I guess they're
             * an alternative way of loading/unloading instruments.
             * 
             * "soundfont" sf_file "remove"
             * "soundfont" sf_file ["order=" order] ["cutoff=" cutoff]
             *                     ["reso=" reso] ["amp=" amp]
             * "font" "exclude" bank preset keynote
             * "font" "order" order bank preset keynote
             */
        SNDDBG(("FIXME: Implmement \"%s\" in TiMidity config.\n", w[0]));
    } else if (!SDL_strcmp(w[0], "progbase"))
    {
            /*
             * The documentation for this makes absolutely no sense to me, but
             * apparently it sets some sort of base offset for tone numbers.
             * Why anyone would want to do this is beyond me.
             */
        SNDDBG(("FIXME: Implement \"progbase\" in TiMidity config.\n"));
    } else if (!SDL_strcmp(w[0], "map")) /* "map" name set1 elem1 set2 elem2 */
    {
            /*
             * This extension is the one we will need to implement, as it is
             * used by the "eawpats". Unfortunately I cannot find any
             * documentation whatsoever for it, but it looks like it's used
             * for remapping one instrument to another somehow.
             */
        SNDDBG(("FIXME: Implement \"map\" in TiMidity config.\n"));
    }

        /* Standard TiMidity config */
    
    else if (!SDL_strcmp(w[0], "dir"))
    {
      if (words < 2)
      {
	SNDDBG(("%s: line %d: No directory given\n", name, line));
	goto fail;
      }
      for (i=1; i<words; i++)
	add_to_pathlist(w[i], SDL_strlen(w[i]));
    }
    else if (!SDL_strcmp(w[0], "source"))
    {
      if (words < 2)
      {
	SNDDBG(("%s: line %d: No file name given\n", name, line));
	goto fail;
      }
      for (i=1; i<words; i++)
      {
	int status;
	rcf_count++;
	status = read_config_file(w[i]);
	rcf_count--;
	if (status != 0) {
	  SDL_RWclose(rw);
	  return status;
	}
      }
    }
    else if (!SDL_strcmp(w[0], "default"))
    {
      if (words != 2)
      {
	SNDDBG(("%s: line %d: Must specify exactly one patch name\n",
		name, line));
	goto fail;
      }
      SDL_strlcpy(def_instr_name, w[1], 256);
    }
    else if (!SDL_strcmp(w[0], "drumset"))
    {
      if (words < 2)
      {
	SNDDBG(("%s: line %d: No drum set number given\n", name, line));
	goto fail;
      }
      i=SDL_atoi(w[1]);
      if (i<0 || i>127)
      {
	SNDDBG(("%s: line %d: Drum set must be between 0 and 127\n",
		name, line));
	goto fail;
      }
      if (!master_drumset[i])
      {
	master_drumset[i] = safe_malloc(sizeof(ToneBank));
	SDL_memset(master_drumset[i], 0, sizeof(ToneBank));
	master_drumset[i]->tone = safe_malloc(128 * sizeof(ToneBankElement));
	SDL_memset(master_drumset[i]->tone, 0, 128 * sizeof(ToneBankElement));
      }
      bank=master_drumset[i];
    }
    else if (!SDL_strcmp(w[0], "bank"))
    {
      if (words < 2)
      {
	SNDDBG(("%s: line %d: No bank number given\n", name, line));
	goto fail;
      }
      i=SDL_atoi(w[1]);
      if (i<0 || i>127)
      {
	SNDDBG(("%s: line %d: Tone bank must be between 0 and 127\n",
		name, line));
	goto fail;
      }
      if (!master_tonebank[i])
      {
	master_tonebank[i] = safe_malloc(sizeof(ToneBank));
	SDL_memset(master_tonebank[i], 0, sizeof(ToneBank));
	master_tonebank[i]->tone = safe_malloc(128 * sizeof(ToneBankElement));
	SDL_memset(master_tonebank[i]->tone, 0, 128 * sizeof(ToneBankElement));
      }
      bank=master_tonebank[i];
    }
    else
    {
      size_t sz;
      if ((words < 2) || (*w[0] < '0' || *w[0] > '9'))
      {
	SNDDBG(("%s: line %d: syntax error\n", name, line));
	goto fail;
      }
      i=SDL_atoi(w[0]);
      if (i<0 || i>127)
      {
	SNDDBG(("%s: line %d: Program must be between 0 and 127\n",
		name, line));
	goto fail;
      }
      if (!bank)
      {
	SNDDBG(("%s: line %d: Must specify tone bank or drum set before assignment\n",
		name, line));
	goto fail;
      }
      if (bank->tone[i].name)
	SDL_free(bank->tone[i].name);
      sz = SDL_strlen(w[1])+1;
      bank->tone[i].name=safe_malloc(sz);
      SDL_memcpy(bank->tone[i].name,w[1],sz);
      bank->tone[i].note=bank->tone[i].amp=bank->tone[i].pan=
      bank->tone[i].strip_loop=bank->tone[i].strip_envelope=
      bank->tone[i].strip_tail=-1;

      for (j=2; j<words; j++)
      {
	if (!(cp=SDL_strchr(w[j], '=')))
	{
	  SNDDBG(("%s: line %d: bad patch option %s\n", name, line, w[j]));
	  goto fail;
	}
	*cp++=0;
	if (!SDL_strcmp(w[j], "amp"))
	{
	  k=SDL_atoi(cp);
	  if ((k<0 || k>MAX_AMPLIFICATION) || (*cp < '0' || *cp > '9'))
	  {
	    SNDDBG(("%s: line %d: amplification must be between 0 and %d\n",
		    name, line, MAX_AMPLIFICATION));
	    goto fail;
	  }
	  bank->tone[i].amp=k;
	}
	else if (!SDL_strcmp(w[j], "note"))
	{
	  k=SDL_atoi(cp);
	  if ((k<0 || k>127) || (*cp < '0' || *cp > '9'))
	  {
	    SNDDBG(("%s: line %d: note must be between 0 and 127\n",
		    name, line));
	    goto fail;
	  }
	  bank->tone[i].note=k;
	}
	else if (!SDL_strcmp(w[j], "pan"))
	{
	  if (!SDL_strcmp(cp, "center"))
	    k=64;
	  else if (!SDL_strcmp(cp, "left"))
	    k=0;
	  else if (!SDL_strcmp(cp, "right"))
	    k=127;
	  else
	    k=((SDL_atoi(cp)+100) * 100) / 157;
	  if ((k<0 || k>127) || (k==0 && *cp!='-' && (*cp < '0' || *cp > '9')))
	  {
	    SNDDBG(("%s: line %d: panning must be left, right, center, or between -100 and 100\n",
		    name, line));
	    goto fail;
	  }
	  bank->tone[i].pan=k;
	}
	else if (!SDL_strcmp(w[j], "keep"))
	{
	  if (!SDL_strcmp(cp, "env"))
	    bank->tone[i].strip_envelope=0;
	  else if (!SDL_strcmp(cp, "loop"))
	    bank->tone[i].strip_loop=0;
	  else
	  {
	    SNDDBG(("%s: line %d: keep must be env or loop\n", name, line));
	    goto fail;
	  }
	}
	else if (!SDL_strcmp(w[j], "strip"))
	{
	  if (!SDL_strcmp(cp, "env"))
	    bank->tone[i].strip_envelope=1;
	  else if (!SDL_strcmp(cp, "loop"))
	    bank->tone[i].strip_loop=1;
	  else if (!SDL_strcmp(cp, "tail"))
	    bank->tone[i].strip_tail=1;
	  else
	  {
	    SNDDBG(("%s: line %d: strip must be env, loop, or tail\n",
		    name, line));
	    goto fail;
	  }
	}
	else
	{
	  SNDDBG(("%s: line %d: bad patch option %s\n", name, line, w[j]));
	  goto fail;
	}
      }
    }
  }
  SDL_RWclose(rw);
  return 0;
fail:
  SDL_RWclose(rw);
  return -2;
}

int Timidity_Init_NoConfig(void)
{
  /* Allocate memory for the standard tonebank and drumset */
  master_tonebank[0] = safe_malloc(sizeof(ToneBank));
  SDL_memset(master_tonebank[0], 0, sizeof(ToneBank));
  master_tonebank[0]->tone = safe_malloc(128 * sizeof(ToneBankElement));
  SDL_memset(master_tonebank[0]->tone, 0, 128 * sizeof(ToneBankElement));

  master_drumset[0] = safe_malloc(sizeof(ToneBank));
  SDL_memset(master_drumset[0], 0, sizeof(ToneBank));
  master_drumset[0]->tone = safe_malloc(128 * sizeof(ToneBankElement));
  SDL_memset(master_drumset[0]->tone, 0, 128 * sizeof(ToneBankElement));

  return 0;
}

int Timidity_Init(const char *config_file)
{
  const char *p;

  Timidity_Init_NoConfig();

  if (config_file == NULL || *config_file == '\0')
      config_file = TIMIDITY_CFG;

  p = SDL_strrchr(config_file, '/');
#if defined(_WIN32)||defined(__OS2__)
  if (!p) p = SDL_strrchr(config_file, '\\');
#endif
  if (p != NULL)
    add_to_pathlist(config_file, p - config_file + 1);

  if (read_config_file(config_file) < 0) {
      Timidity_Exit();
      return -1;
  }
  return 0;
}

MidiSong *Timidity_LoadSong(SDL_RWops *rw, SDL_AudioSpec *audio)
{
  MidiSong *song;
  int i;

  if (rw == NULL)
      return NULL;

  /* Allocate memory for the song */
  song = (MidiSong *)safe_malloc(sizeof(*song));
  if (song == NULL)
      return NULL;
  SDL_memset(song, 0, sizeof(*song));

  for (i = 0; i < 128; i++)
  {
    if (master_tonebank[i])
    {
      song->tonebank[i] = safe_malloc(sizeof(ToneBank));
      SDL_memset(song->tonebank[i], 0, sizeof(ToneBank));
      song->tonebank[i]->tone = master_tonebank[i]->tone;
    }
    if (master_drumset[i])
    {
      song->drumset[i] = safe_malloc(sizeof(ToneBank));
      SDL_memset(song->drumset[i], 0, sizeof(ToneBank));
      song->drumset[i]->tone = master_drumset[i]->tone;
    }
  }

  song->amplification = DEFAULT_AMPLIFICATION;
  song->voices = DEFAULT_VOICES;
  song->drumchannels = DEFAULT_DRUMCHANNELS;

  song->rw = rw;

  song->rate = audio->freq;
  song->encoding = 0;
  if ((audio->format & 0xFF) == 16)
      song->encoding |= PE_16BIT;
  else if ((audio->format & 0xFF) == 32)
      song->encoding |= PE_32BIT;
  if (audio->format & 0x8000)
      song->encoding |= PE_SIGNED;
  if (audio->channels == 1)
      song->encoding |= PE_MONO;
  else if (audio->channels > 2) {
      SDL_SetError("Surround sound not supported");
      SDL_free(song);
      return NULL;
  }
  switch (audio->format) {
  case AUDIO_S8:
	  song->write = s32tos8;
	  break;
  case AUDIO_U8:
	  song->write = s32tou8;
	  break;
  case AUDIO_S16LSB:
	  song->write = s32tos16l;
	  break;
  case AUDIO_S16MSB:
	  song->write = s32tos16b;
	  break;
  case AUDIO_U16LSB:
	  song->write = s32tou16l;
	  break;
  case AUDIO_U16MSB:
	  song->write = s32tou16b;
	  break;
  case AUDIO_S32LSB:
	  song->write = s32tos32l;
	  break;
  case AUDIO_S32MSB:
	  song->write = s32tos32b;
	  break;
  case AUDIO_F32SYS:
	  song->write = s32tof32;
	  break;
  default:
	  SDL_SetError("Unsupported audio format");
	  SDL_free(song);
	  return NULL;
  }

  song->buffer_size = audio->samples;
  song->resample_buffer = safe_malloc(audio->samples * sizeof(sample_t));
  song->common_buffer = safe_malloc(audio->samples * 2 * sizeof(Sint32));
  
  song->control_ratio = audio->freq / CONTROLS_PER_SECOND;
  if (song->control_ratio < 1)
      song->control_ratio = 1;
  else if (song->control_ratio > MAX_CONTROL_RATIO)
      song->control_ratio = MAX_CONTROL_RATIO;

  song->lost_notes = 0;
  song->cut_notes = 0;

  song->events = read_midi_file(song, &(song->groomed_event_count),
      &song->samples);

  /* The RWops can safely be closed at this point, but let's make that the
   * responsibility of the caller.
   */
  
  /* Make sure everything is okay */
  if (!song->events) {
    SDL_free(song);
    return(NULL);
  }

  song->default_instrument = 0;
  song->default_program = DEFAULT_PROGRAM;

  if (*def_instr_name)
    set_default_instrument(song, def_instr_name);

  load_missing_instruments(song);

  return(song);
}

void Timidity_FreeSong(MidiSong *song)
{
  int i;

  free_instruments(song);

  for (i = 0; i < 128; i++)
  {
    if (song->tonebank[i])
      SDL_free(song->tonebank[i]);
    if (song->drumset[i])
      SDL_free(song->drumset[i]);
  }
  
  SDL_free(song->common_buffer);
  SDL_free(song->resample_buffer);
  SDL_free(song->events);
  SDL_free(song);
}

void Timidity_Exit(void)
{
  int i, j;

  for (i = 0; i < 128; i++)
  {
    if (master_tonebank[i])
    {
      ToneBankElement *e = master_tonebank[i]->tone;
      if (e != NULL)
      {
        for (j = 0; j < 128; j++)
        {
          if (e[j].name != NULL)
            SDL_free(e[j].name);
        }
        SDL_free(e);
      }
      SDL_free(master_tonebank[i]);
      master_tonebank[i] = NULL;
    }
    if (master_drumset[i])
    {
      ToneBankElement *e = master_drumset[i]->tone;
      if (e != NULL)
      {
        for (j = 0; j < 128; j++)
        {
          if (e[j].name != NULL)
            SDL_free(e[j].name);
        }
        SDL_free(e);
      }
      SDL_free(master_drumset[i]);
      master_drumset[i] = NULL;
    }
  }

  free_pathlist();
}
