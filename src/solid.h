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
  uint32_t indexCount;
  uint32_t firstIndex;
  uint32_t vertexOffset;
} Entity;

typedef struct{
  mat3 normalMatrix;
  mat4 mvp;
  uint32_t texIndex;
} drawArgs;

/*
  typedef struct VkDrawIndexedIndirectCommand {
    uint32_t    indexCount;
    uint32_t    instanceCount;
    uint32_t    firstIndex;
    int32_t     vertexOffset;
    uint32_t    firstInstance;
} VkDrawIndexedIndirectCommand;
*/

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

int vertex_buffer_append(const vertex*, GfxBuffer*, uint32_t);
int index_buffer_append(const uint32_t*, GfxBuffer*, uint32_t);



int entity_gltf_load(GfxContext, const char*, GfxBuffer*, ImageData*, Entity*);
int models_destroy(GfxContext, uint32_t, GfxBuffer, ImageData* );
Entity entity_add1(Entity, float, float, float);

int make_plane(int width, int height, GfxBuffer* dest, Entity* indirect);

#endif /* SOLID_H */
