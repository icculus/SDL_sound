/*

    TiMidity -- Experimental MIDI to WAVE converter
    Copyright (C) 1995 Tuukka Toivonen <toivonen@clinet.fi>

	 This program is free software; you can redistribute it and/or modify
	 it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
	 (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	 GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "SDL_sound.h"

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
    int newline = 0;

    while (num_read < size && !newline)
    {
	if (SDL_RWread(rw, &s[num_read], 1, 1) != 1)
	    break;

	/* Unlike fgets(), don't store newline. Under Windows/DOS we'll
	 * probably get an extra blank line for every line that's being
	 * read, but that should be ok.
	 */
	if (s[num_read] == '\n' || s[num_read] == '\r')
	{
	    s[num_read] = '\0';
	    newline = 1;
	}
	
	num_read++;
    }

    s[num_read] = '\0';
    
    return (num_read != 0) ? s : NULL;
}

static int read_config_file(char *name)
{
  SDL_RWops *rw;
  char tmp[1024], *w[MAXWORDS], *cp;
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
    w[words=0]=strtok(tmp, " \t\240");
    if (!w[0] || (*w[0]=='#')) continue;
    while (w[words] && *w[words] != '#' && (words < MAXWORDS))
      w[++words]=strtok(0," \t\240");
    if (!strcmp(w[0], "dir"))
    {
      if (words < 2)
      {
	SNDDBG(("%s: line %d: No directory given\n", name, line));
	return -2;
      }
      for (i=1; i<words; i++)
	add_to_pathlist(w[i]);
    }
    else if (!strcmp(w[0], "source"))
    {
      if (words < 2)
      {
	SNDDBG(("%s: line %d: No file name given\n", name, line));
	return -2;
      }
      for (i=1; i<words; i++)
      {
	rcf_count++;
	read_config_file(w[i]);
	rcf_count--;
      }
    }
    else if (!strcmp(w[0], "default"))
    {
      if (words != 2)
      {
	SNDDBG(("%s: line %d: Must specify exactly one patch name\n",
		name, line));
	return -2;
      }
      strncpy(def_instr_name, w[1], 255);
      def_instr_name[255]='\0';
    }
    else if (!strcmp(w[0], "drumset"))
    {
      if (words < 2)
      {
	SNDDBG(("%s: line %d: No drum set number given\n", name, line));
	return -2;
      }
      i=atoi(w[1]);
      if (i<0 || i>127)
      {
	SNDDBG(("%s: line %d: Drum set must be between 0 and 127\n",
		name, line));
	return -2;
      }
      if (!master_drumset[i])
      {
	master_drumset[i] = safe_malloc(sizeof(ToneBank));
	memset(master_drumset[i], 0, sizeof(ToneBank));
	master_drumset[i]->tone = safe_malloc(128 * sizeof(ToneBankElement));
	memset(master_drumset[i]->tone, 0, 128 * sizeof(ToneBankElement));
      }
      bank=master_drumset[i];
    }
    else if (!strcmp(w[0], "bank"))
    {
      if (words < 2)
      {
	SNDDBG(("%s: line %d: No bank number given\n", name, line));
	return -2;
      }
      i=atoi(w[1]);
      if (i<0 || i>127)
      {
	SNDDBG(("%s: line %d: Tone bank must be between 0 and 127\n",
		name, line));
	return -2;
      }
      if (!master_tonebank[i])
      {
	master_tonebank[i] = safe_malloc(sizeof(ToneBank));
	memset(master_tonebank[i], 0, sizeof(ToneBank));
	master_tonebank[i]->tone = safe_malloc(128 * sizeof(ToneBankElement));
	memset(master_tonebank[i]->tone, 0, 128 * sizeof(ToneBankElement));
      }
      bank=master_tonebank[i];
    }
    else
    {
      if ((words < 2) || (*w[0] < '0' || *w[0] > '9'))
      {
	SNDDBG(("%s: line %d: syntax error\n", name, line));
	return -2;
      }
      i=atoi(w[0]);
      if (i<0 || i>127)
      {
	SNDDBG(("%s: line %d: Program must be between 0 and 127\n",
		name, line));
	return -2;
      }
      if (!bank)
      {
	SNDDBG(("%s: line %d: Must specify tone bank or drum set before assignment\n",
		name, line));
	return -2;
      }
      if (bank->tone[i].name)
	free(bank->tone[i].name);
      strcpy((bank->tone[i].name=safe_malloc(strlen(w[1])+1)),w[1]);
      bank->tone[i].note=bank->tone[i].amp=bank->tone[i].pan=
      bank->tone[i].strip_loop=bank->tone[i].strip_envelope=
      bank->tone[i].strip_tail=-1;

      for (j=2; j<words; j++)
      {
	if (!(cp=strchr(w[j], '=')))
	{
	  SNDDBG(("%s: line %d: bad patch option %s\n", name, line, w[j]));
	  return -2;
	}
	*cp++=0;
	if (!strcmp(w[j], "amp"))
	{
	  k=atoi(cp);
	  if ((k<0 || k>MAX_AMPLIFICATION) || (*cp < '0' || *cp > '9'))
	  {
	    SNDDBG(("%s: line %d: amplification must be between 0 and %d\n",
		    name, line, MAX_AMPLIFICATION));
	    return -2;
	  }
	  bank->tone[i].amp=k;
	}
	else if (!strcmp(w[j], "note"))
	{
	  k=atoi(cp);
	  if ((k<0 || k>127) || (*cp < '0' || *cp > '9'))
	  {
	    SNDDBG(("%s: line %d: note must be between 0 and 127\n",
		    name, line));
	    return -2;
	  }
	  bank->tone[i].note=k;
	}
	else if (!strcmp(w[j], "pan"))
	{
	  if (!strcmp(cp, "center"))
	    k=64;
	  else if (!strcmp(cp, "left"))
	    k=0;
	  else if (!strcmp(cp, "right"))
	    k=127;
	  else
	    k=((atoi(cp)+100) * 100) / 157;
	  if ((k<0 || k>127) || (k==0 && *cp!='-' && (*cp < '0' || *cp > '9')))
	  {
	    SNDDBG(("%s: line %d: panning must be left, right, center, or between -100 and 100\n",
		    name, line));
	    return -2;
	  }
	  bank->tone[i].pan=k;
	}
	else if (!strcmp(w[j], "keep"))
	{
	  if (!strcmp(cp, "env"))
	    bank->tone[i].strip_envelope=0;
	  else if (!strcmp(cp, "loop"))
	    bank->tone[i].strip_loop=0;
	  else
	  {
	    SNDDBG(("%s: line %d: keep must be env or loop\n", name, line));
	    return -2;
	  }
	}
	else if (!strcmp(w[j], "strip"))
	{
	  if (!strcmp(cp, "env"))
	    bank->tone[i].strip_envelope=1;
	  else if (!strcmp(cp, "loop"))
	    bank->tone[i].strip_loop=1;
	  else if (!strcmp(cp, "tail"))
	    bank->tone[i].strip_tail=1;
	  else
	  {
	    SNDDBG(("%s: line %d: strip must be env, loop, or tail\n",
		    name, line));
	    return -2;
	  }
	}
	else
	{
	  SNDDBG(("%s: line %d: bad patch option %s\n", name, line, w[j]));
	  return -2;
	}
      }
    }
  }
  SDL_RWclose(rw);
  return 0;
}

