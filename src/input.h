#ifndef INPUT_H
#define INPUT_H

#include <SDL2/SDL.h>

typedef struct {
  vec3 pos;

  vec3 up;
  vec3 front;
  vec3 right;
  
  mat4 projection;
} Camera;


int main_input(GfxContext, Camera*);

/* Camera Commands camera.c */
int camera_rotate(Camera*, float, float);
Camera camera_create(int, int);

#endif /* INPUT_H */
