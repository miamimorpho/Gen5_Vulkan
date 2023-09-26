#include "solid.h"

static const uint32_t MAX_SAMPLERS = 128;
  

int
depth_format_check(VkPhysicalDevice p_dev, VkFormat *format)
{

  VkFormat depth_format = VK_FORMAT_UNDEFINED;
  VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL;
  VkFormatFeatureFlags features = VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;
  
  VkFormat candidates[3] = {
    //VK_FORMAT_D32_SFLOAT,
    //VK_FORMAT_D32_SFLOAT_S8_UINT,
    //VK_FORMAT_D24_UNORM_S8_UINT,
    VK_FORMAT_D16_UNORM,
  };

  /* cycle through format candidates for suitability */
  for(int i = 0; i < 3; i++){
    VkFormatProperties props;
    vkGetPhysicalDeviceFormatProperties(p_dev, candidates[i], &props);

    if(tiling == VK_IMAGE_TILING_LINEAR
       && (props.linearTilingFeatures & features) == features ){
      depth_format = candidates[i];
      break;
    }
    if (tiling == VK_IMAGE_TILING_OPTIMAL
	&& (props.optimalTilingFeatures & features) == features){
      depth_format = candidates[i];
      break;
    }
  }

  if(depth_format != VK_FORMAT_UNDEFINED){
    *format = depth_format;
    return 0;
  }else {
    printf("no suitable depth format found!\n");
    return 1;
  }
}

int
descriptor_set_layouts_create(VkDevice l_dev, GfxPipeline *pipeline)
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
				 &pipeline->slow_layout) != VK_SUCCESS) return 1;

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
				 &pipeline->rapid_layout) != VK_SUCCESS) return 1;


  return 0;
}



VkShaderModule
shader_module_create(VkDevice l_dev, long size, const char* code)
{

  VkShaderModuleCreateInfo create_info = {
    .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
    .codeSize = size,
    .pCode = (const uint32_t*)code
  };

  VkShaderModule shader_mod;
  if (vkCreateShaderModule(l_dev, &create_info, NULL, &shader_mod)
     != VK_SUCCESS) {
    printf("failed to create shader module!");
    return NULL;
  }
  
  return shader_mod;
}

char*
shader_file_read(const char* filename, long* buffer_size)
{
  
  FILE* file = fopen(filename, "rb");
  if(file == NULL) {
      printf("%s not found!", filename);
      return 0;
  }
  if(fseek(file, 0l, SEEK_END) != 0) {
    printf("failed to seek to end of file!");
    return 0;
  }
  *buffer_size = ftell(file);
  if (*buffer_size == -1){
    printf("failed to get file size!");
    return 0;
  }

  char *spv_code = (char *)malloc(*buffer_size * sizeof(char));
  //char spv_code[*buffer_size];
  rewind(file);
  fread(spv_code, *buffer_size, 1, file);
  
  fclose(file);
  
  return spv_code;
}

int
depth_create(GfxContext context, GfxPipeline *pipeline)
{
  VkFormat format;
  if(depth_format_check(context.p_dev, &format) == 1) return 1;
  
  image_create(context, &pipeline->depth, &pipeline->depth_memory,
	       format,
	       VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
	       context.extent.width, context.extent.height);

  image_view_create(context.l_dev, pipeline->depth, &pipeline->depth_view,
		    format, VK_IMAGE_ASPECT_DEPTH_BIT);
  
  return 0;
}

