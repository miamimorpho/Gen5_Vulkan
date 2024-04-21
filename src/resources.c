#include "resources.h"


Entity
entity_add1(GfxModelOffsets model, float x, float y, float z){

  Entity dest;
  glm_vec3_copy( (vec3){x, y, z}, dest.pos);
  glm_quat(dest.rotate, 0.0f, 0.0f, 0.0f, -1.0f);
  
  dest.model.indexCount = model.indexCount;
  dest.model.firstIndex = model.firstIndex;
  dest.model.vertexOffset = model.vertexOffset;
  dest.model.textureIndex = model.textureIndex;
  return dest;
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

  if(vkAllocateDescriptorSets(l_dev, &alloc_info, &resources->slow_set)
     != VK_SUCCESS) return 1;

  return 0;
}


int
texture_descriptors_update(GfxContext context, uint32_t count,
			GfxResources resources)
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
      .imageView = resources.textures[i].view,
      .sampler = resources.textures[i].sampler,
    };
    
    writes[i] = (VkWriteDescriptorSet){
      .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
      .dstSet = resources.slow_set,
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

void
draw_descriptors_create(GfxContext context, GfxResources resources,
		  int count, GfxBuffer *buffers)
{
  VkDescriptorBufferInfo buffer_info[context.frame_c];
  VkWriteDescriptorSet descriptor_write[context.frame_c];

  VkDeviceSize delta_size = count * sizeof(drawArgs);
  
  for(uint32_t i = 0; i < context.frame_c; i++){
    buffer_create(context,
		  delta_size,
		  (VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT),
		  &buffers[i]
		  );
    buffers[i].used_size += delta_size;
    
    buffer_info[i] = (VkDescriptorBufferInfo){
      .buffer = buffers[i].handle,
      .offset = 0,
      .range = buffers[i].total_size,
    };

    descriptor_write[i] = (VkWriteDescriptorSet){
      .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
      .dstSet = resources.rapid_sets[i],
      .dstBinding = 0,
      .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
      .descriptorCount = 1,
      .pBufferInfo = &buffer_info[i],
    };
  }
  vkUpdateDescriptorSets(context.l_dev, context.frame_c,
			 descriptor_write, 0, NULL);
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

int resources_destroy(GfxContext context, GfxResources resources){

  for(uint32_t i = 0; i < MAX_SAMPLERS; i++){
    if(resources.textures[i].handle != VK_NULL_HANDLE){
      image_destroy(context, &resources.textures[i]);
      
    }
  }
  
  buffers_destroy(context, (GfxBuffer*)resources.geometry.p_next, 1);
  buffers_destroy(context, &resources.geometry, 1);
  
  vkDestroyDescriptorPool(context.l_dev, resources.descriptor_pool, NULL);
  vkDestroyDescriptorSetLayout(context.l_dev, resources.rapid_layout, NULL);
  free(resources.rapid_sets);
  vkDestroyDescriptorSetLayout(context.l_dev, resources.slow_layout, NULL);

  return 0;
 
}
