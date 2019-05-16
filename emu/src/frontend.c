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

bool frontend_pollevent() {
    SDL_Event event;
    bool result = true;
    if (SDL_PollEvent(&event)) {
        switch (event.type) {
        case SDL_QUIT:
            result = false;
            break;
        default:
            break;
        }
    }
    return result;
}

void frontend_deinit() {
    free(screen);
    SDL_Quit();
}