int
render_pass_create(GfxContext context, VkRenderPass *render_pass)
{
  VkAttachmentDescription color_attachment = {
    .format = cfg_format,
    .samples = VK_SAMPLE_COUNT_1_BIT,
    .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
    .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
    .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
    .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
    .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
  };

  VkAttachmentReference color_attachment_ref = {
    .attachment = 0,
    .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
  };

  VkFormat depth_format;
  if(depth_format_check(context.p_dev, &depth_format)) return 1;
  
  VkAttachmentDescription depth_attachment = {
    .format = depth_format,
    .samples = VK_SAMPLE_COUNT_1_BIT,
    .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
    .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
    .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
    .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
    .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
  };

  VkAttachmentReference depth_attachment_ref = {
    .attachment = 1,
    .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
  };

  VkSubpassDescription subpass = {
    .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
    .colorAttachmentCount = 1,
    .pColorAttachments = &color_attachment_ref,
    .pDepthStencilAttachment = &depth_attachment_ref,
  };
  
  VkSubpassDependency dependency = {
    .srcSubpass = VK_SUBPASS_EXTERNAL,
    .dstSubpass = 0,
    .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
    | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
    
    .srcAccessMask = 0,
    .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
    | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
    .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT
    | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
  };

  VkAttachmentDescription attachments[2] =
    {color_attachment, depth_attachment};
  
  VkRenderPassCreateInfo render_pass_info = {
    .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
    .attachmentCount = 2,
    .pAttachments = attachments,
    .subpassCount = 1,
    .pSubpasses = &subpass,
    .dependencyCount = 1,
    .pDependencies = &dependency
  };

  if(vkCreateRenderPass(context.l_dev, &render_pass_info, NULL, render_pass)
     != VK_SUCCESS) {
    printf("!failed to create render pass!");
    return 1;
  }

  return 0;
}

