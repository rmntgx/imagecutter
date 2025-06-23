#include "draw.h"

void draw_rotation_icon(SDL_Renderer* renderer, SDL_FRect* rect, Rotation rotation) {
	float center_x = rect->x + rect->w / 2;
	float center_y = rect->y + rect->h / 2;
	float size = fminf(rect->w, rect->h) * 0.1f;
	if (size < 10) size = 10;
	if (size > 30) size = 30;

	SDL_FColor color = { 0, 0, 0, 0.5 };

	SDL_Vertex verts[3];
	switch (rotation) {
		case ROTATION_0:
			verts[0].position.x = center_x;
			verts[0].position.y = center_y - size;

			verts[1].position.x = center_x - size * 0.6f;
			verts[1].position.y = center_y + size * 0.6f;

			verts[2].position.x = center_x + size * 0.6f;
			verts[2].position.y = center_y + size * 0.6f;
			break;

		case ROTATION_90:
			verts[0].position.x = center_x + size;
			verts[0].position.y = center_y;

			verts[1].position.x = center_x - size * 0.6f;
			verts[1].position.y = center_y - size * 0.6f;

			verts[2].position.x = center_x - size * 0.6f;
			verts[2].position.y = center_y + size * 0.6f;
			break;

		case ROTATION_180:
			verts[0].position.x = center_x;
			verts[0].position.y = center_y + size;

			verts[1].position.x = center_x - size * 0.6f;
			verts[1].position.y = center_y - size * 0.6f;

			verts[2].position.x = center_x + size * 0.6f;
			verts[2].position.y = center_y - size * 0.6f;
			break;

		case ROTATION_270:
			verts[0].position.x = center_x - size;
			verts[0].position.y = center_y;

			verts[1].position.x = center_x + size * 0.6f;
			verts[1].position.y = center_y - size * 0.6f;

			verts[2].position.x = center_x + size * 0.6f;
			verts[2].position.y = center_y + size * 0.6f;
			break;
	}

	for (int i = 0; i < 3; ++i) {
		verts[i].color = color;
		verts[i].tex_coord = (SDL_FPoint){0, 0};
	}

	int indices[3] = { 0, 1, 2 };
	SDL_RenderGeometry(renderer, NULL, verts, 3, indices, 3);
}
