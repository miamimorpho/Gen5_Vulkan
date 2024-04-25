#include "resources.h"

int
geometry_buffer_create(GfxContext context, GfxBuffer* geometry,
		       size_t estimated_size)
{
  int err;

  GfxBuffer* vertices = (GfxBuffer*)malloc(sizeof(GfxBuffer));
  GfxBuffer* indices = (GfxBuffer*)malloc(sizeof(GfxBuffer));

  err = buffer_create(context,
		estimated_size * sizeof(vertex),
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		vertices);

  err = buffer_create(context,
	        estimated_size * sizeof(uint32_t) * 2,
		VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		indices);

  *geometry = *vertices;
  geometry->p_next = (void*)indices;

  return err;
}

int
geometry_load(const vertex* vertices, uint32_t vertex_c,
	      const uint32_t* indices, uint32_t index_c,
	      GfxBuffer* dest, GfxModelOffsets* model){
  
  GfxBuffer* dest_indices = (GfxBuffer*)dest->p_next;

  // we need to give offsets before we append to the geometry buffer
  model->firstIndex = dest_indices->used_size / sizeof(uint32_t);
  model->indexCount = index_c;
  model->vertexOffset = dest->used_size / sizeof(vertex);
  model->textureIndex = 0;

  int err;
  err = buffer_append(vertices, dest, vertex_c * sizeof(vertex));
  err = buffer_append(indices, dest_indices, index_c * sizeof(uint32_t));
 
  return err;
}

