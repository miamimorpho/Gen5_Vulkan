#ifndef RESOURCES_H
#define RESOURCES_H

#include "vulkan_public.h"
#include "drawing.h"
#include "loading.h"



int resources_init(GfxContext, GfxResources*);
int resources_bind(GfxContext, GfxPipeline, GfxResources resources);

int texture_descriptors_alloc(VkDevice, GfxResources*);
int texture_descriptors_update(GfxContext, GfxResources, uint32_t);

int resources_destroy(GfxContext, GfxResources);
int unused_textures_free(GfxContext, Entity*, uint32_t, ImageData*);


#endif /* RESOURCES_H */
