/*
 *  Project Coscoroba
 *
 *  Copyright (C) 2019  Wenting Zhang <zephray@outlook.com>
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms and conditions of the GNU General Public License,
 *  version 2, as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope it will be useful, but WITHOUT
 *  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 *  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 *  more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
 */
#include "main.h"
#include "frontend.h"

Frontend::Frontend() {
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

Frontend::~Frontend() {
    free(screen);
    SDL_Quit();
}

bool Frontend::PollEvent() {
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

void Frontend::DrawPixel(int x, int y, int r, int g, int b) {
    uint32_t *buffer = (uint32_t *)screen->pixels;
    // Gamma correction
    /*float rr = pow((float)r / 255.0, 1/2.2);
    float gg = pow((float)g / 255.0, 1/2.2);
    float bb = pow((float)b / 255.0, 1/2.2);
    r = rr * 255;
    g = gg * 255;
    b = bb * 255;*/
    uint32_t color = SDL_MapRGB(screen->format, r, g, b);
    buffer[y * VIDEO_WIDTH + x] = color; 
}

void Frontend::Flip() {
    SDL_Flip(screen);
}

