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

/* Draw Commands solid_draw.c */
int draw_start(GfxContext*, GfxShader shader);

Entity entity_add1(GfxModelOffsets model, float x, float y, float z);
void draw_entities(GfxContext, Camera, Entity*, int count);
int draw_indirect_init(GfxContext, VkDescriptorSet*, size_t);
int draw_indirect_free(GfxContext context, GfxShader* shader);

int
draw_end(GfxContext context, int entity_c, GfxShader shader);

#endif /* SOLID_H */
