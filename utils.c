#include "utils.h"

SDL_FRect get_texture_rect(SDL_Window* window, int tex_width, int tex_height, float* scale) {
	int win_width, win_height;
	SDL_GetWindowSize(window, &win_width, &win_height);

	*scale = fminf((float)win_width / tex_width, (float)win_height / tex_height);
	SDL_FRect rect;
	rect.w = tex_width * *scale;
	rect.h = tex_height * *scale;
	rect.x = (win_width - rect.w) / 2;
	rect.y = (win_height - rect.h) / 2;
	return rect;
}
