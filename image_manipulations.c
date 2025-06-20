#include "image_manipulations.h"

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

	int bpp	   = SDL_BYTESPERPIXEL(original->format);
	int src_pitch = original->pitch;
	int dst_pitch = rotated->pitch;
	Uint8 *src	= (Uint8*)original->pixels;
	Uint8 *dst	= (Uint8*)rotated->pixels;

	for (int ny = 0; ny < new_h; ++ny) {
		Uint8 *dst_row = dst + ny * dst_pitch;
		for (int nx = 0; nx < new_w; ++nx) {
			int sx, sy;
			switch (rotation) {
				case ROTATION_90:
					sx = ny;
					sy = h - 1 - nx;
					break;
				case ROTATION_180:
					sx = w - 1 - nx;
					sy = h - 1 - ny;
					break;
				case ROTATION_270:
					sx = w - 1 - ny;
					sy = nx;
					break;
				default:
					sx = nx; sy = ny;
			}

			Uint8 *src_px = src + sy * src_pitch + sx * bpp;
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
