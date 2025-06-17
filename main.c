#include <SDL3/SDL.h>
#include <SDL3/SDL_render.h>
#include <SDL3_image/SDL_image.h>
#include <stdio.h>
#include <stdbool.h>
#include <math.h>
#include <stdlib.h>

typedef struct {
    SDL_FRect texture_rect;  // Coordinates in texture space
    bool active;
} Selection;

typedef struct {
    Selection* selections;
    int count;
    int capacity;
    bool is_dragging;
    SDL_FPoint drag_start;
    int current_selection;
} SelectionState;

void init_selection_state(SelectionState* state) {
    state->capacity = 10;
    state->selections = malloc(sizeof(Selection) * state->capacity);
    state->count = 0;
    state->is_dragging = false;
    state->current_selection = -1;
}

void add_selection(SelectionState* state, SDL_FRect texture_rect) {
    if (state->count >= state->capacity) {
        state->capacity *= 2;
        state->selections = realloc(state->selections, sizeof(Selection) * state->capacity);
    }
    
    state->selections[state->count].texture_rect = texture_rect;
    state->selections[state->count].active = true;
    state->count++;
}

void clear_selections(SelectionState* state) {
    state->count = 0;
    state->is_dragging = false;
    state->current_selection = -1;
}

void free_selection_state(SelectionState* state) {
    free(state->selections);
}

int main(int argc, char* argv[]) {
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);

    SDL_Window* window = SDL_CreateWindow("Multi-Selection Texture Area Selector", 800, 600, SDL_WINDOW_RESIZABLE);
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

    // Load a default texture if no file is provided
    const char* image_path = "texture.jpg";
    if (argc > 1) {
        image_path = argv[1];
    }

    SDL_Texture* texture = IMG_LoadTexture(renderer, image_path);
    if (!texture) {
        fprintf(stderr, "Failed to load texture: %s\n", SDL_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // Get texture dimensions
    int tex_width, tex_height;
    float w, h;
    SDL_GetTextureSize(texture, &w, &h);
    tex_width = (int)w; tex_height = (int)h;

    SelectionState selection_state;
    init_selection_state(&selection_state);

    bool running = true;
    printf("Controls:\n");
    printf("- Left click and drag to create selection\n");
    printf("- 'C' key to clear all selections\n");
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
                    }
                    break;

                case SDL_EVENT_WINDOW_RESIZED:
                    // Handle window resize
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

                        // Check if click is within the texture area
                        float mouse_x = event.button.x;
                        float mouse_y = event.button.y;
                        
                        if (mouse_x >= tex_display_x && mouse_x <= tex_display_x + tex_display_width &&
                            mouse_y >= tex_display_y && mouse_y <= tex_display_y + tex_display_height) {
                            selection_state.is_dragging = true;
                            selection_state.drag_start.x = mouse_x;
                            selection_state.drag_start.y = mouse_y;
                            selection_state.current_selection = -1; // New selection
                        }
                    }
                    break;

                case SDL_EVENT_MOUSE_BUTTON_UP:
                    if (event.button.button == SDL_BUTTON_LEFT && selection_state.is_dragging) {
                        selection_state.is_dragging = false;
                        
                        // Get window size
                        int win_width, win_height;
                        SDL_GetWindowSize(window, &win_width, &win_height);
                        
                        // Calculate texture display dimensions
                        float scale = fminf((float)win_width / tex_width, (float)win_height / tex_height);
                        float tex_display_width = tex_width * scale;
                        float tex_display_height = tex_height * scale;
                        float tex_display_x = (win_width - tex_display_width) / 2;
                        float tex_display_y = (win_height - tex_display_height) / 2;
                        
                        // Calculate current selection rectangle in screen space
                        float mouse_x = event.button.x;
                        float mouse_y = event.button.y;
                        
                        SDL_FRect screen_rect;
                        screen_rect.x = fminf(selection_state.drag_start.x, mouse_x);
                        screen_rect.y = fminf(selection_state.drag_start.y, mouse_y);
                        screen_rect.w = fabsf(mouse_x - selection_state.drag_start.x);
                        screen_rect.h = fabsf(mouse_y - selection_state.drag_start.y);
                        
                        // Clamp to texture area
                        screen_rect.x = fmaxf(screen_rect.x, tex_display_x);
                        screen_rect.y = fmaxf(screen_rect.y, tex_display_y);
                        screen_rect.w = fminf(screen_rect.w, tex_display_x + tex_display_width - screen_rect.x);
                        screen_rect.h = fminf(screen_rect.h, tex_display_y + tex_display_height - screen_rect.y);
                        
                        // Only add selection if it has meaningful size
                        if (screen_rect.w > 5 && screen_rect.h > 5) {
                            // Convert to texture coordinates
                            SDL_FRect texture_rect;
                            texture_rect.x = (screen_rect.x - tex_display_x) / scale;
                            texture_rect.y = (screen_rect.y - tex_display_y) / scale;
                            texture_rect.w = screen_rect.w / scale;
                            texture_rect.h = screen_rect.h / scale;
                            
                            add_selection(&selection_state, texture_rect);
                            
                            printf("Selection %d in texture coordinates:\n", selection_state.count);
                            printf("Top-left: (%.1f, %.1f)\n", texture_rect.x, texture_rect.y);
                            printf("Bottom-right: (%.1f, %.1f)\n", texture_rect.x + texture_rect.w, texture_rect.y + texture_rect.h);
                            printf("Width: %.1f, Height: %.1f\n\n", texture_rect.w, texture_rect.h);
                        }
                        
                        selection_state.current_selection = -1;
                    }
                    break;

                case SDL_EVENT_MOUSE_MOTION:
                    // No action needed for mouse motion when not dragging
                    break;
            }
        }

        // Clear screen
        SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
        SDL_RenderClear(renderer);

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
                // Convert texture coordinates back to screen coordinates
                SDL_FRect* tex_rect = &selection_state.selections[i].texture_rect;
                SDL_FRect screen_rect;
                screen_rect.x = tex_display_x + tex_rect->x * scale;
                screen_rect.y = tex_display_y + tex_rect->y * scale;
                screen_rect.w = tex_rect->w * scale;
                screen_rect.h = tex_rect->h * scale;
                
                // Draw green semi-transparent fill (much more transparent)
                SDL_SetRenderDrawColor(renderer, 0, 255, 0, 30);
                SDL_RenderFillRect(renderer, &screen_rect);
                
                // Draw green border
                SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
                SDL_RenderRect(renderer, &screen_rect);
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
            
            // Clamp to texture area
            current_rect.x = fmaxf(current_rect.x, tex_display_x);
            current_rect.y = fmaxf(current_rect.y, tex_display_y);
            current_rect.w = fminf(current_rect.w, tex_display_x + tex_display_width - current_rect.x);
            current_rect.h = fminf(current_rect.h, tex_display_y + tex_display_height - current_rect.y);
            
            // Draw green semi-transparent fill (much more transparent)
            SDL_SetRenderDrawColor(renderer, 0, 255, 0, 30);
            SDL_RenderFillRect(renderer, &current_rect);
            
            // Draw green border
            SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
            SDL_RenderRect(renderer, &current_rect);
        }

        // Present renderer
        SDL_RenderPresent(renderer);
    }

    // Cleanup
    free_selection_state(&selection_state);
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