int
descriptor_pool_create(GfxContext context, VkDescriptorPool* descriptor_pool)
{

  VkDescriptorPoolSize pool_sizes[2];
  pool_sizes[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  pool_sizes[0].descriptorCount = MAX_SAMPLERS;
  
  pool_sizes[1].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  pool_sizes[1].descriptorCount = context.frame_c;
 

  VkDescriptorPoolCreateInfo pool_info = {
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
    .poolSizeCount = 2,
    .pPoolSizes = pool_sizes,
    .maxSets = MAX_SAMPLERS + context.frame_c,
    .flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT_EXT,
  };

  if(vkCreateDescriptorPool(context.l_dev, &pool_info, NULL, descriptor_pool)
     != VK_SUCCESS) {
    printf("failed to create descriptor pool\n");
    return 1;
  }
  return 0;
}

int
descriptor_set_layouts_create(VkDevice l_dev, GfxResources *resources)
{
  /* Texture */
   VkDescriptorSetLayoutBinding sampler_binding = {
    .binding = 0,
    .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
    .descriptorCount = MAX_SAMPLERS,
    .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
  };

  VkDescriptorBindingFlags bindless_flags =
    VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT_EXT
    | VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT_EXT
    | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT_EXT;
 
  VkDescriptorSetLayoutBindingFlagsCreateInfo sampler_info2 = {
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO,
    .bindingCount = 1,
    .pBindingFlags = &bindless_flags,
  };
  
  VkDescriptorSetLayoutCreateInfo sampler_info = {
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
    .bindingCount = 1,
    .pBindings = &sampler_binding,
    .flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT_EXT,
    .pNext = &sampler_info2,
  };
  
  if(vkCreateDescriptorSetLayout(l_dev, &sampler_info, NULL,
				 &resources->slow_layout) != VK_SUCCESS) return 1;

  /* Indirect draw call buffer */
  VkDescriptorSetLayoutBinding transform_binding = {
    .binding = 0,
    .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
    .descriptorCount = 1,
    .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
  };

  VkDescriptorSetLayoutCreateInfo transform_layout_info = {
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
    .bindingCount = 1,
    .pBindings = &transform_binding
  };

  if(vkCreateDescriptorSetLayout(l_dev, &transform_layout_info, NULL,
				 &resources->rapid_layout) != VK_SUCCESS) return 1;

  return 0;
}

int
texture_descriptors_alloc(VkDevice l_dev, GfxResources *resources)
{
  
  for(int i = 0; i < MAX_SAMPLERS; i++){
    resources->textures[i].view = VK_NULL_HANDLE;
  }
  
  uint32_t max_binding = MAX_SAMPLERS -1;
  VkDescriptorSetVariableDescriptorCountAllocateInfo count_info = {
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO_EXT,
    .descriptorSetCount = 1,
    .pDescriptorCounts = &max_binding,
  };
  
  /* Allocate descriptor memory */
  VkDescriptorSetAllocateInfo alloc_info = {
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
    .descriptorPool = resources->descriptor_pool,
    .descriptorSetCount = 1,
    .pSetLayouts = &resources->slow_layout,
    .pNext = &count_info,
  };

  if(vkAllocateDescriptorSets(l_dev, &alloc_info, &resources->slow_sets)
     != VK_SUCCESS) return 1;

  return 0;
}

int texture_load(GfxContext context, unsigned char* pixels, ImageData* texture,
		 int width, int height, int channels){

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
  buffer_create(context, image_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		&image_b);

  /* copy pixel data to buffer */
  memcpy(image_b.first_ptr, pixels, image_size);
 
  image_create(context, &texture->handle, &texture->memory,
	       format,
	       VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
	       width, height);

  /* Copy the image buffer to a VkImage proper */
  transition_image_layout(context, texture->handle,
			  VK_IMAGE_LAYOUT_UNDEFINED,
			  VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

  copy_buffer_to_image(context, image_b.handle, texture->handle,
		       (uint32_t)width, (uint32_t)height);

  transition_image_layout(context, texture->handle,
			  VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			  VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

  buffers_destroy(context, &image_b, 1);

 
  
  /* Create image view */
  image_view_create(context.l_dev, texture->handle,
		    &texture->view, format,
		    VK_IMAGE_ASPECT_COLOR_BIT);
  sampler_create(context, &texture->sampler);

  return 0;
}


int textures_slot(GfxResources resources){
  for(uint32_t i = 0; i < MAX_SAMPLERS; i++){
    if(resources.textures[i].handle == VK_NULL_HANDLE){
      printf("texture slot: [%d]\n", i);
      return i;
    }
  }
  return MAX_SAMPLERS;
}

int
texture_descriptors_update(GfxContext context, GfxResources resources, uint32_t count)
{
  VkDescriptorImageInfo infos[count];
  VkWriteDescriptorSet writes[count];
  
  for(uint32_t i = 0; i < count; i++){
  
    infos[i] = (VkDescriptorImageInfo){
      .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
      .imageView = resources.textures[i].view,
      .sampler = resources.textures[i].sampler,
    };
    
    writes[i] = (VkWriteDescriptorSet){
      .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
      .dstSet = resources.slow_sets,
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

int
draw_descriptors_alloc(GfxContext context, GfxResources *resources)
{

  VkDescriptorSetLayout layouts[context.frame_c];
  for(uint32_t i = 0; i < context.frame_c; i++)
    layouts[i] = resources->rapid_layout;
  
  VkDescriptorSetAllocateInfo alloc_info = {
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
    .descriptorPool = resources->descriptor_pool,
    .descriptorSetCount = context.frame_c,
    .pSetLayouts = layouts,
  };
  resources->rapid_sets = malloc(context.frame_c * sizeof( VkDescriptorSet ));
  if(vkAllocateDescriptorSets(context.l_dev, &alloc_info, resources->rapid_sets)
     != VK_SUCCESS) return 1;

  return 0;
}

int geometry_bind(GfxPipeline pipeline, GfxBuffer geometry){

  VkBuffer vertex_buffers[] = {geometry.handle};
  VkDeviceSize offsets[] = {0};
  vkCmdBindVertexBuffers(pipeline.command_buffer, 0, 1, vertex_buffers, offsets);

  GfxBuffer *indices = (GfxBuffer*)geometry.p_next;
  vkCmdBindIndexBuffer(pipeline.command_buffer, indices->handle,
		       0, VK_INDEX_TYPE_UINT32);

}

int resources_bind(GfxContext context, GfxPipeline pipeline,
		   GfxResources resources)
{
  vkCmdBindDescriptorSets(pipeline.command_buffer, 
			  VK_PIPELINE_BIND_POINT_GRAPHICS,
			  pipeline.layout, 0, 1,
			  &resources.slow_sets, 0, NULL);
  
  vkCmdBindDescriptorSets(pipeline.command_buffer,
			  VK_PIPELINE_BIND_POINT_GRAPHICS,
			  pipeline.layout, 1, 1,
			  &resources.rapid_sets[context.current_frame_index],
			  0, NULL);

  return 0;
}

int resources_init(GfxContext context, GfxResources* resources){

  const int VERTEX_COUNT = 100000;
  
  GfxBuffer *vertices = (GfxBuffer*)malloc(sizeof(GfxBuffer));
  GfxBuffer *indices = (GfxBuffer*)malloc(sizeof(GfxBuffer));

  //GfxBuffer *vertices;
  buffer_create(context,
		VERTEX_COUNT * sizeof(vertex),
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		vertices);

  //GfxBuffer *indices;
  buffer_create(context,
		VERTEX_COUNT * sizeof(uint32_t),
		VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		indices);

  resources->geometry = *vertices;
  resources->geometry.p_next = (void*)indices;
    
  for(uint32_t i = 0; i < MAX_SAMPLERS; i++){
    resources->textures[i].handle = VK_NULL_HANDLE;
  }

  descriptor_pool_create(context, &resources->descriptor_pool);
  descriptor_set_layouts_create(context.l_dev, resources);
  texture_descriptors_alloc(context.l_dev, resources);
  draw_descriptors_alloc(context, resources);
  
  return 0;
}

/*
int unused_textures_free(GfxContext context, Entity *entities, uint32_t count,
			 ImageData *textures){

  for(uint32_t i = 0; i < count; i++){
    int ti = entities[i].model.textureIndex;
    if(textures[ti].handle != VK_NULL_HANDLE){
      printf("destroying textures[%d] !\n", ti);
      image_destroy(context, &textures[ti] );
      textures[ti].handle = VK_NULL_HANDLE;
    }
  }
  return 0;
}
*/

int resources_destroy(GfxContext context, GfxResources resources){

  for(uint32_t i = 0; i < MAX_SAMPLERS; i++){
    if(resources.textures[i].handle != VK_NULL_HANDLE){
      image_destroy(context, &resources.textures[i]);
      
    }
  }
  
  buffers_destroy(context, &resources.geometry, 1);

  vkDestroyDescriptorPool(context.l_dev, resources.descriptor_pool, NULL);
  vkDestroyDescriptorSetLayout(context.l_dev, resources.rapid_layout, NULL);
  free(resources.rapid_sets);
  vkDestroyDescriptorSetLayout(context.l_dev, resources.slow_layout, NULL);

  return 0;
 
}
