#include "textures.h"
#include "vulkan_util.h"

#define STB_IMAGE_IMPLEMENTATION
#include "../extern/stb_image.h"

static GfxImage g_texture_array[MAX_SAMPLERS];

/*
 * Set = 0, Binding = Frag on 0
 * Bindless
 *.No Side Effects, Layout must be destroyed
 */
int
texture_descriptors_layout(VkDevice l_dev, VkDescriptorSetLayout* sampler_layout)
{
  
  VkDescriptorSetLayoutBinding binding = {
    .binding = 0,
    .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
    .descriptorCount = MAX_SAMPLERS,
    .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
  };
  
  VkDescriptorBindingFlags bindless_flags =
    VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT_EXT
    | VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT_EXT
    | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT_EXT;
  
  VkDescriptorSetLayoutBindingFlagsCreateInfo info2 = {
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO,
    .bindingCount = 1,
    .pBindingFlags = &bindless_flags,
  };
  
  VkDescriptorSetLayoutCreateInfo info = {
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
    .bindingCount = 1,
    .pBindings = &binding,
    .flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT_EXT,
    .pNext = &info2,
  };
  
  if(vkCreateDescriptorSetLayout(l_dev, &info, NULL,
				 sampler_layout) != VK_SUCCESS) return 1;

  return 0;

}

int
texture_descriptors_alloc(GfxContext context,
			  VkDescriptorSetLayout descriptors_layout,
			  VkDescriptorSet *descriptors,
			  int max_samplers)
{
  uint32_t max_binding = max_samplers -1;
  VkDescriptorSetVariableDescriptorCountAllocateInfo count_info = {
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO_EXT,
    .descriptorSetCount = 1,
    .pDescriptorCounts = &max_binding,
  };
  
  VkDescriptorSetAllocateInfo alloc_info = {
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
    .descriptorPool = context.descriptor_pool,
    .descriptorSetCount = 1,
    .pSetLayouts = &descriptors_layout,
    .pNext = &count_info,
  };

  if(vkAllocateDescriptorSets(context.l_dev, &alloc_info, descriptors)
     != VK_SUCCESS) return 1;

  return 0;
}

int textures_init(GfxContext* context)
{

  texture_descriptors_layout(context->l_dev, &context->texture_descriptors_layout);
  texture_descriptors_alloc(*context, context->texture_descriptors_layout,
			    &context->texture_descriptors,
			    MAX_SAMPLERS);

  for(int i = 0; i < MAX_SAMPLERS; i++){
    g_texture_array[i].handle = VK_NULL_HANDLE;
    g_texture_array[i].view = VK_NULL_HANDLE;
  }

  return MAX_SAMPLERS;
}

int textures_get_slot()
{
  for(uint32_t i = 0; i < MAX_SAMPLERS; i++){
    if(g_texture_array[i].handle == VK_NULL_HANDLE)return i;
  }
  return MAX_SAMPLERS;
}

int
texture_load(GfxContext context, unsigned char* pixels,
	     int width, int height, int channels, uint32_t* index){
  
  *index = textures_get_slot();
  GfxImage* texture = &g_texture_array[*index];
  
  VkFormat format;
  switch(channels){
  case 4:
    format = VK_FORMAT_R8G8B8A8_SRGB;
    break;
  case 3:
    format = VK_FORMAT_R8G8B8_SRGB;
    break;
  case 1:
    format = VK_FORMAT_R8_UNORM;
    break;
  default:
    return 2;
  }
  
  VkDeviceSize image_size = width * height * channels;
  GfxBuffer image_b;
  buffer_create(context, &image_b,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
		image_size);
  
  
  /* copy pixel data to buffer */
  vmaCopyMemoryToAllocation(context.allocator,
			    pixels,
			    image_b.allocation, 0,
			    image_size);
  //memcpy(image_b.first_ptr, pixels, image_size);

  image_create(context, &texture->handle, &texture->allocation,
	       VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
	       format, width, height);


  /* Copy the image buffer to a VkImage proper */
  transition_image_layout(context, texture->handle,
			  VK_IMAGE_LAYOUT_UNDEFINED,
			  VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);


  copy_buffer_to_image(context, image_b.handle, texture->handle,
		       (uint32_t)width, (uint32_t)height);
  
  buffer_destroy(context, &image_b, 1);

  transition_image_layout(context, texture->handle,
			  VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			  VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
  
  /* Create image view */
  image_view_create(context.l_dev, texture->handle,
		    &texture->view, format,
		    VK_IMAGE_ASPECT_COLOR_BIT);
  sampler_create(context, &texture->sampler);

  return 0;
}

int texture_file_load(GfxContext context, const char* filename, uint32_t* index)
{
  int x, y, n;
  stbi_info(filename, &x, &y, &n);
  unsigned char* pixels = stbi_load(filename, &x, &y, &n, n);
  if(pixels == NULL){
    printf("stbi_load() fail\n");
    return 1;
  }
  
  texture_load(context, pixels, x, y, n, index);
  return 0;
}

int texture_memory_load(GfxContext context, uint8_t* raw_pixels, size_t size,
			uint32_t* index)
{
  
  int width, height, channels;
  stbi_info_from_memory(raw_pixels, size,
			&width, &height, &channels);
  /* interpret view file */
  unsigned char* pixels = stbi_load_from_memory
    ((stbi_uc*)raw_pixels, size,
     &width, &height, &channels, channels);
  if(pixels == NULL) {
    printf("stbi_load_from_memory() failed\n");
    return 1;
  }
  
  texture_load(context, pixels, width, height, channels, index);
  return 0;
} 

int
texture_descriptors_update(GfxContext context, uint32_t count)
{
  VkDescriptorImageInfo infos[count];
  VkWriteDescriptorSet writes[count];

  for(uint32_t i = 0; i < count; i++){

    if(g_texture_array[i].handle == VK_NULL_HANDLE)printf("texture loading error\n");

    infos[i] = (VkDescriptorImageInfo){
      .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
      .imageView = g_texture_array[i].view,
      .sampler = g_texture_array[i].sampler,
    };
    
    writes[i] = (VkWriteDescriptorSet){
      .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
      .dstSet = context.texture_descriptors,
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

int textures_bind(GfxContext context, GfxShader shader){
      
  vkCmdBindDescriptorSets(context.command_buffer, 
			  VK_PIPELINE_BIND_POINT_GRAPHICS,
			  shader.pipeline_layout, 0, 1,
			  &context.texture_descriptors, 0, NULL);

  return 0;
}

int textures_free(GfxContext context){
  
  for(uint32_t i = 0; i < MAX_SAMPLERS; i++){
    if(g_texture_array[i].handle != VK_NULL_HANDLE){
      image_destroy(context, g_texture_array[i]);
    }
  }
  return 0;
}