int Timidity_Init()
{
  /* !!! FIXME: This may be ugly, but slightly less so than requiring the
   *            default search path to have only one element. I think.
   *
   *            We only need to include the likely locations for the config
   *            file itself since that file should contain any other directory
   *            that needs to be added to the search path.
   */
#ifdef WIN32
  add_to_pathlist("\\TIMIDITY");
#else
  add_to_pathlist("/usr/local/lib/timidity");
  add_to_pathlist("/etc");
#endif

  /* Allocate memory for the standard tonebank and drumset */
  master_tonebank[0] = safe_malloc(sizeof(ToneBank));
  memset(master_tonebank[0], 0, sizeof(ToneBank));
  master_tonebank[0]->tone = safe_malloc(128 * sizeof(ToneBankElement));
  memset(master_tonebank[0]->tone, 0, 128 * sizeof(ToneBankElement));

  master_drumset[0] = safe_malloc(sizeof(ToneBank));
  memset(master_drumset[0], 0, sizeof(ToneBank));
  master_drumset[0]->tone = safe_malloc(128 * sizeof(ToneBankElement));
  memset(master_drumset[0]->tone, 0, 128 * sizeof(ToneBankElement));

  return read_config_file(CONFIG_FILE);
}

MidiSong *Timidity_LoadSong(SDL_RWops *rw, SDL_AudioSpec *audio)
{
  MidiSong *song;
  Sint32 events;
  int i;

  if (rw == NULL)
      return NULL;
  
  /* Allocate memory for the song */
  song = (MidiSong *)safe_malloc(sizeof(*song));
  memset(song, 0, sizeof(*song));

  for (i = 0; i < 128; i++)
  {
    if (master_tonebank[i])
    {
      song->tonebank[i] = safe_malloc(sizeof(ToneBank));
      song->tonebank[i]->tone = master_tonebank[i]->tone;
    }
    if (master_drumset[i])
    {
      song->drumset[i] = safe_malloc(sizeof(ToneBank));
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
  if (audio->format & 0x8000)
      song->encoding |= PE_SIGNED;
  if (audio->channels == 1)
      song->encoding |= PE_MONO;
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
      default:
	  SNDDBG(("Unsupported audio format"));
	  song->write = s32tou16l;
	  break;
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

  song->events = read_midi_file(song, &events, &song->samples);

  /* The RWops can safely be closed at this point, but let's make that the
   * responsibility of the caller.
   */
  
  /* Make sure everything is okay */
  if (!song->events) {
    free(song);
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
      free(song->tonebank[i]);
    if (song->drumset[i])
      free(song->drumset[i]);
  }
  
  free(song->common_buffer);
  free(song->resample_buffer);
  free(song->events);
  free(song);
}

void Timidity_Exit(void)
{
  int i;

  for (i = 0; i < 128; i++)
  {
    if (master_tonebank[i])
    {
      free(master_tonebank[i]->tone);
      free(master_tonebank[i]);
    }
    if (master_drumset[i])
    {
      free(master_drumset[i]->tone);
      free(master_drumset[i]);
    }
  }

  free_pathlist();
}
