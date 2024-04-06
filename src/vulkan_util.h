#include "core.h"
#include <vulkan/vulkan.h>
#include <stdio.h>

int
vram_type_index(VkPhysicalDevice p_dev, uint32_t type_filter,
		VkMemoryPropertyFlags required_properties);

int
transition_image_layout(GfxContext context, VkImage image,
			VkImageLayout old_layout, VkImageLayout new_layout);

int
copy_buffer_to_image(GfxContext context,
		     VkBuffer buffer, VkImage image,
		     uint32_t width, uint32_t height);

int
sampler_create(GfxContext context, VkSampler *sampler);
