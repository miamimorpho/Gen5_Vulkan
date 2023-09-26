#include "solid.h"

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

void
buffers_destroy(GfxContext context, int count, DeviceBuffer *buffers)
{
  for(int i = 0; i < count; i++){
    vkUnmapMemory(context.l_dev, buffers[i].memory);
    vkDestroyBuffer(context.l_dev, buffers[i].handle, NULL);
    vkFreeMemory(context.l_dev, buffers[i].memory, NULL);
  }
}

/**
 * A generalised way to allocate memory for vertex buffers
 * 'properties' defines the type of memory we want to use
 * memory allocating
 */
int
buffer_create(GfxContext context, VkDeviceSize size, VkBufferUsageFlags usage, DeviceBuffer *buffer)
{
  int err = 0;
  
  VkBufferCreateInfo buffer_info = {
    .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
    .size = size,
    .usage = usage,
    .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
  };

  if(vkCreateBuffer(context.l_dev, &buffer_info, NULL, &buffer->handle)
     != VK_SUCCESS) err = 1;

  VkMemoryRequirements mem_requirements;
  vkGetBufferMemoryRequirements(context.l_dev, buffer->handle, &mem_requirements);
  
  VkMemoryAllocateInfo alloc_info = {
    .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
    .allocationSize = mem_requirements.size,
    .memoryTypeIndex = vram_type_index
    (context.p_dev, mem_requirements.memoryTypeBits,
     (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
      | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))
    };
  
  if(vkAllocateMemory(context.l_dev, &alloc_info, NULL, &buffer->memory)
     != VK_SUCCESS) err = 2;


  vkBindBufferMemory(context.l_dev, buffer->handle, buffer->memory, 0);
  vkMapMemory(context.l_dev, buffer->memory, 0,
	      size, 0, &buffer->first_index);

  buffer->size = size;
 
  return err;
}

void
image_destroy(GfxContext context, ImageData *image)
{
  vkDestroySampler(context.l_dev, image->sampler, NULL);
  vkDestroyImageView(context.l_dev, image->view, NULL);
  vkDestroyImage(context.l_dev, image->handle, NULL);
  vkFreeMemory(context.l_dev, image->memory, NULL);
}

int
image_create(GfxContext context, VkImage *image, VkDeviceMemory *image_memory,
	     VkFormat format, VkImageUsageFlags usage, uint32_t width, uint32_t height)
{
  /* Allocate VkImage memory */
  VkImageCreateInfo image_info = {
    .sType= VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
    .imageType = VK_IMAGE_TYPE_2D,
    .extent.width = width,
    .extent.height = height,
    .extent.depth = 1,
    .mipLevels = 1,
    .arrayLayers = 1,
    .format = format,
    .tiling = VK_IMAGE_TILING_OPTIMAL,
    .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    //.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
    .usage = usage,
    .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    .samples = VK_SAMPLE_COUNT_1_BIT,
    .flags = 0,
  };

  if(vkCreateImage(context.l_dev, &image_info, NULL, image)
     != VK_SUCCESS){
    printf("failed to create image!\n");
    return 1;
  }

  VkMemoryRequirements mem_requirements;
  vkGetImageMemoryRequirements(context.l_dev, *image, &mem_requirements);

  VkMemoryAllocateInfo alloc_info = {
    .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
    .allocationSize = mem_requirements.size,
    .memoryTypeIndex = vram_type_index(context.p_dev,
				       mem_requirements.memoryTypeBits,
				       VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT),
  };

  if(vkAllocateMemory(context.l_dev, &alloc_info, NULL, image_memory)
     != VK_SUCCESS) {
    printf("failed to allocate image memory\n");
    return 1;
  }

  vkBindImageMemory(context.l_dev, *image, *image_memory, 0);

  return 0;
}

int
image_view_create(VkDevice l_dev, VkImage image,
		  VkImageView *view, VkFormat format,
		  VkImageAspectFlags aspect_flags)
{
 VkImageViewCreateInfo create_info = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .image = image,
      .viewType = VK_IMAGE_VIEW_TYPE_2D,
      .format = format,
      .components.r = VK_COMPONENT_SWIZZLE_IDENTITY,
      .components.g = VK_COMPONENT_SWIZZLE_IDENTITY,
      .components.b = VK_COMPONENT_SWIZZLE_IDENTITY,
      .components.a = VK_COMPONENT_SWIZZLE_IDENTITY,
      .subresourceRange.aspectMask = aspect_flags,
      .subresourceRange.baseMipLevel = 0,
      .subresourceRange.levelCount = 1,
      .subresourceRange.baseArrayLayer = 0,
      .subresourceRange.layerCount = 1,
    };
    if (vkCreateImageView(l_dev, &create_info, NULL, view)
      != VK_SUCCESS){
      printf("!failed to create image views!\n");
      return 1;
    }
    return 0;
}




