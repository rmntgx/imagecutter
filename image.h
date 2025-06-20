#ifndef IMAGE_H
#define IMAGE_H

#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
	char** image_paths;
	int count;
	int current_index;
	int total_cropped;
} ImageList;

void init_image_list(ImageList* list, int argc, char* argv[]);
void free_image_list(ImageList* list);
char* get_base_filename(const char* path);

#endif /* IMAGE_H */
