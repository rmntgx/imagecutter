#include "selection.h"

void init_selection_state(SelectionState* state) {
	state->capacity = 10;
	state->selections = malloc(sizeof(Selection) * state->capacity);
	state->count = 0;
	state->is_dragging = false;
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

int find_selection_at_point(SelectionState* state, float x, float y, float tex_display_x, float tex_display_y, float scale) {
	for (int i = state->count - 1; i >= 0; i--) {
		if (!state->selections[i].active) continue;

		SDL_FRect* tex_rect = &state->selections[i].texture_rect;
		SDL_FRect screen_rect;
		screen_rect.x = tex_display_x + tex_rect->x * scale;
		screen_rect.y = tex_display_y + tex_rect->y * scale;
		screen_rect.w = tex_rect->w * scale;
		screen_rect.h = tex_rect->h * scale;

		if (x >= screen_rect.x && x <= screen_rect.x + screen_rect.w &&
			y >= screen_rect.y && y <= screen_rect.y + screen_rect.h) {
			return i;
		}
	}
	return -1;
}
