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
int geometry_init(GfxContext, GfxBuffer*, size_t);
int geometry_load(GfxContext context,
		  GfxStagingMesh,
		  GfxBuffer* dest, GfxModelOffsets* model);
int geometry_bind(GfxContext, GfxBuffer);

#endif /*MODEL_H*/
