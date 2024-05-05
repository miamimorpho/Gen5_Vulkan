#include "vulkan_public.h"

int textures_init(GfxContext*);

int
texture_load(GfxContext context, unsigned char* pixels,
	     int width, int height, int channels, uint32_t*);
int texture_file_load(GfxContext context, const char* filename, uint32_t* index);
int texture_memory_load(GfxContext, uint8_t*, size_t, uint32_t*);
int texture_descriptors_alloc(GfxContext,
			      VkDescriptorSetLayout, VkDescriptorSet*, int);
int texture_descriptors_update(GfxContext, uint32_t);
int textures_free(GfxContext);
