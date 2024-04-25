#ifndef RESOURCES_H
#define RESOURCES_H

#include "vulkan_public.h"
#include "vulkan_util.h"
#include <string.h>

typedef struct {
 
  uint32_t firstIndex;
  uint32_t indexCount;
  uint32_t vertexOffset;
  uint32_t textureIndex;
} GfxModelOffsets;

int png_load(GfxContext, const char*, GfxResources*);
int entity_gltf_load(GfxContext, const char*,
		     GfxModelOffsets*, GfxResources*);

int geometry_buffer_create(GfxContext, GfxBuffer*, size_t);
int geometry_load(const vertex*, uint32_t,
		  const uint32_t*, uint32_t, GfxBuffer*, GfxModelOffsets*);
int geometry_bind(GfxPipeline, GfxBuffer);

int resources_init(GfxContext, GfxResources*);
int resources_bind(GfxContext, GfxPipeline, GfxResources resources);

int texture_load(GfxContext, unsigned char*, ImageData*,
		 int width, int height, int channels);
int textures_slot(GfxResources resources);
  
int texture_descriptors_update(GfxContext, GfxResources, uint32_t);

int resources_destroy(GfxContext, GfxResources);

#endif /* RESOURCES_H */
