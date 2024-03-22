#include "solid.h"

VkCommandBuffer
command_single_begin(GfxContext context)
{
  VkCommandBufferAllocateInfo alloc_info = {
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
    .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
    .commandPool = context.command_pool,
    .commandBufferCount = 1,
  };

  VkCommandBuffer command_buffer;
  vkAllocateCommandBuffers(context.l_dev, &alloc_info, &command_buffer);

  VkCommandBufferBeginInfo begin_info = {
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
    .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
  };

  vkBeginCommandBuffer(command_buffer, &begin_info);

  return command_buffer;
}

int
command_single_end(GfxContext context, VkCommandBuffer *command_buffer)
{
  vkEndCommandBuffer(*command_buffer);

  VkSubmitInfo submit_info = {
    .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
    .commandBufferCount = 1,
    .pCommandBuffers = command_buffer,
  };

  if(vkQueueSubmit(context.queue, 1, &submit_info, VK_NULL_HANDLE)
     != VK_SUCCESS) return 1;
  
  vkQueueWaitIdle(context.queue);
  vkFreeCommandBuffers(context.l_dev, context.command_pool, 1, command_buffer);
  return 0;
}

uint32_t
draw_start(GfxContext *context, GfxPipeline gfx, GeometryData geo)
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

  // tricky formattting to not give warnings
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

  VkBuffer vertex_buffers[] = {geo.vertices.handle};
  VkDeviceSize offsets[] = {0};

  vkCmdBindVertexBuffers(gfx.command_buffer, 0, 1, vertex_buffers, offsets);
  vkCmdBindIndexBuffer(gfx.command_buffer, geo.indices.handle,
		       0, VK_INDEX_TYPE_UINT32);

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

int
indirect_buffer_create(GfxContext context, int count,
		       DeviceBuffer *buffer)
{
  buffer_create(context,
		count * sizeof(VkDrawIndexedIndirectCommand),
		VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT, buffer
		);
		
  return 0;
}

int
indirect_buffer_update(DeviceBuffer buffer, Entity *entities)
{
  int count = buffer.size / sizeof(VkDrawIndexedIndirectCommand);
  
  for(int i = 0; i < count; i++){
    VkDrawIndexedIndirectCommand* data_ptr =
      ((VkDrawIndexedIndirectCommand*)buffer.first_index)+i;
    data_ptr->indexCount = entities[i].indices_len;
    data_ptr->instanceCount = 1;
    data_ptr->firstIndex = entities[i].first_index;
    data_ptr->vertexOffset = entities[i].vertex_offset;
    data_ptr->firstInstance = i;
  }
  return 0;
}

void
draw_args_update(Camera cam, Entity *entities,
		 DeviceBuffer buffer)
{
  // every object in the entity list
  int count = buffer.size / sizeof(drawArgs);
  for(int i = 0; i < count; i++){
    drawArgs *args_ptr = ((drawArgs*)buffer.first_index)+i;
    args_ptr->texture_index = entities[i].texture_index;
    fps_mvp_calc(cam, entities[i],
		  &args_ptr->transform);
  }
 
}

void
draw_args_create(GfxContext context, GfxPipeline pipeline,
		  int count, DeviceBuffer *buffers)
{
  VkDescriptorBufferInfo buffer_info[context.frame_c];
  VkWriteDescriptorSet descriptor_write[context.frame_c];
  
  for(uint32_t i = 0; i < context.frame_c; i++){
    buffer_create(context,
		  count * sizeof(drawArgs),
		  (VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT),
		  &buffers[i]
		  );

    buffer_info[i] = (VkDescriptorBufferInfo){
      .buffer = buffers[i].handle,
      .offset = 0,
      .range = buffers[i].size,
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




