# Migrating to SDL_sound 3.0

SDL_sound 3.0 (aka "SDL3_sound") is almost identical in API to previous
releases. In fact, most of the API is identicalto the original 1.0 release!
To keep migration simple, we didn't do a full modernization pass on the API,
renaming functions, moving true/false "int" return values to "bool", etc.

There were extremely minor changes necessitated by the move to SDL3,
however, and those are listed here.

SDL3_sound requires SDL3. It relies on features that are new to SDL3, both
internally and in the public API, so if your project is on SDL 1.2 or
SDL2, you'll have to move your project to SDL3 at the same time.

## Including SDL3_sound

The proper way to include SDL3_sounds's header is:

```c
#include <SDL3_sound/SDL_sound.h>
```

Like SDL3, the new convention is to use `<>` brackets and a subdirectory.


## Versioning

Versions are now bits packed into a single int instead of a struct, using
SDL3's usual magic for unpacking these. We _did_ change this to match SDL3
conventions.

- SOUND_VER_MAJOR => SDL_SOUND_MAJOR_VERSION
- SOUND_VER_MINOR => SDL_SOUND_MINOR_VERSION
- SOUND_VER_PATCH => SDL_SOUND_MICRO_VERSION
- Sound_Version => Just use `int` instead of this removed struct.
- SOUND_VERSION(&v) => `v = SDL_SOUND_VERSION;`
- Sound_GetLinkedVersion(&v) => `v = Sound_Version();`  (yes, this is the same symbol that used to be a struct.)


## Calling conventions

`SNDDECLSPEC` was removed, using SDL3's standard symbol instead. This is only
important if, for some reason, your app decided to use this to declare its own
functions, which is unlikely. A simple search/replace or `#define` will solve
this problem.

- SNDDECLSPEC => SDL_DECLSPEC


## SDL_RWops vs SDL_IOStream

SDL3 removed the SDL_RWops interface, replacing it with SDL_IOStream. Largely
they are functionally equivalent, and in terms of SDL_sound's public API,
there is only one place where it is directly used: `Sound_NewSample()`.

Simply replace your SDL_RWops with SDL_IOStreams, as you would have to do
moving to SDL3 anyhow, or use one of the other `Sound_NewSample*()` functions,
which will manage SDL_IOStreams internally on your app's behalf.


## Audio format information

Sound_AudioInfo is gone, replaced with the standard SDL_AudioSpec, which is
almost exactly equivalent in SDL3. The individual types are different (ints
instead of Uint32), so don't memcpy between them, but the fields of the two
structs are otherwise functionally equivalent and in the same order.

This affects certain fields in `Sound_Sample`, and the parameters to the
various `Sound_NewSample*()` functions.

A simple search/replace of `Sound_AudioInfo` => `SDL_AudioSpec`, or a
`#define` of the former to the latter, will likely work.


## That's all!

If you've run into a problem not covered here, please let us know through
[the bug tracker](https://github.com/icculus/SDL_sound/issues).

Thanks!

