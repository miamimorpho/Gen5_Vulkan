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
int  draw_start(GfxContext*, GfxPipeline);
int  draw_indirect_create(GfxContext, GfxBuffer*, int);
void draw_descriptors_update(Camera, Entity *entities, GfxBuffer);
int  draw_indirect_update(GfxBuffer, Entity* );
int  draw_end(GfxContext, GfxPipeline);

/* Geometry Memory Mangement */
int geometry_buffer_bind(VkCommandBuffer, GfxBuffer);

/* Shapes */
int make_cube(GfxBuffer *geometry, GfxModelOffsets *model);
int make_plane(int, int, GfxBuffer*, GfxModelOffsets*);


Entity entity_add1(GfxModelOffsets model, float x, float y, float z);


#endif /* SOLID_H */
