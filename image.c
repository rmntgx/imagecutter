#include "image.h"

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
