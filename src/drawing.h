#ifndef DRAWING_H
#define DRAWING_H

#include <cglm/cglm.h>
#include "vulkan_public.h"
#include "model.h"
#include "input.h"

typedef struct{
  mat4 rotate_m;
  mat4 mvp;
  uint32_t texIndex;
} drawArgs;

typedef struct {
  vec3 pos;
  versor rotate;
  GfxModelOffsets model;
} Entity;

int indirect_b_create(GfxContext context, GfxShader* shader, size_t draw_count);

/* Draw Commands solid_draw.c */
int draw_start(GfxContext*);

Entity entity_add1(GfxModelOffsets model, float x, float y, float z);
void draw_entities(GfxContext, GfxShader, Camera, Entity*, int count);

int
draw_end(GfxContext context);

#endif /* SOLID_H */
