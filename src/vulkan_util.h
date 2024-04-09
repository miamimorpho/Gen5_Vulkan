#include <vulkan/vulkan.h>
#include <stdio.h>

#include "vulkan_public.h"

int
vram_type_index(VkPhysicalDevice p_dev, uint32_t type_filter,
		VkMemoryPropertyFlags required_properties);
int
transition_image_layout(GfxContext context, VkImage image,
			VkImageLayout old_layout, VkImageLayout new_layout);
int copy_buffer_to_image(GfxContext, VkBuffer, VkImage, uint32_t, uint32_t );
int sampler_create(GfxContext, VkSampler* );

VkCommandBuffer command_single_begin(GfxContext context);
int command_single_end(GfxContext context, VkCommandBuffer *command_buffer);
