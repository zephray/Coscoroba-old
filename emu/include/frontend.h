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
#pragma once

#include "main.h"
#include <SDL/SDL.h>

// TODO: move these out here
#ifdef DEBUG_BUILD
constexpr unsigned VIDEO_WIDTH = 800;
constexpr unsigned VIDEO_HEIGHT = 480;
#else
constexpr unsigned VIDEO_WIDTH = 400;
constexpr unsigned VIDEO_HEIGHT = 240;
#endif

class Frontend {
public:
    Frontend();
    ~Frontend();
    
    bool PollEvent();
    void Clear();
    void DrawPixel(int x, int y, int r, int g, int b);
    void Flip();
    void Wait();

private:
    SDL_Surface *screen;
    uint32_t last_tick;
};
