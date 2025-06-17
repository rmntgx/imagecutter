#include <SDL3/SDL.h>

typedef enum {
    ROTATION_0 = 0,
    ROTATION_90 = 1,
    ROTATION_180 = 2,
    ROTATION_270 = 3
} Rotation;

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
