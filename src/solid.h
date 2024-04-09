#ifndef SOLID_H
#define SOLID_H

#include <cglm/cglm.h>
#include "vulkan_public.h"


typedef struct {
  vec3 pos;

  vec3 up;
  vec3 front;
  vec3 right;
  
  mat4 projection;
} Camera;

typedef struct {
  /* Rapid Descriptor Data */
  vec3 pos;
  versor rotate;
  uint32_t texture_index;

  /* Slow Descriptor Data */
  uint32_t indices_len;
  uint32_t first_index;
  uint32_t vertex_offset;
} Entity;

typedef struct{
  mat3 normalMatrix;
  mat4 mvp;
  uint32_t texIndex;
} drawArgs;

/* Camera Commands solid_camera.c */

/* camera.c */
int camera_rotate(Camera*, float, float);
Camera camera_create(int, int);

/* Draw Commands solid_draw.c */
int  draw_start(GfxContext*, GfxPipeline);
void draw_args_create(GfxContext, GfxPipeline, int, GfxBuffer* );
void draw_args_update(Camera, Entity*, GfxBuffer);
int  draw_indirect_create(GfxContext, GfxBuffer*, int);
int  draw_indirect_update(GfxBuffer, Entity* );
int  draw_end(GfxContext, GfxPipeline);

/* Model Loading solid_model.c */
int geometry_buffer_create(GfxContext context, int count, GfxBuffer *dest);
int geometry_buffer_bind(VkCommandBuffer, GfxBuffer);
int entity_gltf_load(GfxContext, const char*, GfxBuffer*, ImageData*, Entity*);
int models_destroy(GfxContext, uint32_t, GfxBuffer, ImageData* );
Entity entity_add1(Entity, float, float, float);


#endif /* SOLID_H */
