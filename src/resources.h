#ifndef RESOURCES_H
#define RESOURCES_H

#include "vulkan_public.h"
#include "drawing.h"
#include "loading.h"

void draw_descriptors_create(GfxContext, GfxResources, int, GfxBuffer*);
int texture_descriptors_update(GfxContext, uint32_t, GfxResources);
  
int texture_descriptors_alloc(VkDevice, GfxResources*);
int texture_descriptors_update(GfxContext, uint32_t, GfxResources);

int resources_init(GfxContext context, GfxResources* resources);
int resources_destroy(GfxContext context, GfxResources resources);

int unused_textures_free(GfxContext, Entity*, uint32_t, ImageData*);


#endif /* RESOURCES_H */
