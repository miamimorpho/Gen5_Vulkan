#include "drawing.h"

int
draw_start(GfxContext *context, GfxPipeline gfx)
{
  VkFence fences[] = { gfx.in_flight };
  vkWaitForFences(context->l_dev, 1, fences, VK_TRUE, UINT64_MAX);
  vkResetFences(context->l_dev, 1, fences);
  
  vkAcquireNextImageKHR(context->l_dev, context->swapchain_handle, UINT64_MAX,
			gfx.image_available, VK_NULL_HANDLE,
			&context->frame_index);
  vkResetCommandBuffer(gfx.command_buffer, 0);

  VkCommandBufferBeginInfo begin_info = {
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
    .flags = 0,
    .pInheritanceInfo = NULL,
  };

  if(vkBeginCommandBuffer(gfx.command_buffer, &begin_info) != VK_SUCCESS){
    printf("!failed to begin recording command buffer!\n");
  }

  VkClearValue clears[2];
  clears[0].color = (VkClearColorValue){ { 0.0f, 0.0f, 0.0f, 1.0f } };
  clears[1].depthStencil = (VkClearDepthStencilValue){ 1.0f, 0 };
 
  VkRenderPassBeginInfo render_pass_info = {
    .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
    .renderPass = gfx.render_pass,
    .framebuffer = gfx.framebuffers[context->frame_index],
    .renderArea.offset = {0, 0},
    .renderArea.extent = context->extent,
    .clearValueCount = 2,
    .pClearValues = clears,
  };
    
  vkCmdBeginRenderPass(gfx.command_buffer, &render_pass_info,
		       VK_SUBPASS_CONTENTS_INLINE);

  VkViewport viewport = {
    .x = 0.0f,
    .y = 0.0f,
    .width = context->extent.width,
    .height = context->extent.height,
    .minDepth = 0.0f,
    .maxDepth = 1.0f,
  };
  vkCmdSetViewport(gfx.command_buffer, 0, 1, &viewport);

  VkRect2D scissor = {
    .offset = {0, 0},
    .extent = context->extent,
  };
    
  vkCmdSetScissor(gfx.command_buffer, 0, 1, &scissor);
  vkCmdBindPipeline(gfx.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
	            gfx.pipeline);

  /*
  VkBuffer vertex_buffers[] = {geo.vertices->handle};
  VkDeviceSize offsets[] = {0};

  vkCmdBindVertexBuffers(gfx.command_buffer, 0, 1, vertex_buffers, offsets);
  vkCmdBindIndexBuffer(gfx.command_buffer, geo.indices->handle,
		       0, VK_INDEX_TYPE_UINT32);
  */
		       
  return 0;
}

/*
 *  view * model
 *  projection * vm
 */

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

void
draw_descriptors_update(Camera cam, Entity *entities,
		 GfxBuffer buffer)
{
  mat4 view;
  glm_look(cam.pos, cam.front, cam.up, view); //view
  
  // every object in the entity list
  int c = buffer.used_size / sizeof(drawArgs);
  for(int i = 0; i < c; i++){
    // get pointer from stride
    drawArgs *args_ptr = ((drawArgs*)buffer.first_ptr) +i;
    args_ptr->texIndex = entities[i].model.textureIndex;
 
    mat4 model;
    model_matrix(entities[i], &model);
  
    // MVP matrix
    mat4 vm;
    glm_mat4_mul(view, model, vm);
    glm_mat4_mul(cam.projection, vm, args_ptr->mvp);
    
    // Lighting Matrix
    glm_mat4_inv(model, args_ptr->rotate_m);
    glm_mat4_transpose(args_ptr->rotate_m);

  }
 
}

int
geometry_buffer_bind(VkCommandBuffer commands, GfxBuffer geometry){
  VkBuffer vertex_buffers[] = {geometry.handle};
  VkDeviceSize offsets[] = {0};

  GfxBuffer *indices = (GfxBuffer*)geometry.p_next;
  vkCmdBindVertexBuffers(commands, 0, 1, vertex_buffers, offsets);
  vkCmdBindIndexBuffer(commands, indices->handle,
		       0, VK_INDEX_TYPE_UINT32);

  return 0;
}

int
draw_indirect_create(GfxContext context, GfxBuffer *buffer, int count)
{
  VkDeviceSize delta = count * sizeof(VkDrawIndexedIndirectCommand);
  
  buffer_create(context, delta,
		VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT, buffer );
  buffer->used_size += delta;
  return 0;
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
    data_ptr->indexCount = entities[i].model.indexCount;
    data_ptr->instanceCount = 1;
    data_ptr->firstIndex = entities[i].model.firstIndex;
    data_ptr->vertexOffset = entities[i].model.vertexOffset;
    data_ptr->firstInstance = i;
  }
  return 0;
}

int
draw_end(GfxContext context, GfxPipeline pipeline)
{

  vkCmdEndRenderPass(pipeline.command_buffer);
  if(vkEndCommandBuffer(pipeline.command_buffer) != VK_SUCCESS) {
    printf("!failed to record command buffer!");
    return 1;
  }

  VkSemaphore wait_semaphores[] = { pipeline.image_available };
  VkPipelineStageFlags wait_stages[] =
    {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

  VkSemaphore signal_semaphores[] = { pipeline.render_finished };
  VkCommandBuffer command_buffers[] = { pipeline.command_buffer };

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

  if(vkQueueSubmit(context.queue, 1, &submit_info, pipeline.in_flight)
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
    .pImageIndices = &context.frame_index,
    .pResults = NULL
  };
   
  VkResult results = vkQueuePresentKHR(context.queue, &present_info);
  if(results != VK_SUCCESS) {
    return 1;
  }
  
  return 0;
}



