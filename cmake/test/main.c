#define SDL_MAIN_HANDLED
#include "SDL.h"
#include "SDL_sound.h"
#include <stdio.h>

int main(int argc, char *argv[]) {
    SDL_SetMainReady();
    if (SDL_Init(0) < 0) {
        fprintf(stderr, "SDL_Init: could not initialize SDL: %s\n", SDL_GetError());
        return 1;
    }
    if (Sound_Init() == 0) {
        fprintf(stderr, "Sound_Init: failed (%s)\n", Sound_GetError());
    }
    Sound_Quit();
    SDL_Quit();
    return 0;
}
