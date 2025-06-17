#include <SDL3/SDL.h>
#include <SDL3/SDL_render.h>
#include <SDL3_image/SDL_image.h>
#include <stdio.h>
#include <stdbool.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

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


SDL_Surface* create_rotated_surface(SDL_Surface* original, Rotation rotation) {
    if (rotation == ROTATION_0) {
        return original;
    }

    int w = original->w, h = original->h;
    int new_w = (rotation == ROTATION_90 || rotation == ROTATION_270) ? h : w;
    int new_h = (rotation == ROTATION_90 || rotation == ROTATION_270) ? w : h;

    SDL_Surface* rotated = SDL_CreateSurface(new_w, new_h, original->format);
    if (!rotated) {
        return NULL;
    }

    SDL_LockSurface(original);
    SDL_LockSurface(rotated);

    int bpp       = SDL_BYTESPERPIXEL(original->format);
    int src_pitch = original->pitch;
    int dst_pitch = rotated->pitch;
    Uint8 *src    = (Uint8*)original->pixels;
    Uint8 *dst    = (Uint8*)rotated->pixels;

    // Проходим по dst-пикселям подряд
    for (int ny = 0; ny < new_h; ++ny) {
        Uint8 *dst_row = dst + ny * dst_pitch;
        for (int nx = 0; nx < new_w; ++nx) {
            int sx, sy;
            switch (rotation) {
                case ROTATION_90:
                    // dst(nx,ny) = src(w-1-ny, nx)
                    sx = ny;
                    sy = h - 1 - nx;
                    break;
                case ROTATION_180:
                    // dst(nx,ny) = src(w-1-nx, h-1-ny)
                    sx = w - 1 - nx;
                    sy = h - 1 - ny;
                    break;
                case ROTATION_270:
                    // dst(nx,ny) = src(h-1-ny, ?)
                    sx = w - 1 - ny;
                    sy = nx;
                    break;
                default:
                    sx = nx; sy = ny;
            }

            // src-пиксель
            Uint8 *src_px = src + sy * src_pitch + sx * bpp;
            // dst-пиксель (запись подряд!)
            Uint8 *dst_px = dst_row + nx * bpp;
            memcpy(dst_px, src_px, bpp);
        }
    }

    SDL_UnlockSurface(rotated);
    SDL_UnlockSurface(original);
    return rotated;
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

void draw_rotation_icon(SDL_Renderer* renderer, SDL_FRect* rect, Rotation rotation) {
    float center_x = rect->x + rect->w / 2;
    float center_y = rect->y + rect->h / 2;
    float size = fminf(rect->w, rect->h) * 0.1f;
    if (size < 10) size = 10;
    if (size > 30) size = 30;
    
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 200);
    
    // Draw rotation indicator based on rotation
    float points[8];
    switch (rotation) {
        case ROTATION_0:
            // Arrow pointing up
            points[0] = center_x; points[1] = center_y - size;
            points[2] = center_x - size/2; points[3] = center_y;
            points[4] = center_x + size/2; points[5] = center_y;
            points[6] = center_x; points[7] = center_y - size;
            break;
        case ROTATION_90:
            // Arrow pointing right
            points[0] = center_x + size; points[1] = center_y;
            points[2] = center_x; points[3] = center_y - size/2;
            points[4] = center_x; points[5] = center_y + size/2;
            points[6] = center_x + size; points[7] = center_y;
            break;
        case ROTATION_180:
            // Arrow pointing down
            points[0] = center_x; points[1] = center_y + size;
            points[2] = center_x - size/2; points[3] = center_y;
            points[4] = center_x + size/2; points[5] = center_y;
            points[6] = center_x; points[7] = center_y + size;
            break;
        case ROTATION_270:
            // Arrow pointing left
            points[0] = center_x - size; points[1] = center_y;
            points[2] = center_x; points[3] = center_y - size/2;
            points[4] = center_x; points[5] = center_y + size/2;
            points[6] = center_x - size; points[7] = center_y;
            break;
    }
    
    // Draw arrow lines
    SDL_RenderLine(renderer, points[0], points[1], points[2], points[3]);
    SDL_RenderLine(renderer, points[2], points[3], points[4], points[5]);
    SDL_RenderLine(renderer, points[4], points[5], points[6], points[7]);
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

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Usage: %s <image1> [image2] [image3] ...\n", argv[0]);
        return 1;
    }

    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);

    SDL_Window* window = SDL_CreateWindow("Multi-Selection Image Cropper", 800, 600, SDL_WINDOW_RESIZABLE);
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
    printf("Controls:\n");
    printf("- Left click and drag to create selection\n");
    printf("- Right click on selection to select it for rotation\n");
    printf("- 'Q'/'E' keys to rotate selected area left/right\n");
    printf("- 'S' key to save all selections\n");
    printf("- 'C' key to clear all selections\n");
    printf("- 'N' key for next image\n");
    printf("- 'P' key for previous image\n");
    printf("- 'ESC' to quit\n\n");

    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_EVENT_QUIT:
                    running = false;
                    break;

                case SDL_EVENT_KEY_DOWN:
                    if (event.key.key == SDLK_ESCAPE) {
                        running = false;
                    } else if (event.key.key == SDLK_C) {
                        clear_selections(&selection_state);
                        printf("All selections cleared.\n");
                    } else if (event.key.key == SDLK_S) {
                        if (current_base_name && image_surface) {
                            save_selections(&selection_state, image_surface, current_base_name, &image_list.total_cropped);
                            clear_selections(&selection_state);
                        }
                    } else if (event.key.key == SDLK_Q && selection_state.selected_index >= 0) {
                        Selection* sel = &selection_state.selections[selection_state.selected_index];
                        sel->rotation = (sel->rotation + 3) % 4; // Rotate left
                    } else if (event.key.key == SDLK_E && selection_state.selected_index >= 0) {
                        Selection* sel = &selection_state.selections[selection_state.selected_index];
                        sel->rotation = (sel->rotation + 1) % 4; // Rotate right
                    } else if (event.key.key == SDLK_N) {
                        // Next image
                        if (image_list.current_index < image_list.count - 1) {
                            image_list.current_index++;
                            clear_selections(&selection_state);
                            
                            if (texture) SDL_DestroyTexture(texture);
                            if (image_surface) SDL_DestroySurface(image_surface);
                            if (current_base_name) free(current_base_name);
                            
                            image_surface = IMG_Load(image_list.image_paths[image_list.current_index]);
                            if (image_surface) {
                                texture = SDL_CreateTextureFromSurface(renderer, image_surface);
                                tex_width = image_surface->w;
                                tex_height = image_surface->h;
                                current_base_name = get_base_filename(image_list.image_paths[image_list.current_index]);
                                printf("Loaded: %s\n", image_list.image_paths[image_list.current_index]);
                            }
                        }
                    } else if (event.key.key == SDLK_P) {
                        // Previous image
                        if (image_list.current_index > 0) {
                            image_list.current_index--;
                            clear_selections(&selection_state);
                            
                            if (texture) SDL_DestroyTexture(texture);
                            if (image_surface) SDL_DestroySurface(image_surface);
                            if (current_base_name) free(current_base_name);
                            
                            image_surface = IMG_Load(image_list.image_paths[image_list.current_index]);
                            if (image_surface) {
                                texture = SDL_CreateTextureFromSurface(renderer, image_surface);
                                tex_width = image_surface->w;
                                tex_height = image_surface->h;
                                current_base_name = get_base_filename(image_list.image_paths[image_list.current_index]);
                                printf("Loaded: %s\n", image_list.image_paths[image_list.current_index]);
                            }
                        }
                    }
                    break;

                case SDL_EVENT_WINDOW_RESIZED:
                    break;

                case SDL_EVENT_MOUSE_BUTTON_DOWN:
                    if (event.button.button == SDL_BUTTON_LEFT) {
                        // Get window size
                        int win_width, win_height;
                        SDL_GetWindowSize(window, &win_width, &win_height);
                        
                        // Calculate texture display dimensions
                        float scale = fminf((float)win_width / tex_width, (float)win_height / tex_height);
                        float tex_display_width = tex_width * scale;
                        float tex_display_height = tex_height * scale;
                        float tex_display_x = (win_width - tex_display_width) / 2;
                        float tex_display_y = (win_height - tex_display_height) / 2;

                        float mouse_x = event.button.x;
                        float mouse_y = event.button.y;
                        
                        if (mouse_x >= tex_display_x && mouse_x <= tex_display_x + tex_display_width &&
                            mouse_y >= tex_display_y && mouse_y <= tex_display_y + tex_display_height) {
                            selection_state.is_dragging = true;
                            selection_state.drag_start.x = mouse_x;
                            selection_state.drag_start.y = mouse_y;
                            selection_state.current_selection = -1;
                            selection_state.selected_index = -1;
                        }
                    } else if (event.button.button == SDL_BUTTON_RIGHT) {
                        // Right click to select a selection for rotation
                        int win_width, win_height;
                        SDL_GetWindowSize(window, &win_width, &win_height);
                        
                        float scale = fminf((float)win_width / tex_width, (float)win_height / tex_height);
                        float tex_display_width = tex_width * scale;
                        float tex_display_height = tex_height * scale;
                        float tex_display_x = (win_width - tex_display_width) / 2;
                        float tex_display_y = (win_height - tex_display_height) / 2;

                        float mouse_x = event.button.x;
                        float mouse_y = event.button.y;
                        
                        int selected = find_selection_at_point(&selection_state, mouse_x, mouse_y, 
                                                             tex_display_x, tex_display_y, scale);
                        selection_state.selected_index = selected;
                        if (selected >= 0) {
                            printf("Selected area %d for rotation (use Q/E keys)\n", selected + 1);
                        }
                    }
                    break;

                case SDL_EVENT_MOUSE_BUTTON_UP:
                    if (event.button.button == SDL_BUTTON_LEFT && selection_state.is_dragging) {
                        selection_state.is_dragging = false;
                        
                        int win_width, win_height;
                        SDL_GetWindowSize(window, &win_width, &win_height);
                        
                        float scale = fminf((float)win_width / tex_width, (float)win_height / tex_height);
                        float tex_display_width = tex_width * scale;
                        float tex_display_height = tex_height * scale;
                        float tex_display_x = (win_width - tex_display_width) / 2;
                        float tex_display_y = (win_height - tex_display_height) / 2;
                        
                        float mouse_x = event.button.x;
                        float mouse_y = event.button.y;
                        
                        SDL_FRect screen_rect;
                        screen_rect.x = fminf(selection_state.drag_start.x, mouse_x);
                        screen_rect.y = fminf(selection_state.drag_start.y, mouse_y);
                        screen_rect.w = fabsf(mouse_x - selection_state.drag_start.x);
                        screen_rect.h = fabsf(mouse_y - selection_state.drag_start.y);
                        
                        screen_rect.x = fmaxf(screen_rect.x, tex_display_x);
                        screen_rect.y = fmaxf(screen_rect.y, tex_display_y);
                        screen_rect.w = fminf(screen_rect.w, tex_display_x + tex_display_width - screen_rect.x);
                        screen_rect.h = fminf(screen_rect.h, tex_display_y + tex_display_height - screen_rect.y);
                        
                        if (screen_rect.w > 5 && screen_rect.h > 5) {
                            SDL_FRect texture_rect;
                            texture_rect.x = (screen_rect.x - tex_display_x) / scale;
                            texture_rect.y = (screen_rect.y - tex_display_y) / scale;
                            texture_rect.w = screen_rect.w / scale;
                            texture_rect.h = screen_rect.h / scale;
                            
                            add_selection(&selection_state, texture_rect);
                            
                            printf("Selection %d created at (%.1f, %.1f) size %.1fx%.1f\n", 
                                   selection_state.count, texture_rect.x, texture_rect.y, 
                                   texture_rect.w, texture_rect.h);
                        }
                        
                        selection_state.current_selection = -1;
                    }
                    break;

                case SDL_EVENT_MOUSE_MOTION:
                    break;
            }
        }

        // Clear screen
        SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
        SDL_RenderClear(renderer);

        if (texture) {
            // Get window size
            int win_width, win_height;
            SDL_GetWindowSize(window, &win_width, &win_height);
            
            // Calculate texture display dimensions
            float scale = fminf((float)win_width / tex_width, (float)win_height / tex_height);
            float tex_display_width = tex_width * scale;
            float tex_display_height = tex_height * scale;
            float tex_display_x = (win_width - tex_display_width) / 2;
            float tex_display_y = (win_height - tex_display_height) / 2;

            // Draw texture
            SDL_FRect tex_dst = { tex_display_x, tex_display_y, tex_display_width, tex_display_height };
            SDL_RenderTexture(renderer, texture, NULL, &tex_dst);

            // Draw all completed selections
            for (int i = 0; i < selection_state.count; i++) {
                if (selection_state.selections[i].active) {
                    SDL_FRect* tex_rect = &selection_state.selections[i].texture_rect;
                    SDL_FRect screen_rect;
                    screen_rect.x = tex_display_x + tex_rect->x * scale;
                    screen_rect.y = tex_display_y + tex_rect->y * scale;
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
                
                current_rect.x = fmaxf(current_rect.x, tex_display_x);
                current_rect.y = fmaxf(current_rect.y, tex_display_y);
                current_rect.w = fminf(current_rect.w, tex_display_x + tex_display_width - current_rect.x);
                current_rect.h = fminf(current_rect.h, tex_display_y + tex_display_height - current_rect.y);
                
                SDL_SetRenderDrawColor(renderer, 0, 255, 0, 30);
                SDL_RenderFillRect(renderer, &current_rect);
                SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
                SDL_RenderRect(renderer, &current_rect);
            }
        }

        // Draw counter in top-left corner
        char counter_text[64];
        snprintf(counter_text, sizeof(counter_text), "Cropped: %d | Image: %d/%d", 
                 image_list.total_cropped, image_list.current_index + 1, image_list.count);
        
        // Simple text rendering using rectangles (since we don't have a font system)
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 150);
        SDL_FRect bg_rect = {10, 10, (float)strlen(counter_text) * 8, 20};
        SDL_RenderFillRect(renderer, &bg_rect);
        
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderRect(renderer, &bg_rect);

        SDL_RenderPresent(renderer);
    }

    // Cleanup
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
