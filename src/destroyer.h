#ifndef DEFERD
#define DEFERD

#include "vulkan_public.h"

int deferd_add( void (*func)(GfxContext, void*), void* arg1);
int deferd_run(GfxContext context);

void deferd_buffer(GfxContext context, void* src_buffer);
void deferd_image(GfxContext context, void* src);
void deferd_VkPipelineLayout(GfxContext context, void* src);
void deferd_VkPipeline(GfxContext context, void* src);

#endif // DEFERD
