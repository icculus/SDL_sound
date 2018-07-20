SDL_sound. An abstract soundfile decoder.

SDL_sound is a library that handles the decoding of several popular sound file
 formats, such as .WAV and .MP3. It is meant to make the programmer's sound
 playback tasks simpler. The programmer gives SDL_sound a filename, or feeds
 it data directly from one of many sources, and then reads the decoded
 waveform data back at her leisure. If resource constraints are a concern,
 SDL_sound can process sound data in programmer-specified blocks. Alternately,
 SDL_sound can decode a whole sound file and hand back a single pointer to the
 whole waveform. SDL_sound can also handle sample rate, audio format, and
 channel conversion on-the-fly and behind-the-scenes, if the programmer
 desires.

Please check the website for the most up-to-date information about SDL_sound:
   https://icculus.org/SDL_sound/

SDL_sound _REQUIRES_ Simple Directmedia Layer (SDL) 2.x to function, and cannot
 be built without it. You can get SDL from https://www.libsdl.org/. SDL_sound
 2.x only works with SDL2, and SDL_sound 1.x only works with SDL 1.x.
 Reports of success or failure are welcome.

 Unless explicitly disabled during initial build configuration, SDL_sound
 always supports these file formats internally:

 - Wave (.WAV) files
 - MPEG-1 layers I-III (.MP3, .MP2, .MP1)
 - Ogg Vorbis (.OGG) files
 - Free Lossless Audio Codec (.FLAC) files
 - Audio Interchange format (.AIFF) files
 - Sun/NeXT Audio (.AU) files
 - Shorten (.SHN) files
 - Creative Labs Voice (.VOC) files
 - Various "module" formats (.MOD, .669, .AMF, .XM, .IT, .S3M, .STM, etc)
 - MIDI (.mid) files
 - ABC (.abc) files
 - Raw PCM data
 - (macOS/iOS only) anything that CoreAudio can decode.

Building/Installing:
  Please read the docs/INSTALL.txt document.

Reporting bugs/commenting:
 There is a mailing list available. Subscription and mailing list archives
 can be found here:

    https://icculus.org/mailman/listinfo/sdlsound

 The mailing list is the best way to get in touch with SDL_sound developers.

--ryan. (icculus@icculus.org)

