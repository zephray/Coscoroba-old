#pragma once

#include <SDL/SDL.h>

#define VIDEO_WIDTH  400
#define VIDEO_HEIGHT 240

void frontend_init();
bool frontend_pollevent();
void frontend_deinit();

extern SDL_Surface *screen;