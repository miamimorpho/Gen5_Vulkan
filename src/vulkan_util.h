#include <vulkan/vulkan.h>
#include <stdio.h>

#include "vulkan_public.h"

int
shader_spv_load(VkDevice l_dev, const char* filename, VkShaderModule* shader);


int pipeline_layout_create(VkDevice l_dev, VkDescriptorSetLayout*, VkPipelineLayout*);

int vram_type_index(VkPhysicalDevice, uint32_t, VkMemoryPropertyFlags);
int transition_image_layout(GfxContext, VkImage, VkImageLayout, VkImageLayout);
int copy_buffer_to_image(GfxContext, VkBuffer, VkImage, uint32_t, uint32_t );
int sampler_create(GfxContext, VkSampler* );

VkCommandBuffer command_single_begin(GfxContext context);
int command_single_end(GfxContext context, VkCommandBuffer *command_buffer);
