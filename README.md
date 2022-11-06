# SDL_sound

SDL_sound; an abstract soundfile decoder.

## What is this?

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
be built without it. You can get SDL from https://www.libsdl.org/

Current versions of SDL_sound do not use any external libraries for decoding
various file formats. All needed decoder source code is included with the
library. Unless explicitly disabled during initial build configuration,
SDL_sound always supports the following file formats internally:

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
- Raw PCM data
- (macOS/iOS only) anything that CoreAudio can decode.

## Building/Installing:

Please read docs/INSTALL.txt.

## Reporting bugs/commenting:

Please visit https://github.com/icculus/SDL_sound/issues for the bug tracker.