int
pipeline_create(GfxContext context, GfxPipeline *gfx)
{
  long vert_spv_size;
  char* vert_spv = shader_file_read("shaders/vert.spv", &vert_spv_size);
  long frag_spv_size;
  char* frag_spv = shader_file_read("shaders/frag.spv", &frag_spv_size);

  VkShaderModule vert_shader_mod = shader_module_create
    (context.l_dev, vert_spv_size, vert_spv);
  VkShaderModule frag_shader_mod = shader_module_create
    (context.l_dev, frag_spv_size, frag_spv);
  free(vert_spv);
  free(frag_spv);
  
  VkPipelineShaderStageCreateInfo vert_info = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
    .stage = VK_SHADER_STAGE_VERTEX_BIT,
    .module = vert_shader_mod,
    .pName = "main",
    .pSpecializationInfo = NULL,
  };

  VkPipelineShaderStageCreateInfo frag_info = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
    .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
    .module = frag_shader_mod,
    .pName = "main",
    .pSpecializationInfo = NULL,
  };

  VkPipelineShaderStageCreateInfo shader_stages[2] =
    {vert_info, frag_info};

  VkPipelineDepthStencilStateCreateInfo depth_stencil = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
    .depthTestEnable = VK_TRUE,
    .depthWriteEnable = VK_TRUE,
    .depthCompareOp = VK_COMPARE_OP_LESS,
    .depthBoundsTestEnable = VK_FALSE,
    .minDepthBounds = 0.0f,
    .maxDepthBounds = 1.0f,
    .stencilTestEnable = VK_FALSE,
    .front = { 0 },
    .back = { 0 },
  };
  
  VkDynamicState dynamic_states[2] =
    { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
  
  VkPipelineDynamicStateCreateInfo dynamic_state_info = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
    .dynamicStateCount = sizeof(dynamic_states) / sizeof(dynamic_states[0]),
    .pDynamicStates = dynamic_states
  };

  // Vertex Buffer Creation
  VkVertexInputAttributeDescription attribute_descriptions[2];
  attribute_descriptions[0].binding = 0;
  attribute_descriptions[0].location = 0;
  attribute_descriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
  attribute_descriptions[0].offset = offsetof(vertex, pos);

  attribute_descriptions[1].binding = 0;
  attribute_descriptions[1].location = 1;
  attribute_descriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
  attribute_descriptions[1].offset = offsetof(vertex, tex);

  VkVertexInputBindingDescription binding_description = {
    .binding = 0,
    .stride = sizeof(vertex),
    .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
  };
  
  VkPipelineVertexInputStateCreateInfo vertex_input_info = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
    .vertexBindingDescriptionCount = 1,
    .vertexAttributeDescriptionCount = 2,
    .pVertexBindingDescriptions = &binding_description,
    .pVertexAttributeDescriptions = attribute_descriptions,
  };

  VkPipelineInputAssemblyStateCreateInfo input_assembly_info = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
    .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
    .primitiveRestartEnable = VK_FALSE,
  };

  VkViewport viewport = {
    .x = 0.0f,
    .y = 0.0f,
    .width = context.extent.width,
    .height = context.extent.height,
    .minDepth = 0.0f,
    .maxDepth = 1.0f,
  };

  VkRect2D scissor  = {
    .offset = { 0, 0},
    .extent = context.extent,
  };

  VkPipelineViewportStateCreateInfo viewport_info = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
    .viewportCount = 1,
    .pViewports = &viewport,
    .scissorCount = 1,
    .pScissors = &scissor,
  };

  VkPipelineRasterizationStateCreateInfo rasterization_info = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
    .depthClampEnable = VK_FALSE,
    .rasterizerDiscardEnable = VK_FALSE,
    .polygonMode = VK_POLYGON_MODE_FILL,
    .lineWidth = 1.0f,
    .cullMode = VK_CULL_MODE_BACK_BIT,
    .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
    .depthBiasEnable = VK_FALSE,
    .depthBiasConstantFactor = 0.0f,
    .depthBiasClamp = 0.0f,
    .depthBiasSlopeFactor = 0.0f,
  };

  VkPipelineMultisampleStateCreateInfo multisample_info = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
    .sampleShadingEnable = VK_FALSE,
    .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
    .minSampleShading = 1.0f,
    .pSampleMask = NULL,
    .alphaToCoverageEnable = VK_FALSE,
    .alphaToOneEnable = VK_FALSE,
  };

  VkPipelineColorBlendAttachmentState color_blend_attachment = {
    .colorWriteMask =
    VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT
    | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
    .blendEnable = VK_FALSE,
    .srcColorBlendFactor = VK_BLEND_FACTOR_ONE, // Optional
    .dstColorBlendFactor = VK_BLEND_FACTOR_ZERO, // Optional
    .colorBlendOp = VK_BLEND_OP_ADD, // Optional
    .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE, // Optional
    .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO, // Optional
    .alphaBlendOp = VK_BLEND_OP_ADD, // Optional
  };

  VkPipelineColorBlendStateCreateInfo color_blend_info = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
    .logicOpEnable = VK_FALSE,
    .logicOp = VK_LOGIC_OP_COPY,
    .attachmentCount = 1,
    .pAttachments = &color_blend_attachment,
    .blendConstants[0] = 0.0f,
    .blendConstants[1] = 0.0f,
    .blendConstants[2] = 0.0f,
    .blendConstants[3] = 0.0f,
  };
  VkDescriptorSetLayout descriptor_set_layouts[2] = {
    gfx->slow_layout, gfx->rapid_layout
  };
  
  VkPipelineLayoutCreateInfo pipeline_layout_info = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
    .setLayoutCount = 2,
    .pSetLayouts = descriptor_set_layouts,

  };

  if(vkCreatePipelineLayout(context.l_dev, &pipeline_layout_info,
			    NULL, &gfx->layout)
       != VK_SUCCESS)
    {
      printf("!failed to create pipeline layout!\n");
      return 1;
    }

  VkGraphicsPipelineCreateInfo pipeline_info = {
    .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
    .stageCount = 2,
    .pStages = shader_stages,
    .pVertexInputState = &vertex_input_info,
    .pInputAssemblyState = &input_assembly_info,
    .pViewportState = &viewport_info,
    .pRasterizationState = &rasterization_info,
    .pMultisampleState = &multisample_info,
    .pDepthStencilState = &depth_stencil,
    .pColorBlendState = &color_blend_info,
    .pDynamicState = &dynamic_state_info,
    .layout = gfx->layout,
    .renderPass = gfx->render_pass,
    .subpass = 0,
    .basePipelineHandle = VK_NULL_HANDLE,
    .basePipelineIndex = -1,
  };

  if(vkCreateGraphicsPipelines
     (context.l_dev, VK_NULL_HANDLE, 1, &pipeline_info, NULL, &gfx->pipeline) != VK_SUCCESS) {
    printf("!failed to create graphics pipeline!\n");
  }
    
  vkDestroyShaderModule(context.l_dev, frag_shader_mod, NULL);
  vkDestroyShaderModule(context.l_dev, vert_shader_mod, NULL);
    
  return 0;
}

