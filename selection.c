#include "selection.h"

void init_selection_state(SelectionState* state) {
	state->capacity = 10;
	state->selections = malloc(sizeof(Selection) * state->capacity);
	state->count = 0;
	state->is_dragging = false;
	state->is_resizing = false;
	state->resize_corner = -1;
	state->selected_index = -1;
}

void add_selection(SelectionState* state, SDL_FRect texture_rect) {
	if (state->count >= state->capacity) {
		state->capacity *= 2;
		state->selections = realloc(state->selections, sizeof(Selection) * state->capacity);
	}

	state->selections[state->count].texture_rect = texture_rect;
	state->selections[state->count].rotation = ROTATION_0;
	state->selections[state->count].active = true;
	state->count++;
}

void delete_selection(SelectionState* state, int index) {
	for(int i = index; i < state->count - 1; i++) {
		state->selections[i] = state->selections[i + 1];
	}
	state->count--;
	if(state->selected_index == index) state->selected_index = -1;
	if(state->selected_index > index) state->selected_index--;
}

void clear_selections(SelectionState* state) {
	state->count = 0;
	state->is_dragging = false;
	state->selected_index = -1;
}

void free_selection_state(SelectionState* state) {
	free(state->selections);
}

int find_selection_at_point(SelectionState* state, SDL_FPoint mouse) {
    for (int i = state->count - 1; i >= 0; i--) {
        if (!state->selections[i].active) continue;

        const SDL_FRect* rect = &state->selections[i].texture_rect;
        if (mouse.x >= rect->x && mouse.x <= rect->x + rect->w &&
            mouse.y >= rect->y && mouse.y <= rect->y + rect->h) {
            return i;
        }
    }
    return -1;
}

bool find_move_point(SelectionState* state, SDL_FPoint mouse, float scale, int* corner) {
	if(state->selected_index < 0) return false;

	const float dist = (10 / scale);
	const SDL_FRect* rect = &state->selections[state->selected_index].texture_rect;
	*corner = fabsf(mouse.x - rect->x) < dist;
	*corner += (fabsf(mouse.x - (rect->x + rect->w)) < dist) << 1;
	*corner += (fabsf(mouse.y - rect->y) < dist) << 2;
	*corner += (fabsf(mouse.y - (rect->y + rect->h)) < dist) << 3;
	
	return (*corner & 0b0011) && (*corner & 0b1100);
}

void stop_dragging(SelectionState* state, float mouse_x, float mouse_y, SDL_FRect tex_display, float scale) {
	state->is_dragging = false;

	mouse_x = fminf(fmaxf(mouse_x, tex_display.x), tex_display.x + tex_display.w);
	mouse_y = fminf(fmaxf(mouse_y, tex_display.y), tex_display.y + tex_display.h);

	SDL_FRect screen_rect;
	screen_rect.x = fminf(state->drag_start.x, mouse_x);
	screen_rect.y = fminf(state->drag_start.y, mouse_y);
	screen_rect.w = fabsf(mouse_x - state->drag_start.x);
	screen_rect.h = fabsf(mouse_y - state->drag_start.y);

	if (screen_rect.w <= 5 && screen_rect.h <= 5) {
		return;
	}
	SDL_FRect texture_rect;
	texture_rect.x = (screen_rect.x - tex_display.x) / scale;
	texture_rect.y = (screen_rect.y - tex_display.y) / scale;
	texture_rect.w = screen_rect.w / scale;
	texture_rect.h = screen_rect.h / scale;

	add_selection(state, texture_rect);

	printf("Selection %d created at (%.1f, %.1f) size %.1fx%.1f\n", 
			state->count, texture_rect.x, texture_rect.y, 
			texture_rect.w, texture_rect.h);
}

void stop_resizing(SelectionState* state) {
	state->is_resizing = false;
	SDL_FRect* resizable_rect = &state->selections[state->selected_index].texture_rect;
	if(resizable_rect->w <= 0) {
		resizable_rect->x += resizable_rect->w;
		resizable_rect->w = -resizable_rect->w + (resizable_rect->w == 0);
	}
	if(resizable_rect->h <= 0) {
		resizable_rect->y += resizable_rect->h;
		resizable_rect->h = -resizable_rect->h + (resizable_rect->h == 0);
	}
}

void update_resizable(SelectionState* state, SDL_FPoint norm_mouse) {
	SDL_FRect* resizable_rect = &state->selections[state->selected_index].texture_rect;
	if(state->resize_corner == 0b10000) {
		resizable_rect->x = state->before_resize.x + (norm_mouse.x - state->drag_start.x);
		resizable_rect->y = state->before_resize.y + (norm_mouse.y - state->drag_start.y);
		return;
	}
	if(state->resize_corner & 0b0001) {
		resizable_rect->x = norm_mouse.x;
		resizable_rect->w = state->before_resize.w - (norm_mouse.x - state->before_resize.x);
	} else {
		resizable_rect->w = norm_mouse.x - resizable_rect->x;
	}
	if(state->resize_corner & 0b0100) {
		resizable_rect->h = state->before_resize.h - (norm_mouse.y - state->before_resize.y);
		resizable_rect->y = norm_mouse.y;
	} else {
		resizable_rect->h = norm_mouse.y - resizable_rect->y;
	}
}
