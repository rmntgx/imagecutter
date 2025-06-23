#ifndef SELECTION_H
#define SELECTION_H

#include <SDL3/SDL.h>
#include <stdbool.h>
#include <stdlib.h>
#include <math.h>
#include <stdio.h>

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
	bool is_resizing;
	SDL_FPoint drag_start;
	SDL_FRect before_resize;
	int selected_index; // Currently selected selection for rotation
	int resize_corner;
} SelectionState;

void init_selection_state(SelectionState* state);
void add_selection(SelectionState* state, SDL_FRect texture_rect);
void delete_selection(SelectionState* state, int index);
void clear_selections(SelectionState* state);
void free_selection_state(SelectionState* state);
int find_selection_at_point(SelectionState* state, SDL_FPoint mouse);
bool find_move_point(SelectionState* state, SDL_FPoint mouse, float scale, int* corner);
void stop_dragging(SelectionState* state, float mouse_x, float mouse_y, SDL_FRect tex_display, float scale);
void stop_resizing(SelectionState* state);
void update_resizable(SelectionState* state, SDL_FPoint norm_mouse);

#endif /* SELECTION_H */
