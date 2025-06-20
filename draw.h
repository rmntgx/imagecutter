#ifndef DRAW_H
#define DRAW_H

#include <SDL3/SDL.h>
#include <math.h>
#include "selection.h"

void draw_rotation_icon(SDL_Renderer* renderer, SDL_FRect* rect, Rotation rotation);

#endif /* DRAW_H */