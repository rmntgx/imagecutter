#ifndef SELECTION_H
#define SELECTION_H

#include <SDL3/SDL.h>
#include <stdbool.h>
#include <stdlib.h>

typedef enum {
	ROTATION_0 = 0,
	ROTATION_90 = 1,
	ROTATION_180 = 2,
	ROTATION_270 = 3
} Rotation;

typedef struct {
	SDL_FRect texture_rect;  // Coordinates in texture space
	Rotation rotation;
	bool active;
} Selection;

typedef struct {
	Selection* selections;
	int count;
	int capacity;
	bool is_dragging;
	SDL_FPoint drag_start;
	int selected_index; // Currently selected selection for rotation
} SelectionState;

void init_selection_state(SelectionState* state);
void add_selection(SelectionState* state, SDL_FRect texture_rect);
void delete_selection(SelectionState* state, int index);
void clear_selections(SelectionState* state);
void free_selection_state(SelectionState* state);
int find_selection_at_point(SelectionState* state, float x, float y, float tex_display_x, float tex_display_y, float scale);

#endif /* SELECTION_H */
