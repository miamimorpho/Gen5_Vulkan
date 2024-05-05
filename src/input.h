#ifndef INPUT_H
#define INPUT_H

#include <SDL2/SDL.h>

int main_input(GfxContext, Camera*);

/* Camera Commands camera.c */
int camera_rotate(Camera*, float, float);
Camera camera_create(int, int);

#endif /* INPUT_H */
