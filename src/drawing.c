#include "drawing.h"

static int CURRENT_INDIRECT_INDEX;
static GfxBuffer INDIRECT_BUFFER;
static GfxBuffer* INDIRECT_ARGS_BUFFERS;

int
draw_descriptors_update(GfxContext context, GfxResources resources, int count)
{
  VkDescriptorBufferInfo buffer_info;
  VkWriteDescriptorSet descriptor_write;

  INDIRECT_ARGS_BUFFERS = (GfxBuffer*)malloc(sizeof(GfxBuffer) * context.frame_c);

  for(unsigned int i = 0; i < context.frame_c; i++){
    buffer_create(context,
		  count * sizeof(drawArgs),
		  (VK_BUFFER_USAGE_STORAGE_BUFFER_BIT
		   | VK_BUFFER_USAGE_TRANSFER_DST_BIT),
		  &INDIRECT_ARGS_BUFFERS[i]
		  );
  
    buffer_info = (VkDescriptorBufferInfo){
      .buffer = INDIRECT_ARGS_BUFFERS[i].handle,
      .offset = 0,
      .range = INDIRECT_ARGS_BUFFERS[i].total_size,
    };
    
    descriptor_write = (VkWriteDescriptorSet){
      .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
      .dstSet = resources.rapid_sets[i],
      .dstBinding = 0,
      .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
      .descriptorCount = 1,
      .pBufferInfo = &buffer_info,
    };

    vkUpdateDescriptorSets(context.l_dev, 1, &descriptor_write, 0, NULL);
  }
  
  buffer_create(context,
		count * sizeof(VkDrawIndexedIndirectCommand),
		VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT, &INDIRECT_BUFFER );
  
  return 0;
}

int
draw_start(GfxContext *context, GfxPipeline gfx)
{
  CURRENT_INDIRECT_INDEX = 0;
  
  VkFence fences[] = { gfx.in_flight };
  vkWaitForFences(context->l_dev, 1, fences, VK_TRUE, UINT64_MAX);
  vkResetFences(context->l_dev, 1, fences);
  
  vkAcquireNextImageKHR(context->l_dev, context->swapchain_handle, UINT64_MAX,
			gfx.image_available, VK_NULL_HANDLE,
			&context->current_frame_index);
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
    .framebuffer = gfx.framebuffers[context->current_frame_index],
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

void render_entity(GfxContext context, Camera cam, Entity entity){
  mat4 view;
  glm_look(cam.pos, cam.front, cam.up, view);

  // INDIRECT ARGS BUFFER
  drawArgs *args_ptr =
    ((drawArgs*)INDIRECT_ARGS_BUFFERS[context.current_frame_index].first_ptr)
    + CURRENT_INDIRECT_INDEX;
  args_ptr->texIndex = entity.model.textureIndex;
  
  mat4 model;
  model_matrix(entity, &model);
  
  // MVP matrix
  mat4 vm;
  glm_mat4_mul(view, model, vm);
  glm_mat4_mul(cam.projection, vm, args_ptr->mvp);
  
  // Lighting Matrix
  glm_mat4_inv(model, args_ptr->rotate_m);
  glm_mat4_transpose(args_ptr->rotate_m);

  // INDIRECT BUFFER
  VkDrawIndexedIndirectCommand* indirect_ptr =
    ((VkDrawIndexedIndirectCommand*)INDIRECT_BUFFER.first_ptr)
    +CURRENT_INDIRECT_INDEX;
  indirect_ptr->indexCount = entity.model.indexCount;
  indirect_ptr->instanceCount = 1;
  indirect_ptr->firstIndex = entity.model.firstIndex;
  indirect_ptr->vertexOffset = entity.model.vertexOffset;
  indirect_ptr->firstInstance = CURRENT_INDIRECT_INDEX;
  
  CURRENT_INDIRECT_INDEX += 1;
}

int
draw_end(GfxContext context, GfxPipeline pipeline, int entity_c)
{
  vkCmdDrawIndexedIndirect(pipeline.command_buffer, INDIRECT_BUFFER.handle, 0,
			   entity_c, sizeof(VkDrawIndexedIndirectCommand));
  
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
    .pImageIndices = &context.current_frame_index,
    .pResults = NULL
  };
   
  VkResult results = vkQueuePresentKHR(context.queue, &present_info);
  if(results != VK_SUCCESS) {
    return 1;
  }
  
  return 0;
}

int draw_descriptors_free(GfxContext context){
  buffers_destroy(context, INDIRECT_ARGS_BUFFERS, context.frame_c);
  buffers_destroy(context, &INDIRECT_BUFFER, 1);

  return 0;
  
}



