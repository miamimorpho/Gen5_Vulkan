#include "vulkan_public.h"
#include "vulkan_util.h"
#include <string.h>

void
buffers_destroy(GfxContext context, GfxBuffer *buffers, int count)
{
  for(int i = 0; i < count; i++){
    vkUnmapMemory(context.l_dev, buffers[i].memory);
    vkDestroyBuffer(context.l_dev, buffers[i].handle, NULL);
    vkFreeMemory(context.l_dev, buffers[i].memory, NULL);
  }
}

int
buffer_append(const void *data, GfxBuffer *dest, size_t size)
{
  if(dest->total_size < dest->used_size + size ){
    printf("out of  memory!\n");
    return 1;
  }

  void *write = ((uint8_t*)dest->first_ptr) + dest->used_size;
  memcpy(write, data, size);
  dest->used_size += size;
  return 0;
}

/**
 * A generalised way to allocate memory for vertex buffers
 * 'properties' defines the type of memory we want to use
 * memory allocating
 *
 * size is unit count * sizeof(unit)
 */
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
 
  return err;
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

int
rapid_descriptors_alloc(GfxContext context, GfxPipeline *pipeline)
{

  VkDescriptorSetLayout layouts[context.frame_c];
  for(uint32_t i = 0; i < context.frame_c; i++)
    layouts[i] = pipeline->rapid_layout;
  
  VkDescriptorSetAllocateInfo alloc_info = {
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
    .descriptorPool = pipeline->descriptor_pool,
    .descriptorSetCount = context.frame_c,
    .pSetLayouts = layouts,
  };
  pipeline->rapid_sets = malloc(context.frame_c * sizeof( VkDescriptorSet ));
  if(vkAllocateDescriptorSets(context.l_dev, &alloc_info, pipeline->rapid_sets)
     != VK_SUCCESS) return 1;

  return 0;
}
