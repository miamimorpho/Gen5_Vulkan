#include "vulkan_public.h"
#include "vulkan_util.h"
#include <string.h>

int
buffers_destroy(GfxContext context, GfxBuffer *buffers, int count)
{
  int err = 0;
  
  for(int i = 0; i < count; i++){
    if(buffers[i].p_next != NULL){
      GfxBuffer* linked_buffer = (GfxBuffer*)buffers[i].p_next;
      err = buffers_destroy(context, linked_buffer, 1);
    }
    vkUnmapMemory(context.l_dev, buffers[i].memory);
    vkDestroyBuffer(context.l_dev, buffers[i].handle, NULL);
    vkFreeMemory(context.l_dev, buffers[i].memory, NULL);
  }

  return err;
}

int
buffer_append(const void *data, GfxBuffer *dest, size_t size)
{
  if(dest->total_size < dest->used_size + size )return 1;

  void *write = ((uint8_t*)dest->first_ptr) + dest->used_size;
  memcpy(write, data, size);
  dest->used_size += size;
  return 0;
}

int
buffer_write(const void *data, GfxBuffer *dest, size_t size, size_t offset){

  if(dest->total_size < offset + size)return 1;
  
  void *write = ((uint8_t*)dest->first_ptr) + offset;
  memcpy(write, data, size);
  return 0;
}

int
buffer_create(GfxContext context, VkDeviceSize size, VkBufferUsageFlags usage, GfxBuffer *buffer)
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
	      size, 0, &buffer->first_ptr);


  buffer->total_size = size;
  buffer->used_size = 0;
  buffer->p_next = NULL;
 
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
    .anisotropyEnable = VK_FALSE,
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




