/* 
 * Project Coscoroba
 * Copyright 2019 Wenting Zhang
 * 
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. 
 */
#pragma once

#include <SDL/SDL.h>

// TODO: move these out here
constexpr unsigned VIDEO_WIDTH = 400;
constexpr unsigned VIDEO_HEIGHT = 240;

namespace Frontend {
    void Init();
    bool PollEvent();
    void Deinit();

    extern SDL_Surface *screen;
}
