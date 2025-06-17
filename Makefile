all:
	gcc main.c -lSDL3 -lSDL3_image -lm -o imagecutter
debug:
	gcc main.c -lSDL3 -lSDL3_image -lm -o imagecutter -g -O0
