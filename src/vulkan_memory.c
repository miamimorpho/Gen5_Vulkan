#include "vulkan_public.h"
#include "vulkan_util.h"
#include <string.h>

int
buffers_destroy(GfxContext context, GfxBuffer* buffers, int count){
  int err;
  for(int i = 0; i < count; i++)
    err = buffer_destroy(context, buffers[i]);

  return err;
}

int
buffer_destroy(GfxContext context, GfxBuffer b)
{
  
  int err = 0;
 
  VmaAllocationInfo b_info;
  vmaGetAllocationInfo(context.allocator, b.allocation, &b_info);
    
  if(b_info.pUserData != NULL){
    GfxBuffer* linked_buffer = (GfxBuffer*)b_info.pUserData;
    err = buffer_destroy(context, *linked_buffer);
    free(linked_buffer);
  }
  
  vmaDestroyBuffer(context.allocator, b.handle, b.allocation);
  return err;
}

int
buffer_create(GfxContext context, GfxBuffer* buffer,
	      VkBufferUsageFlags usage,
	      VmaAllocatorCreateFlags flags,
	      VkDeviceSize size){
 
  VkBufferCreateInfo buffer_info = {
    .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
    .size = size,
    .usage = usage
  };

/* potential usage flags
 * VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
 * VK_BUFFER_USAGE_INDEX_BUFFER_BIT
 * VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT
 * VK_BUFFER_USAGE_STORAGE_BUFFER_BIT
 * VK_BUFFER_USAGE_TRANSFER_DST_BIT
 *
 */
  
  VmaAllocationCreateInfo vma_alloc_info = {
    .usage = VMA_MEMORY_USAGE_AUTO,
    .flags = flags,
  };

  if(vmaCreateBuffer(context.allocator, &buffer_info, &vma_alloc_info,
			    &buffer->handle,
			    &buffer->allocation,
		     NULL) != VK_SUCCESS)return 1;

  buffer->used_size = 0;
  
  return 0;
}

int buffer_append(GfxContext context, GfxBuffer *dest,
		  const void* src, VkDeviceSize src_size)
{
  VmaAllocationInfo dest_info;
  vmaGetAllocationInfo(context.allocator, dest->allocation, &dest_info);
  
  if(dest->used_size + src_size > dest_info.size)
    return 1;
  
  vmaCopyMemoryToAllocation(context.allocator, src,
			    dest->allocation, dest->used_size,
			    src_size);
  dest->used_size += src_size;
  return 0;
}

GfxBuffer* buffer_next(GfxContext context, GfxBuffer first)
{
  VmaAllocationInfo first_info;
  vmaGetAllocationInfo(context.allocator, first.allocation,
		       &first_info);
  
  GfxBuffer* second = (GfxBuffer*)first_info.pUserData;
  return second;
}

void
image_destroy(GfxContext context, GfxImage image)
{
  vkDestroySampler(context.l_dev, image.sampler, NULL);
  vkDestroyImageView(context.l_dev, image.view, NULL);
  vmaDestroyImage(context.allocator, image.handle, image.allocation);
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
image_create(GfxContext context,
	     VkImage *image, VmaAllocation *image_alloc,
	     VkImageUsageFlags usage, VkFormat format,
	     uint32_t width, uint32_t height){

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
  };

  VmaAllocationCreateInfo alloc_create_info = {
    .usage = VMA_MEMORY_USAGE_AUTO,
    .flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
    .priority = 1.0f,
  };
  
  vmaCreateImage(context.allocator, &image_info, &alloc_create_info,
		 image, image_alloc, NULL);
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




