#ifndef UTILS_H
#define UTILS_H

#include <SDL3/SDL.h>
#include <math.h>

SDL_FRect get_texture_rect(SDL_Window* window, int tex_width, int tex_height, float* scale);

#endif /* UTILS_H */