int
framebuffers_create(GfxContext context, GfxPipeline *pipeline)
{
  pipeline->framebuffers = malloc(context.frame_c * sizeof(VkFramebuffer));

  for(size_t i = 0; i < context.frame_c; i++){
    VkImageView attachments[] = {
      context.views[i],
      pipeline->depth_view,
    };

    VkFramebufferCreateInfo framebuffer_info = {
      .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
      .renderPass = pipeline->render_pass,
      .attachmentCount = 2,
      .pAttachments = attachments,
      .width = context.extent.width,
      .height = context.extent.height,
      .layers = 1,
    };

    if(vkCreateFramebuffer(context.l_dev, &framebuffer_info,
			   NULL, &pipeline->framebuffers[i] )
       != VK_SUCCESS) {
      return 1;
      printf("!failed to create framebuffer!\n");
    }
  }

  return 0;
}

int
command_buffer_create(GfxContext context, VkCommandBuffer *command_buffer){
  VkCommandBufferAllocateInfo allocate_info = {
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
    .commandPool = context.command_pool,
    .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
    .commandBufferCount = 1,
  };

  if(vkAllocateCommandBuffers(context.l_dev, &allocate_info, command_buffer)
     != VK_SUCCESS) {
    printf("!failed to allocate command buffers!\n");
    return 1;
  }

  return 0;
}

int
stage2_create(GfxContext context, GfxPipeline *pipeline)
{
  depth_create(context, pipeline);
  render_pass_create(context, &pipeline->render_pass);
  framebuffers_create(context, pipeline);
  
  /* Sync Objects for Command Buffers */
  command_buffer_create(context, &pipeline->command_buffer);
  VkSemaphoreCreateInfo semaphore_info = {
    .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
  };

  VkFenceCreateInfo fence_info = {
    .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
    .flags = VK_FENCE_CREATE_SIGNALED_BIT,
  };

  if(vkCreateSemaphore(context.l_dev, &semaphore_info,
		       NULL, &pipeline->image_available)
     != VK_SUCCESS ||
     vkCreateSemaphore(context.l_dev, &semaphore_info,
		       NULL, &pipeline->render_finished)
     != VK_SUCCESS ||
     vkCreateFence(context.l_dev, &fence_info,
		   NULL, &pipeline->in_flight)
     != VK_SUCCESS ) {
    printf("!failed to create sync objects!\n");
    return 1;
  }

  descriptor_set_layouts_create(context.l_dev, pipeline);
  pipeline_create(context, pipeline);

  return 0;
}

int
stage2_destroy(GfxContext context, GfxPipeline pipeline)
{
  vkResetCommandBuffer(pipeline.command_buffer, 0);
  vkDestroyCommandPool(context.l_dev, context.command_pool, NULL);
  vkDestroyImageView(context.l_dev, pipeline.depth_view, NULL);
  vkDestroyImage(context.l_dev, pipeline.depth, NULL);
  vkFreeMemory(context.l_dev, pipeline.depth_memory, NULL);
  
  /* Cleaning Up */
  for(uint32_t i = 0; i < context.frame_c; i++){
    vkDestroyFramebuffer(context.l_dev, pipeline.framebuffers[i], NULL);
  }
  free(pipeline.framebuffers);

  vkDestroySemaphore(context.l_dev, pipeline.image_available, NULL);
  printf("render finsihed!\n");
  vkDestroySemaphore(context.l_dev, pipeline.render_finished, NULL);
  vkDestroyFence(context.l_dev, pipeline.in_flight, NULL);
  vkDestroyRenderPass(context.l_dev, pipeline.render_pass, NULL);
  
  vkDestroyPipeline(context.l_dev, pipeline.pipeline, NULL);
  vkDestroyPipelineLayout(context.l_dev, pipeline.layout, NULL);
  
  vkDestroyDescriptorPool(context.l_dev, pipeline.descriptor_pool, NULL);
  vkDestroyDescriptorSetLayout(context.l_dev, pipeline.rapid_layout, NULL);
  free(pipeline.rapid_sets);
  vkDestroyDescriptorSetLayout(context.l_dev, pipeline.slow_layout, NULL);
  
  return 0;
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


