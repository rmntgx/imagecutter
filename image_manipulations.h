#ifndef IMAGE_MANIPULATIONS_H
#define IMAGE_MANIPULATIONS_H

#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <stdio.h>

#include "selection.h"

SDL_Surface* create_rotated_surface(SDL_Surface* original, Rotation rotation);
void save_selections(SelectionState* state, SDL_Surface* image_surface, const char* base_name, int* total_cropped);

#endif /* IMAGE_MANIPULATIONS_H */