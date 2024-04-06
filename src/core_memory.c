#include "core.h"
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






