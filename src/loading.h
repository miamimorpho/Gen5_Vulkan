#ifndef LOADING_H
#define LOADING_H

#include "vulkan_public.h"
#include "vulkan_util.h"
#include <string.h>

typedef struct {
 
  uint32_t firstIndex;
  uint32_t indexCount;
  uint32_t vertexOffset;
  uint32_t textureIndex;
} GfxModelOffsets;

int vertex_buffer_append(const vertex *vertices, GfxBuffer *dest, uint32_t count );
int index_buffer_append(const uint32_t *indices, GfxBuffer *dest, uint32_t count);

int png_load(GfxContext, const char*, GfxResources*);
int entity_gltf_load(GfxContext, const char*,
		     GfxModelOffsets*, GfxResources*);

#endif /* LOADING_H */
