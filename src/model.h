#ifndef MODEL_H
#define MODEL_H

#include "vulkan_public.h"

typedef struct {
  vertex3* vertices;
  uint32_t vertex_c;
  uint32_t* indices;
  uint32_t index_c;
} GfxStagingMesh;

typedef struct {
  
  VkPipelineLayout pipeline_layout;
  VkPipeline pipeline;

  VkDescriptorSetLayout descriptors_layout;
  VkDescriptorSet* descriptors;
  GfxBuffer* uniform_b;
  GfxBuffer indirect_b;

} model_shader_t;

typedef struct {
  vec3 pos;
  versor rotate;
  GfxModelOffsets model;
} Entity;

typedef struct{
  mat4 rotate_m;
  mat4 mvp;
  uint32_t texIndex;
} drawArgs;

typedef struct {
  vec3 pos;

  vec3 up;
  vec3 front;
  vec3 right;
  
  mat4 projection;
} Camera;

int gltf_load(GfxContext, const char*, GfxModelOffsets*, GfxBuffer*);
int mesh_b_create(GfxContext, GfxBuffer*, size_t);
int mesh_b_load(GfxContext, GfxStagingMesh, GfxBuffer*, GfxModelOffsets*);
int mesh_b_bind(GfxContext, GfxBuffer);

int model_shader_create(GfxContext, model_shader_t*);
int model_shader_bind(GfxContext, model_shader_t);
int
model_descriptors_update(GfxContext, model_shader_t*, size_t);
void model_shader_draw(GfxContext, model_shader_t, Camera, Entity*, int);
int model_shader_destroy(GfxContext, model_shader_t*);

#endif /*MODEL_H*/
