#ifndef DRAWING_H
#define DRAWING_H

#include <cglm/cglm.h>
#include "vulkan_public.h"
#include "loading.h"
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

/* Draw Commands solid_draw.c */
int draw_descriptors_update(GfxContext, GfxResources, int);
int draw_start(GfxContext*, GfxPipeline);

Entity entity_add1(GfxModelOffsets model, float x, float y, float z);
void render_entity(GfxContext, Camera, Entity);

int  draw_end(GfxContext, GfxPipeline, int);

/* Shapes */
int make_cube(GfxBuffer *geometry, GfxModelOffsets *model);
int make_plane(int, int, GfxBuffer*, GfxModelOffsets*);

int draw_descriptors_free(GfxContext context);

#endif /* SOLID_H */
