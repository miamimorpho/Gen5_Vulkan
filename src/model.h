#ifndef MODEL_H
#define MODEL_H

#include "vulkan_public.h"

typedef struct {
  vertex3* vertices;
  uint32_t vertex_c;
  uint32_t* indices;
  uint32_t index_c;
} GfxStagingMesh;

int gltf_load(GfxContext, const char*, GfxModelOffsets*, GfxBuffer*);
int mesh_b_create(GfxContext, GfxBuffer*, size_t);
int mesh_b_load(GfxContext, GfxStagingMesh, GfxBuffer*, GfxModelOffsets*);
int mesh_b_bind(GfxContext, GfxBuffer);

#endif /*MODEL_H*/
