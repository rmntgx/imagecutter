#ifndef UTILS_H
#define UTILS_H

#include <SDL3/SDL.h>
#include <math.h>
#include "selection.h"

SDL_FRect get_texture_rect(SDL_Window* window, int tex_width, int tex_height, float* scale);
SDL_FPoint get_normalized_mouse(SDL_Window* window, int tex_width, int tex_height, float mouse_x, float mouse_y, float* scale);
void change_resizing_cursor(SelectionState* state, bool cursor_on_move_point, int corner, SDL_Cursor* resize_cursor, SDL_Cursor* default_cursor);

#endif /* UTILS_H */
