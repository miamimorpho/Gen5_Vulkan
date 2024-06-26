#include "model.h"
#include "vulkan_util.h"

/*
 * Set = 1, Binding = Vert on 0
 *.No Side Effects, Layout must be destroyed
 */
int
ssbo_descriptors_layout(VkDevice l_dev, VkDescriptorSetLayout* ssbo_layout)
{
  VkDescriptorSetLayoutBinding binding = {
    .binding = 0,
    .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
    .descriptorCount = 1,
    .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
  };

  VkDescriptorSetLayoutCreateInfo layout_info = {
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
    .bindingCount = 1,
    .pBindings = &binding
  };

  if(vkCreateDescriptorSetLayout(l_dev, &layout_info, NULL,
				 ssbo_layout) != VK_SUCCESS) return 1;

  return 0;
}

int
ssbo_descriptors_alloc(GfxContext context,
		       VkDescriptorSetLayout descriptors_layout,
		       VkDescriptorSet* descriptor_sets,
		       uint32_t count)    
{

 
  
  VkDescriptorSetLayout layouts[count];
  for(uint32_t i = 0; i < count; i++){
    layouts[i] = descriptors_layout;
    descriptor_sets[i] = VK_NULL_HANDLE;
  }
  
  VkDescriptorSetAllocateInfo alloc_info = {
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
    .descriptorPool = context.descriptor_pool,
    .descriptorSetCount = count,
    .pSetLayouts = layouts,
  };

  if(vkAllocateDescriptorSets(context.l_dev, &alloc_info, descriptor_sets)
     != VK_SUCCESS){
    printf("fail super duper\n");
    return 1;
  }

  return 0;
}

int
pipeline_layout_create(VkDevice l_dev,
		       VkDescriptorSetLayout* descriptors_layouts,
		       VkPipelineLayout* pipeline_layout)
{
  
  VkPipelineLayoutCreateInfo pipeline_layout_info = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
    .setLayoutCount = 2,
    .pSetLayouts = descriptors_layouts,
  };

  if(vkCreatePipelineLayout(l_dev, &pipeline_layout_info,
			    NULL, pipeline_layout)
       != VK_SUCCESS)
    {
      printf("!failed to create pipeline layout!\n");
      return 1;
    }

  return 0;
}


int model_pipeline_create(GfxContext context,
		      VkPipelineLayout pipeline_layout,
		      VkPipeline* pipeline)
{
  VkShaderModule vert_shader;
  shader_spv_load(context.l_dev, "shaders/model.vert.spv", &vert_shader);
  VkShaderModule frag_shader;
  shader_spv_load(context.l_dev, "shaders/model.frag.spv", &frag_shader);
  
  VkPipelineShaderStageCreateInfo vert_info = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
    .stage = VK_SHADER_STAGE_VERTEX_BIT,
    .module = vert_shader,
    .pName = "main",
    .pSpecializationInfo = NULL,
  };

  VkPipelineShaderStageCreateInfo frag_info = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
    .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
    .module = frag_shader,
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
  }; // Model Specific
  
  VkDynamicState dynamic_states[2] =
    { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
  
  VkPipelineDynamicStateCreateInfo dynamic_state_info = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
    .dynamicStateCount = sizeof(dynamic_states) / sizeof(dynamic_states[0]),
    .pDynamicStates = dynamic_states
  };

  // Vertex Buffer Creation
  VkVertexInputAttributeDescription attribute_descriptions[3];
  attribute_descriptions[0].binding = 0;
  attribute_descriptions[0].location = 0;
  attribute_descriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
  attribute_descriptions[0].offset = offsetof(vertex3, pos);

  attribute_descriptions[1].binding = 0;
  attribute_descriptions[1].location = 1;
  attribute_descriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
  attribute_descriptions[1].offset = offsetof(vertex3, normal);

  attribute_descriptions[2].binding = 0;
  attribute_descriptions[2].location = 2;
  attribute_descriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
  attribute_descriptions[2].offset = offsetof(vertex3, uv);
  
  VkVertexInputBindingDescription binding_description = {
    .binding = 0,
    .stride = sizeof(vertex3),
    .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
  };
  
  VkPipelineVertexInputStateCreateInfo vertex_input_info = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
    .vertexBindingDescriptionCount = 1,
    .vertexAttributeDescriptionCount = 3,
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
    .layout = pipeline_layout,
    .renderPass = context.render_pass,
    .subpass = 0,
    .basePipelineHandle = VK_NULL_HANDLE,
    .basePipelineIndex = -1,
  };

  if(vkCreateGraphicsPipelines
     (context.l_dev, VK_NULL_HANDLE, 1, &pipeline_info, NULL, pipeline) != VK_SUCCESS) {
    printf("!failed to create graphics pipeline!\n");
  }
    
  vkDestroyShaderModule(context.l_dev, frag_shader, NULL);
  vkDestroyShaderModule(context.l_dev, vert_shader, NULL);
  
  return 0;
}

int
model_descriptors_update(GfxContext context, model_shader_t* shader, size_t draw_count)
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

int model_shader_create(GfxContext context, model_shader_t* shader)
{
  ssbo_descriptors_layout(context.l_dev, &shader->descriptors_layout);
  shader->descriptors =
    (VkDescriptorSet*)malloc(context.frame_c * sizeof( VkDescriptorSet ));
  ssbo_descriptors_alloc(context,
			 shader->descriptors_layout,
			 shader->descriptors,
			 context.frame_c);
  
  VkDescriptorSetLayout descriptors_layouts[2] =
    { context.texture_descriptors_layout, shader->descriptors_layout };
  pipeline_layout_create(context.l_dev, descriptors_layouts,
			 &shader->pipeline_layout);
  model_pipeline_create(context, shader->pipeline_layout,
			&shader->pipeline);

  return 0;
}
    
int model_shader_bind(GfxContext context, model_shader_t shader){
  VkViewport viewport = {
    .x = 0.0f,
    .y = 0.0f,
    .width = context.extent.width,
    .height = context.extent.height,
    .minDepth = 0.0f,
    .maxDepth = 1.0f,
  };
  vkCmdSetViewport(context.command_buffer, 0, 1, &viewport);

  VkRect2D scissor = {
    .offset = {0, 0},
    .extent = context.extent,
  };
    
  vkCmdSetScissor(context.command_buffer, 0, 1, &scissor);
  vkCmdBindPipeline(context.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
	            shader.pipeline);

  vkCmdBindDescriptorSets(context.command_buffer,
			  VK_PIPELINE_BIND_POINT_GRAPHICS,
			  shader.pipeline_layout, 1, 1,
			  &shader.descriptors[context.current_frame_index],
			  0, NULL);
    
  return 0;
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

void model_shader_draw(GfxContext context, model_shader_t shader,
		       Camera cam, Entity* entities, int count){
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
model_shader_destroy(GfxContext context, model_shader_t* shader)
{

  free(shader->descriptors);

  buffers_destroy(context,
		 shader->uniform_b,
		 context.frame_c);

  free(shader->uniform_b);
  
  buffer_destroy(context,
		 shader->indirect_b);
 
  vkDestroyPipeline(context.l_dev, shader->pipeline, NULL);
  vkDestroyPipelineLayout(context.l_dev, shader->pipeline_layout, NULL); 
  vkDestroyDescriptorSetLayout(context.l_dev, shader->descriptors_layout, NULL); 
 
  return 0;
}
