#include "drawing.h"

int
indirect_b_create(GfxContext context, GfxShader* shader, size_t draw_count)
{
  shader->uniform_b = (GfxBuffer*)malloc(sizeof(GfxBuffer) * context.frame_c);
  
  for(unsigned int i = 0; i < context.frame_c; i++){

    GfxBuffer* b = &shader->uniform_b[i];
    VkDeviceSize size = draw_count * sizeof(drawArgs);
    
    buffer_create(context, b,
		  (VK_BUFFER_USAGE_STORAGE_BUFFER_BIT  
		   | VK_BUFFER_USAGE_TRANSFER_DST_BIT), // usage
		  (VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
		   | VMA_ALLOCATION_CREATE_MAPPED_BIT ), // flags
		  size);
		 
    VkDescriptorBufferInfo descriptor_buffer_info = {
      .buffer = b->handle,
      .offset = 0,
      .range = size,
    };    
    VkWriteDescriptorSet descriptor_write = {
      .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
      .dstSet = shader->descriptors[i],
      .dstBinding = 0,
      .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
      .descriptorCount = 1,
      .pBufferInfo = &descriptor_buffer_info,
    };
  
    vkUpdateDescriptorSets(context.l_dev, 1, &descriptor_write, 0, NULL);
  }

  buffer_create(context, &shader->indirect_b,
		VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT,
		(VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
		 | VMA_ALLOCATION_CREATE_MAPPED_BIT ),
		draw_count * sizeof(VkDrawIndexedIndirectCommand));

  return 0;
}

int
draw_start(GfxContext* context)
{
  VkFence fences[] = { context->in_flight };
  vkWaitForFences(context->l_dev, 1, fences, VK_TRUE, UINT64_MAX);
  vkResetFences(context->l_dev, 1, fences);
  
  vkAcquireNextImageKHR(context->l_dev, context->swapchain_handle, UINT64_MAX,
			context->image_available, VK_NULL_HANDLE,
			&context->current_frame_index);
  vkResetCommandBuffer(context->command_buffer, 0);

  VkCommandBufferBeginInfo begin_info = {
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
    .flags = 0,
    .pInheritanceInfo = NULL,
  };

  if(vkBeginCommandBuffer(context->command_buffer, &begin_info) != VK_SUCCESS){
    printf("!failed to begin recording command buffer!\n");
  }

  VkClearValue clears[2];
  clears[0].color = (VkClearColorValue){ { 0.0f, 0.0f, 0.0f, 1.0f } };
  clears[1].depthStencil = (VkClearDepthStencilValue){ 1.0f, 0 };
 
  VkRenderPassBeginInfo render_pass_info = {
    .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
    .renderPass = context->render_pass,
    .framebuffer = context->framebuffers[context->current_frame_index],
    .renderArea.offset = {0, 0},
    .renderArea.extent = context->extent,
    .clearValueCount = 2,
    .pClearValues = clears,
  };
    
  vkCmdBeginRenderPass(context->command_buffer, &render_pass_info,
		       VK_SUBPASS_CONTENTS_INLINE);
	       
  return 0;
}

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
model_matrix(Entity entity, mat4 *dest)
{
  mat4 model;
  glm_mat4_identity(model);
  glm_translate(model, entity.pos);
  
  mat4 model_rotate;
  glm_quat_mat4(entity.rotate, model_rotate);
  glm_mat4_mul(model, model_rotate, model); 
  
  glm_scale(model, (vec3){1.0f,1.0f,1.0f});
  glm_mat4_copy(model, *dest);
  
  return 0;
}

void draw_entities(GfxContext context, GfxShader shader, Camera cam, Entity* entities, int count){
  mat4 view;
  glm_look(cam.pos, cam.front, cam.up, view);

  VmaAllocationInfo indirect;
  vmaGetAllocationInfo(context.allocator,
		       shader.indirect_b.allocation,
		       &indirect);
  
  VmaAllocationInfo indirect_args;
  vmaGetAllocationInfo(context.allocator,
		       shader.uniform_b[context.current_frame_index].allocation,
		       &indirect_args);
  
  for(int i = 0; i < count; i++){
    Entity* entity = &entities[i];
    // INDIRECT ARGS BUFFER
    drawArgs *args_ptr = (drawArgs*)indirect_args.pMappedData + i;
    args_ptr->texIndex = entity->model.textureIndex;
    
    mat4 model;
    model_matrix(*entity, &model);
  
    // MVP matrix
    mat4 vm;
    glm_mat4_mul(view, model, vm);
    glm_mat4_mul(cam.projection, vm, args_ptr->mvp);
    
    // Lighting Matrix
    glm_mat4_inv(model, args_ptr->rotate_m);
    glm_mat4_transpose(args_ptr->rotate_m);

    // INDIRECT BUFFER
    VkDrawIndexedIndirectCommand* indirect_ptr =
      (VkDrawIndexedIndirectCommand*)indirect.pMappedData + i;
    indirect_ptr->indexCount = entity->model.indexCount;
    indirect_ptr->instanceCount = 1;
    indirect_ptr->firstIndex = entity->model.firstIndex;
    indirect_ptr->vertexOffset = entity->model.vertexOffset;
    indirect_ptr->firstInstance = i;
  }

  vkCmdDrawIndexedIndirect(context.command_buffer, shader.indirect_b.handle, 0,
			   count, sizeof(VkDrawIndexedIndirectCommand));
}

int
draw_end(GfxContext context)
{

  vkCmdEndRenderPass(context.command_buffer);
  if(vkEndCommandBuffer(context.command_buffer) != VK_SUCCESS) {
    printf("!failed to record command buffer!");
    return 1;
  }

  VkSemaphore wait_semaphores[] = { context.image_available };
  VkPipelineStageFlags wait_stages[] =
    {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

  VkSemaphore signal_semaphores[] = { context.render_finished };
  VkCommandBuffer command_buffers[] = { context.command_buffer };

  VkSubmitInfo submit_info = {
    .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
    .waitSemaphoreCount = 1,
    .pWaitSemaphores = wait_semaphores,
    .pWaitDstStageMask = wait_stages,
    .commandBufferCount = 1,
    .pCommandBuffers = command_buffers,
    .signalSemaphoreCount = 1,
    .pSignalSemaphores = signal_semaphores,
  };

  if(vkQueueSubmit(context.queue, 1, &submit_info, context.in_flight)
     != VK_SUCCESS) {
    printf("!failed to submit draw command buffer!\n");
    return 1;
  }
  
  VkSwapchainKHR swapchains[] = {context.swapchain_handle};
  
  VkPresentInfoKHR present_info = {
    .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
    .waitSemaphoreCount = 1,
    .pWaitSemaphores = signal_semaphores,
    .swapchainCount = 1,
    .pSwapchains = swapchains,
    .pImageIndices = &context.current_frame_index,
    .pResults = NULL
  };
   
  VkResult results = vkQueuePresentKHR(context.queue, &present_info);
  if(results != VK_SUCCESS) {
    return 1;
  }
  
  return 0;
}



