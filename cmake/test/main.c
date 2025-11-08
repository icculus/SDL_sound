#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3_sound/SDL_sound.h>

int main(int argc, char *argv[]) {
    SDL_SetMainReady();
    if (!SDL_Init(0)) {
        SDL_Log("SDL_Init failed (%s)", SDL_GetError());
        return 1;
    }
    if (Sound_Init() == 0) {
        SDL_Log("Sound_Init failed (%s)", Sound_GetError());
        return 1;
    }
    Sound_Quit();
    SDL_Quit();
    return 0;
}
