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


SDL_FPoint get_normalized_mouse(SDL_Window* window, int tex_width, int tex_height, float mouse_x, float mouse_y, float* scale) {
	float scale2;
	SDL_FRect tex_display = get_texture_rect(window, tex_width, tex_height, &scale2);
	if(scale != NULL) *scale = scale2;

	mouse_x = fminf(fmaxf(mouse_x, tex_display.x), tex_display.x + tex_display.w);
	mouse_y = fminf(fmaxf(mouse_y, tex_display.y), tex_display.y + tex_display.h);

	SDL_FPoint norm_mouse;
	norm_mouse.x = (mouse_x - tex_display.x) / scale2;
	norm_mouse.y = (mouse_y - tex_display.y) / scale2;
	return norm_mouse;
}

void change_resizing_cursor(SelectionState* state, bool cursor_on_move_point, int corner, SDL_Cursor* resize_cursor, SDL_Cursor* default_cursor) {
	if(cursor_on_move_point) {
		if(state->resize_corner < 0) {
			if(resize_cursor != NULL) {
				SDL_DestroyCursor(resize_cursor);
				resize_cursor = NULL;
			}
			switch (corner) {
				case 0b1010:
					resize_cursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SE_RESIZE);
					break;
				case 0b1001:
					resize_cursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SW_RESIZE);
					break;
				case 0b0110:
					resize_cursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_NE_RESIZE);
					break;
				case 0b0101:
				default:
					resize_cursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_NW_RESIZE);
					break;
			}
			SDL_SetCursor(resize_cursor);
			state->resize_corner = corner;
		}
	} else {
		if(state->resize_corner >= 0) {
			SDL_SetCursor(default_cursor);
			SDL_DestroyCursor(resize_cursor);
			resize_cursor = NULL;
			state->resize_corner = -1;
		}
	}
}
