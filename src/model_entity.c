#include "model.h"

int
mvp_calc(Camera cam, vec3 pos, mat4 *dest)
{
  /* This function calculates the model view projection matrix
   * doing m*vector so the order is (p*(v*(m*vector)))
   * so the pre multiplied matrix is p * (v*m)
   * vm * p is not the same as `p * vm`
   */

  mat4 model;
  glm_mat4_identity(model);
  glm_translate(model, pos);

  mat4 view;
  glm_look(cam.pos, cam.front, cam.up,
	     view);

  mat4 vm; // view*model
  glm_mat4_mul(view, model, vm);
  
  mat4 projection;
  glm_perspective(cam.fov, cam.aspect_ratio,
		  cam.near, cam.far, projection);
  projection[1][1] *= -1;

  glm_mat4_mul(projection, vm, *dest);
  return 0;
}


void
draw_args_update(Camera cam, Entity *entities,
		 GfxBuffer buffer)
{
  /* in the shader code we use the struct drawArgs
   * uint32_t texture_index;
   * mat4 transform;
   */

  // every object in the entity list
  int c = buffer.used_size / sizeof(drawArgs);
  for(int i = 0; i < c; i++){
    drawArgs *args_ptr = ((drawArgs*)buffer.first_ptr) +i;
    args_ptr->texture_index = entities[i].texture_index;
    mvp_calc(cam, entities[i].pos, &args_ptr->transform);
  }
 
}

void
draw_args_create(GfxContext context, GfxPipeline pipeline,
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
      .dstSet = pipeline.rapid_sets[i],
      .dstBinding = 0,
      .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
      .descriptorCount = 1,
      .pBufferInfo = &buffer_info[i],
    };
  }
  vkUpdateDescriptorSets(context.l_dev, context.frame_c,
			 descriptor_write, 0, NULL);
}

/* draw_indirect_update and draw_args_update  write directly
 * into the buffers memory, instead of creating a block of data
 * to copy over into the target buffer. This may be more efficient.
 */
int
draw_indirect_update(GfxBuffer buffer, Entity *entities)
{
  size_t stride = sizeof(VkDrawIndexedIndirectCommand);
  int count = buffer.used_size / stride;
  
  for(int i = 0; i < count; i++){
    VkDrawIndexedIndirectCommand* data_ptr =
      ((VkDrawIndexedIndirectCommand*)buffer.first_ptr) +i;
    //VkDrawIndexedIndirectCommand *data_ptr = buffer_ptr(buffer, i * stride);
    data_ptr->indexCount = entities[i].indices_len;
    data_ptr->instanceCount = 1;
    data_ptr->firstIndex = entities[i].first_index;
    data_ptr->vertexOffset = entities[i].vertex_offset;
    data_ptr->firstInstance = i;
  }
  return 0;
}

Entity
entity_duplicate(Entity mother, float x, float y, float z){

  Entity child;
  glm_vec3_copy( (vec3){x, y, z}, child.pos);
  child.indices_len = mother.indices_len;
  child.first_index = mother.first_index;
  child.vertex_offset = mother.vertex_offset;
  child.texture_index = mother.texture_index;
  return child;
}
