#ifndef MODEL_H
#define MODEL_H

#include "core.h"
#include <cglm/cglm.h>

typedef struct {
  /* Rapid Descriptor Data */
  vec3 pos;
  uint32_t texture_index;

  /* Slow Descriptor Data */
  uint32_t indices_len;
  uint32_t first_index;
  uint32_t vertex_offset;
} Entity;

typedef struct{
  uint32_t texture_index;
  mat4 transform;
} drawArgs;

/* models.c */
int geometry_buffer_create(GfxContext context, int count, GfxBuffer *dest);
int geometry_buffer_bind(VkCommandBuffer, GfxBuffer);
int models_destroy(GfxContext, uint32_t, GfxBuffer, ImageData* );

/* entity.c */
int    draw_indirect_update(GfxBuffer buffer, Entity *entities);
Entity entity_duplicate(Entity mother, float x, float y, float z);
int    entity_gltf_load(GfxContext, const char*, GfxBuffer*, ImageData*, Entity*);
void   draw_args_create(GfxContext, GfxPipeline, int, GfxBuffer* );
void   draw_args_update(Camera, Entity*, GfxBuffer);

#endif /* MODEL_H */
