#include "vulkan_util.h"

/**
 * type_filter is the  memory type vulkan needs to use
 * mem_properties lists all available memory types on device
 * required_properties mitmasks the memory type that we want to use
 * i is memory type index, we return the best, if cant then -1
 */
int
vram_type_index(VkPhysicalDevice p_dev, uint32_t type_filter,
	  VkMemoryPropertyFlags required_properties)
{
  VkPhysicalDeviceMemoryProperties mem_properties;
  vkGetPhysicalDeviceMemoryProperties(p_dev, &mem_properties);

  /* (1U << i): sets the i-th bit to 1 and all other bits to 0
     This creates a bitmask where only the i-th bit is set. */

  int memory_type_index = -1;
  for(uint32_t i = 0; i < mem_properties.memoryTypeCount; i++){
    if( type_filter & (1 << i)){
      
      if ((mem_properties.memoryTypes[i].propertyFlags & required_properties)
	  == required_properties){
	memory_type_index = i;
      }
    }
  }

  if(memory_type_index < 0)
    printf("no shared host/device memory\n");
 
  return memory_type_index;
}

int
transition_image_layout(GfxContext context, VkImage image,
			VkImageLayout old_layout, VkImageLayout new_layout)
{
  VkCommandBuffer commands = command_single_begin(context);

  VkImageMemoryBarrier barrier = {
    .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
    .oldLayout = old_layout,
    .newLayout = new_layout,
    .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
    .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
    .image = image,
    .subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
    .subresourceRange.baseMipLevel = 0,
    .subresourceRange.levelCount = 1,
    .subresourceRange.baseArrayLayer = 0,
    .subresourceRange.layerCount = 1,
    .srcAccessMask = 0,
    .dstAccessMask = 0,
  };

  VkPipelineStageFlags source_stage, destination_stage;
  
  if(old_layout == VK_IMAGE_LAYOUT_UNDEFINED
     && new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL){
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    destination_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;

  }else if(old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
	    && new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL){
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    
    source_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    destination_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    
  } else {
    printf("unsupported layout transition\n");
    return 1;
  }
  
  vkCmdPipelineBarrier(commands,
		       source_stage, destination_stage,
		       0,
		       0, NULL,
		       0, NULL,
		       1, &barrier);
  
  return command_single_end(context, &commands);
}

int
copy_buffer_to_image(GfxContext context,
		     VkBuffer buffer, VkImage image,
		     uint32_t width, uint32_t height)
{
  VkCommandBuffer command = command_single_begin(context);
  
  VkBufferImageCopy region = {
    .bufferOffset = 0,
    .bufferRowLength = 0,
    .bufferImageHeight = 0,

    .imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
    .imageSubresource.mipLevel = 0,
    .imageSubresource.baseArrayLayer = 0,
    .imageSubresource.layerCount = 1,

    .imageOffset = {0, 0, 0},
    .imageExtent  = {width, height, 1},
  };

  vkCmdCopyBufferToImage(command, buffer, image,
			 VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			 1, &region);
  
  return command_single_end(context, &command);
}

int
sampler_create(GfxContext context, VkSampler *sampler)
{

  VkPhysicalDeviceProperties properties = { 0 };
  vkGetPhysicalDeviceProperties(context.p_dev, &properties);
  
  VkSamplerCreateInfo sampler_info = {
    .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
    .magFilter = VK_FILTER_NEAREST,
    .minFilter = VK_FILTER_NEAREST,
    .addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
    .addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
    .addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
    .anisotropyEnable = VK_TRUE,
    .maxAnisotropy = properties.limits.maxSamplerAnisotropy,
    .borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
    .unnormalizedCoordinates = VK_FALSE,
    .compareEnable = VK_FALSE,
    .compareOp = VK_COMPARE_OP_ALWAYS,
    .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
    .mipLodBias = 0.0f,
    .minLod = 0.0f,
    .maxLod = 0.0f,
  };

  if(vkCreateSampler(context.l_dev, &sampler_info, NULL, sampler)
     != VK_SUCCESS){
    printf("!failed to create buffer\n!");
    return 1;
  }
  
  return 0;
  
}