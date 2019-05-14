#include "main.h"
#include "frontend.h"

SDL_Surface *screen;

void frontend_init() {
    // Initialize the window
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        //LOG_CRITICAL(Frontend, "Failed to initialize SDL1! Exiting...");
        exit(1);
    }

    SDL_WM_SetCaption("Coscoroba Emu", NULL);

    screen = SDL_SetVideoMode(VIDEO_WIDTH, VIDEO_HEIGHT, 32, SDL_SWSURFACE);

    if (screen == NULL) {
        //LOG_CRITICAL(Frontend, "Failed to create window! Exiting...");
        exit(1);
    }
}