#include <SDL3/SDL.h>
#include <SDL3/SDL_keycode.h>
#include <SDL3/SDL_mouse.h>
#include <SDL3/SDL_rect.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_stdinc.h>
#include <SDL3_image/SDL_image.h>
#include <stdio.h>
#include <stdbool.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "rotation.c"
#include "draw.c"

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
	int current_selection;
	int selected_index; // Currently selected selection for rotation
} SelectionState;

typedef struct {
	char** image_paths;
	int count;
	int current_index;
	int total_cropped;
} ImageList;

void init_selection_state(SelectionState* state) {
	state->capacity = 10;
	state->selections = malloc(sizeof(Selection) * state->capacity);
	state->count = 0;
	state->is_dragging = false;
	state->current_selection = -1;
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

void clear_selections(SelectionState* state) {
	state->count = 0;
	state->is_dragging = false;
	state->current_selection = -1;
	state->selected_index = -1;
}

void free_selection_state(SelectionState* state) {
	free(state->selections);
}

void init_image_list(ImageList* list, int argc, char* argv[]) {
	list->count = argc - 1;
	list->current_index = 0;
	list->total_cropped = 0;
	list->image_paths = malloc(sizeof(char*) * list->count);
	
	for (int i = 0; i < list->count; i++) {
		list->image_paths[i] = argv[i + 1];
	}
}

void free_image_list(ImageList* list) {
	free(list->image_paths);
}

char* get_base_filename(const char* path) {
	const char* filename = strrchr(path, '/');
	if (!filename) filename = strrchr(path, '\\');
	if (!filename) filename = path;
	else filename++;
	
	char* base = malloc(strlen(filename) + 1);
	strcpy(base, filename);
	
	char* dot = strrchr(base, '.');
	if (dot) *dot = '\0';
	
	return base;
}

void save_selections(SelectionState* state, SDL_Surface* image_surface, const char* base_name, int* total_cropped) {
	for (int i = 0; i < state->count; i++) {
		if (!state->selections[i].active) continue;
		
		Selection* sel = &state->selections[i];
		SDL_FRect* rect = &sel->texture_rect;
		
		// Create crop rectangle
		SDL_Rect crop_rect = {
			(int)rect->x, (int)rect->y,
			(int)rect->w, (int)rect->h
		};
		
		// Clamp to image bounds
		if (crop_rect.x < 0) crop_rect.x = 0;
		if (crop_rect.y < 0) crop_rect.y = 0;
		if (crop_rect.x + crop_rect.w > image_surface->w) crop_rect.w = image_surface->w - crop_rect.x;
		if (crop_rect.y + crop_rect.h > image_surface->h) crop_rect.h = image_surface->h - crop_rect.y;
		
		if (crop_rect.w <= 0 || crop_rect.h <= 0) continue;
		
		// Create cropped surface
		SDL_Surface* cropped = SDL_CreateSurface(crop_rect.w, crop_rect.h, image_surface->format);
		if (!cropped) continue;
		
		SDL_BlitSurface(image_surface, &crop_rect, cropped, NULL);
		
		// Apply rotation if needed
		SDL_Surface* final_surface = create_rotated_surface(cropped, sel->rotation);

		// Generate filename
		char filename[256];
		snprintf(filename, sizeof(filename), "%s_%d.png", base_name, (*total_cropped) + 1);
		
		// Save as PNG
		if (IMG_SavePNG(final_surface, filename)) {
			printf("Saved: %s\n", filename);
			(*total_cropped)++;
		} else {
			printf("Failed to save %s: %s\n", filename, SDL_GetError());
		}
		
		// Cleanup
		if (final_surface != cropped) {
			SDL_DestroySurface(final_surface);
		}
		SDL_DestroySurface(cropped);
	}
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


int main(int argc, char* argv[]) {
	if (argc < 2) {
		printf("Usage: %s <image1> [image2] [image3] ...\n", argv[0]);
		return 1;
	}

	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);

	SDL_Window* window = SDL_CreateWindow("Image Cropper", 800, 600, SDL_WINDOW_RESIZABLE);
	if (!window) {
		fprintf(stderr, "SDL_CreateWindow Error: %s\n", SDL_GetError());
		SDL_Quit();
		return 1;
	}

	SDL_Renderer* renderer = SDL_CreateRenderer(window, NULL);
	if (!renderer) {
		fprintf(stderr, "SDL_CreateRenderer Error: %s\n", SDL_GetError());
		SDL_DestroyWindow(window);
		SDL_Quit();
		return 1;
	}

	// Enable alpha blending for transparency
	SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

	ImageList image_list;
	init_image_list(&image_list, argc, argv);

	SDL_Texture* texture = NULL;
	SDL_Surface* image_surface = NULL;
	int tex_width = 0, tex_height = 0;
	char* current_base_name = NULL;

	SDL_Cursor* loading_cursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_WAIT);
	SDL_Cursor* default_cursor =  SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_DEFAULT);
	SDL_SetCursor(loading_cursor);

	// Clear screen
	SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
	SDL_RenderClear(renderer);
	SDL_RenderPresent(renderer);

	// Load first image
	if (image_list.count > 0) {
		image_surface = IMG_Load(image_list.image_paths[0]);
		if (image_surface) {
			texture = SDL_CreateTextureFromSurface(renderer, image_surface);
			if (texture) {
				tex_width = image_surface->w;
				tex_height = image_surface->h;
				current_base_name = get_base_filename(image_list.image_paths[0]);
			}
		}
	}

	SDL_SetCursor(default_cursor);

	if (!texture) {
		fprintf(stderr, "Failed to load first image\n");
		free_image_list(&image_list);
		SDL_DestroyRenderer(renderer);
		SDL_DestroyWindow(window);
		SDL_Quit();
		return 1;
	}

	SelectionState selection_state;
	init_selection_state(&selection_state);

	bool running = true;
	printf("Controls:\n\
- Left click and drag to create selection\n\
- Right click on selection to select it for rotation\n\
- 'Q'/'E' keys to rotate selected area left/right\n\
- 'S' key to save all selections\n\
- 'C' key to clear all selections\n\
- 'N' key for next image\n\
- 'P' key for previous image\n\
- 'ESC' to quit\n\n");

	const int targetFPS = 60;
	const Uint32 frameDelay = 1000 / targetFPS;
	bool redraw = true;

	while (running) {
		Uint32 frameStart = SDL_GetTicks(); // Framerate limit
		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			switch (event.type) {
				case SDL_EVENT_QUIT:
					running = false;
					break;

				case SDL_EVENT_KEY_DOWN:
					switch (event.key.key) {
						case SDLK_ESCAPE:
							running = false;
						break;
						case SDLK_C:
							clear_selections(&selection_state);
							printf("All selections cleared.\n");
							redraw = true;
						break;
						case SDLK_S:
							if (current_base_name && image_surface) {
								save_selections(&selection_state, image_surface, current_base_name, &image_list.total_cropped);
								clear_selections(&selection_state);
								redraw = true;
							}
						break;
						case SDLK_E:
						case SDLK_Q:
							if(selection_state.selected_index >= 0) {
								Selection* sel = &selection_state.selections[selection_state.selected_index];
								sel->rotation = (sel->rotation + 1 + (2 * (event.key.key == SDLK_Q))) % 4; // Rotate left
								redraw = true;
							}
						break;
						case SDLK_N:
						case SDLK_P:
							bool changed = false;
							if(event.key.key == SDLK_N && image_list.current_index < image_list.count - 1) {
								image_list.current_index++;
								changed = true;
							} else if (event.key.key == SDLK_P && image_list.current_index > 0) {
								image_list.current_index--;
								changed = true;
							}
							if(changed) {
								clear_selections(&selection_state);
								
								if (texture) SDL_DestroyTexture(texture);
								if (image_surface) SDL_DestroySurface(image_surface);
								if (current_base_name) free(current_base_name);
								
								SDL_SetCursor(loading_cursor);
								image_surface = IMG_Load(image_list.image_paths[image_list.current_index]);
								if (image_surface) {
									texture = SDL_CreateTextureFromSurface(renderer, image_surface);
									tex_width = image_surface->w;
									tex_height = image_surface->h;
									current_base_name = get_base_filename(image_list.image_paths[image_list.current_index]);
									printf("Loaded: %s\n", image_list.image_paths[image_list.current_index]);
								}
								SDL_SetCursor(default_cursor);
							}
							redraw = true;
						break;
					}
					break;

				case SDL_EVENT_WINDOW_RESIZED:
					redraw = true;
					break;

				case SDL_EVENT_MOUSE_BUTTON_DOWN:
					if (event.button.button == SDL_BUTTON_LEFT) {
						// Get window size
						float scale;
						SDL_FRect tex_display = get_texture_rect(window, tex_width, tex_height, &scale);

						float mouse_x = event.button.x;
						float mouse_y = event.button.y;
						
						if (mouse_x >= tex_display.x && mouse_x <= tex_display.x + tex_display.w &&
							mouse_y >= tex_display.y && mouse_y <= tex_display.y + tex_display.h) {
							selection_state.is_dragging = true;
							selection_state.drag_start.x = mouse_x;
							selection_state.drag_start.y = mouse_y;
							selection_state.current_selection = -1;
							selection_state.selected_index = -1;
						}
						redraw = true;
					} else if (event.button.button == SDL_BUTTON_RIGHT) {
						// Right click to select a selection for rotation
						float scale;
						SDL_FRect tex_display = get_texture_rect(window, tex_width, tex_height, &scale);
						float mouse_x = event.button.x;
						float mouse_y = event.button.y;
						
						int selected = find_selection_at_point(&selection_state, mouse_x, mouse_y, 
															 tex_display.x, tex_display.y, scale);
						selection_state.selected_index = selected;
						if (selected >= 0) {
							printf("Selected area %d for rotation (use Q/E keys)\n", selected + 1);
						}
						redraw = true;
					}
					break;

				case SDL_EVENT_MOUSE_BUTTON_UP:
					if (event.button.button == SDL_BUTTON_LEFT && selection_state.is_dragging) {
						selection_state.is_dragging = false;
						
						float scale;
						SDL_FRect tex_display = get_texture_rect(window, tex_width, tex_height, &scale);
						
						float mouse_x = event.button.x;
						float mouse_y = event.button.y;
						
						SDL_FRect screen_rect;
						screen_rect.x = fminf(selection_state.drag_start.x, mouse_x);
						screen_rect.y = fminf(selection_state.drag_start.y, mouse_y);
						screen_rect.w = fabsf(mouse_x - selection_state.drag_start.x);
						screen_rect.h = fabsf(mouse_y - selection_state.drag_start.y);
						
						screen_rect.x = fmaxf(screen_rect.x, tex_display.x);
						screen_rect.y = fmaxf(screen_rect.y, tex_display.y);
						screen_rect.w = fminf(screen_rect.w, tex_display.x + tex_display.w - screen_rect.x);
						screen_rect.h = fminf(screen_rect.h, tex_display.y + tex_display.h - screen_rect.y);
						
						if (screen_rect.w > 5 && screen_rect.h > 5) {
							SDL_FRect texture_rect;
							texture_rect.x = (screen_rect.x - tex_display.x) / scale;
							texture_rect.y = (screen_rect.y - tex_display.y) / scale;
							texture_rect.w = screen_rect.w / scale;
							texture_rect.h = screen_rect.h / scale;
							
							add_selection(&selection_state, texture_rect);
							
							printf("Selection %d created at (%.1f, %.1f) size %.1fx%.1f\n", 
								   selection_state.count, texture_rect.x, texture_rect.y, 
								   texture_rect.w, texture_rect.h);
						}
						
						selection_state.current_selection = -1;
						redraw = true;
					}
					break;

				case SDL_EVENT_MOUSE_MOTION:
					break;
			}
		}
		if(redraw || selection_state.is_dragging) {
			// Clear screen
			SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
			SDL_RenderClear(renderer);

			if (texture) {
				// Draw texture
				float scale;
				SDL_FRect tex_dst = get_texture_rect(window, tex_width, tex_height, &scale);
				SDL_RenderTexture(renderer, texture, NULL, &tex_dst);

				// Draw all completed selections
				for (int i = 0; i < selection_state.count; i++) {
					if (selection_state.selections[i].active) {
						SDL_FRect* tex_rect = &selection_state.selections[i].texture_rect;
						SDL_FRect screen_rect;
						screen_rect.x = tex_dst.x + tex_rect->x * scale;
						screen_rect.y = tex_dst.y + tex_rect->y * scale;
						screen_rect.w = tex_rect->w * scale;
						screen_rect.h = tex_rect->h * scale;
						
						// Highlight selected selection
						if (i == selection_state.selected_index) {
							SDL_SetRenderDrawColor(renderer, 0, 255, 0, 60);
							SDL_RenderFillRect(renderer, &screen_rect);
							SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
							// Draw thicker border for selected
							for (int j = 0; j < 3; j++) {
								SDL_FRect thick_rect = {screen_rect.x - j, screen_rect.y - j, 
													  screen_rect.w + 2*j, screen_rect.h + 2*j};
								SDL_RenderRect(renderer, &thick_rect);
							}
						} else {
							SDL_SetRenderDrawColor(renderer, 0, 255, 0, 30);
							SDL_RenderFillRect(renderer, &screen_rect);
							SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
							SDL_RenderRect(renderer, &screen_rect);
						}
						
						// Draw rotation icon
						draw_rotation_icon(renderer, &screen_rect, selection_state.selections[i].rotation);
					}
				}

				// Draw current selection being dragged
				if (selection_state.is_dragging) {
					float mouse_x, mouse_y;
					SDL_GetMouseState(&mouse_x, &mouse_y);
					
					SDL_FRect current_rect;
					current_rect.x = fminf(selection_state.drag_start.x, mouse_x);
					current_rect.y = fminf(selection_state.drag_start.y, mouse_y);
					current_rect.w = fabsf(mouse_x - selection_state.drag_start.x);
					current_rect.h = fabsf(mouse_y - selection_state.drag_start.y);
					
					current_rect.x = fmaxf(current_rect.x, tex_dst.x);
					current_rect.y = fmaxf(current_rect.y, tex_dst.y);
					current_rect.w = fminf(current_rect.w, tex_dst.x + tex_dst.w - current_rect.x);
					current_rect.h = fminf(current_rect.h, tex_dst.y + tex_dst.h - current_rect.y);
					
					SDL_SetRenderDrawColor(renderer, 0, 255, 0, 30);
					SDL_RenderFillRect(renderer, &current_rect);
					SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
					SDL_RenderRect(renderer, &current_rect);
				}
			}

			SDL_RenderPresent(renderer);
		}

		Uint32 frameTime = SDL_GetTicks() - frameStart; // Framerate limit
		if (frameTime < frameDelay) {
			SDL_Delay(frameDelay - frameTime);
		}
		redraw = false;
	}

	// Cleanup
	SDL_DestroyCursor(loading_cursor);
	SDL_DestroyCursor(default_cursor);
	free_selection_state(&selection_state);
	if (texture) SDL_DestroyTexture(texture);
	if (image_surface) SDL_DestroySurface(image_surface);
	if (current_base_name) free(current_base_name);
	free_image_list(&image_list);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}
