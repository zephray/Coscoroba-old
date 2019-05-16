/* 
 * Project Coscoroba
 * Copyright 2019 Wenting Zhang
 * 
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. 
 */
#include "main.h"
#include "frontend.h"

namespace Frontend {

    SDL_Surface *screen;

    void Init() {
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

    bool PollEvent() {
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

    void Deinit() {
        free(screen);
        SDL_Quit();
    }

} // namespace FrontEnd