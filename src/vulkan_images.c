#include "vulkan_public.h"
#include "vulkan_util.h"

#define STB_IMAGE_IMPLEMENTATION
#include "../extern/stb_image.h"

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

int
slow_descriptors_alloc(VkDevice l_dev, GfxPipeline *pipeline)
{
  uint32_t max_binding = MAX_SAMPLERS -1;
  VkDescriptorSetVariableDescriptorCountAllocateInfo count_info = {
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO_EXT,
    .descriptorSetCount = 1,
    .pDescriptorCounts = &max_binding,
  };
  
  /* Allocate descriptor memory */
  VkDescriptorSetAllocateInfo alloc_info = {
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
    .descriptorPool = pipeline->descriptor_pool,
    .descriptorSetCount = 1,
    .pSetLayouts = &pipeline->slow_layout,
    .pNext = &count_info,
  };

  if(vkAllocateDescriptorSets(l_dev, &alloc_info, &pipeline->slow_set)
     != VK_SUCCESS) return 1;

  return 0;
}

int
slow_descriptors_update(GfxContext context, uint32_t count,
			ImageData *textures, GfxPipeline *pipeline)
/* count is the quantity of unique texture files
   the renderer is going to need to allocate GPU memory for
   to use at one time
 */
{
  VkDescriptorImageInfo infos[count];
  VkWriteDescriptorSet writes[count];
  
  for(uint32_t i = 0; i < count; i++){
  
    infos[i] = (VkDescriptorImageInfo){
      .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
      .imageView = textures[i].view,
      .sampler = textures[i].sampler,
    };
    
    writes[i] = (VkWriteDescriptorSet){
      .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
      .dstSet = pipeline->slow_set,
      .dstBinding = 0,
      .dstArrayElement = i,
      .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
      .descriptorCount = 1,
      .pImageInfo = &infos[i],
    };
  }

  vkUpdateDescriptorSets(context.l_dev, count, writes, 0, NULL); 
  
  return 0;
}

int image_file_load(GfxContext context, unsigned char *data,
		    size_t offset, size_t size, size_t stride,
		    ImageData *image){
  /* load raw data */
  stbi_uc* raw_pixels = malloc(size);
  for(unsigned int i = 0; i < size; i++){
    raw_pixels[i] = data[offset];
    offset += stride;
  }

  /* Choose image format */
  int width, height, channels;
  stbi_info_from_memory(raw_pixels, size,
			&width, &height, &channels);
  VkFormat format;
  switch(channels){
  case 4:
    format = VK_FORMAT_R8G8B8A8_SRGB;
    break;
  case 3:
    format = VK_FORMAT_R8G8B8_SRGB;
    break;
  default:
    return 2;
  }
  
  /* interpret view file */
  stbi_uc* pixels = stbi_load_from_memory
    ((stbi_uc*)raw_pixels, size,
     &width, &height, &channels, channels);
  if(pixels == NULL) {
    printf("failed to interpret raw pixel data\n");
    return 2;
  }
  free(raw_pixels);

  VkDeviceSize image_size = width * height * channels;
  GfxBuffer image_b;
  buffer_create(context, image_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		&image_b);

  /* copy pixel data to buffer */
  memcpy(image_b.first_ptr, pixels, image_size);

  stbi_image_free(pixels);
  //  free(pixels);

  image_create(context, &image->handle, &image->memory,
	       format,
	       VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
	       width, height);

  /* Copy the image buffer to a VkImage proper */
  transition_image_layout(context, image->handle,
			  VK_IMAGE_LAYOUT_UNDEFINED,
			  VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

  copy_buffer_to_image(context, image_b.handle, image->handle,
		       (uint32_t)width, (uint32_t)height);

  transition_image_layout(context, image->handle,
			  VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			  VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

  buffers_destroy(context, &image_b, 1);
  
  /* Create image view */
  image_view_create(context.l_dev, image->handle,
		    &image->view, format,
		    VK_IMAGE_ASPECT_COLOR_BIT);
  sampler_create(context, &image->sampler);
  
  
  return 0;
  /* images->buffer_view->buffers */
  /* loaded by cgltf_load_buffers */
}



