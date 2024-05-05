#include "drawing.h"

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



