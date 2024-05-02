#include "vulkan_util.h"
#include "ui.h"
#include "model.h"
#include "textures.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int FONT_LOADED = 0;

int bdf_load(GfxFont* font, char* filename){

  const int L_LEN = 80;
  const int W_LEN = 8;
  printf("BDF_LOAD\n");
  
  FILE *fp = fopen(filename, "r");
  if(fp == NULL) {
    printf("Error opening font file\n");
    return 1;
  }
 
  char line[L_LEN];
  char setting[L_LEN];
  
  int supported = 0;
  /* METADATA LOADING START */
  while(fgets(line, sizeof(line), fp) != NULL) {

    if(supported == 3)break;
    
    sscanf(line, " %s", setting);

    if(strcmp(setting, "FONTBOUNDINGBOX") == 0){

      char x_s[W_LEN];
      char y_s[W_LEN];
      char x_offset_s[W_LEN];
      char y_offset_s[W_LEN];
      
      sscanf(line, "%*s %s %s %s %s",
	     x_s, y_s, x_offset_s, y_offset_s);

      font->glyph_width = atoi(x_s);
      font->height = atoi(y_s);
  
      supported += 1;
      continue;
    }

    if(strcmp(setting, "CHARSET_ENCODING") == 0){
      char encoding[W_LEN]; 
      
      sscanf(line, "%*s %s", encoding);
      if(strcmp(encoding, "\"437\"") == 0 )printf("Supported Encoding!\n");

      supported += 1;
      continue;
    }

    if(strcmp(setting, "CHARS") == 0){
      char glyph_c_s[W_LEN];
      sscanf(line, "%*s %s", glyph_c_s);

      font->glyph_c = atoi(glyph_c_s);
      //printf("Glyph Count %d\n", font.glyph_c);
      supported += 1;
      continue;
    }
   
  }
  
  if(supported != 3){
    printf("not enough config information\n");
    return 2;
  }/* META LOADING END */
  printf("font supported\n");
  
  int image_size = font->glyph_width
    * font->glyph_c
    * font->height;
  font->pixels = (uint8_t*)malloc(image_size);
  memset(font->pixels, 255, image_size);
  
  rewind(fp);
  int in_bytes = 0;
  int row_i = 0;
  int code_i = 0;

  /* BYTE LOADING START */
  while(fgets(line, sizeof(line), fp) != NULL) {
 
    sscanf(line, "%s", setting);

    if(strcmp(setting, "ENCODING") == 0){
      char code[W_LEN];
      sscanf(line, "%*s %s", code);
      code_i = atoi(code);
 
   
      continue;
    }
    
    if(strcmp(setting, "ENDCHAR") == 0){
      row_i = 0;
      code_i = 0;
      in_bytes = 0;
    
      continue;
    }
    
    if(in_bytes == 1){
   
      uint8_t row = (uint8_t)strtol(line, NULL, 16);
      int y = row_i++ * (font->glyph_width * font->glyph_c);
    
      for(signed int i = 7; i >= 0; --i){
	int pixel_xy = y + (code_i * font->glyph_width) + i;
	uint8_t pixel = 0;
	if((row >> (7 - i) ) & 0x01)pixel = 255;	
	font->pixels[pixel_xy] = pixel;
      }
   
      continue;
    }
      
    if(strcmp(setting, "STARTCHAR") == 0){
      char* glyph_name = strchr(line, ' ');
      if(glyph_name != NULL)glyph_name += 1;
      glyph_name[strlen(glyph_name)-1] = '\0';
 
      continue;
    }

    if(strcmp(setting, "BITMAP") == 0){
      in_bytes = 1;
   
      continue;
    }
  }/* BYTE LOADING END*/

  fclose(fp);
  return 0;
  
}

int gfx_font_load(GfxContext context, GfxFont* font, char* filename){
  int err;
  
  err = bdf_load(font, filename);

  int width = font->glyph_width * font->glyph_c;
  
  err = texture_load(context, font->pixels, width, font->height, 1,
		     &font->texture_index);

  FONT_LOADED = 1;
  
  return err;
}


int text_pipeline_create(GfxContext context,
		      VkPipelineLayout pipeline_layout,
		      VkPipeline* pipeline)
{
  VkShaderModule vert_shader;
  shader_spv_load(context.l_dev, "shaders/text.vert.spv", &vert_shader);
  VkShaderModule frag_shader;
  shader_spv_load(context.l_dev, "shaders/text.frag.spv", &frag_shader);
  
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
  VkVertexInputAttributeDescription attribute_descriptions[2];
  attribute_descriptions[0].binding = 0;
  attribute_descriptions[0].location = 0;
  attribute_descriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
  attribute_descriptions[0].offset = offsetof(vertex2, pos);

  attribute_descriptions[1].binding = 0;
  attribute_descriptions[1].location = 1;
  attribute_descriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
  attribute_descriptions[1].offset = offsetof(vertex2, uv);
  
  VkVertexInputBindingDescription binding_description = {
    .binding = 0,
    .stride = sizeof(vertex2),
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
    .blendEnable = VK_TRUE,
    .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
    .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
    .colorBlendOp = VK_BLEND_OP_ADD, // Optional
    .srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA, // Optional
    .dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, // Optional
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


int text_shader_create(GfxContext context, GfxShader* shader)
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
  text_pipeline_create(context, shader->pipeline_layout,
			&shader->pipeline);

  return 0;
}
int
text_indirect_b_create(GfxContext context, GfxShader* shader, size_t draw_count)
{
  shader->uniform_b = (GfxBuffer*)malloc(sizeof(GfxBuffer) * context.frame_c);
  
  for(unsigned int i = 0; i < context.frame_c; i++){

    GfxBuffer* b = &shader->uniform_b[i];
    VkDeviceSize size = draw_count * sizeof(text_args);
    
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
text_load(GfxContext context,
	      GfxStagingText staging,
	      GfxBuffer* g, GfxModelOffsets* model){

  VmaAllocationInfo g_vertices_info;
  vmaGetAllocationInfo(context.allocator, g->allocation, &g_vertices_info);
  GfxBuffer* g_indices = (GfxBuffer*)g_vertices_info.pUserData;
  VmaAllocationInfo g_indices_info;
  vmaGetAllocationInfo(context.allocator, g_indices->allocation, &g_indices_info);
  
  size_t vb_size = staging.vertex_c * sizeof(vertex2);
  if(g_vertices_info.size < g->used_size + vb_size )return 1;
  
  size_t ib_size = staging.index_c * sizeof(uint32_t);
  if(g_indices_info.size < g_indices->used_size + ib_size )return 1;
  
  // we need to give offsets before we append to the geometry buffer
  model->firstIndex = g_indices->used_size / sizeof(uint32_t);
  model->indexCount = staging.index_c;
  model->vertexOffset = g->used_size / sizeof(vertex2);
  model->textureIndex = 0;

  int err;
  err = vmaCopyMemoryToAllocation(context.allocator, staging.vertices,
			    g->allocation, g->used_size, vb_size);
  g->used_size += vb_size;
  
  err = vmaCopyMemoryToAllocation(context.allocator, staging.indices,
				  g_indices->allocation, g_indices->used_size,
				  ib_size);
  g_indices->used_size += ib_size;
  
  return err;
}

int text_render(GfxContext context, GfxFont font, char* string,
		GfxBuffer* dest, GfxModelOffsets* model){
  if(FONT_LOADED == 0)return 1;

  float aspect = ((float)font.glyph_width / (float)font.height);
  float pos_stride_x = (float)font.glyph_width / context.extent.width * 4;
  float pos_stride_y = pos_stride_x / aspect;
  printf("pos_strides x%f y %f\n", pos_stride_x, pos_stride_y);
  
  float uv_stride = (float)font.glyph_width /
    (float)(font.glyph_c * font.glyph_width);
  
  int len = strlen(string);
  char* c = string;

  size_t vertex_c = len * 4;
  size_t index_c = len * 6;
  vertex2* vertices = (vertex2*)malloc(sizeof(vertex2) * vertex_c);
  uint32_t* indices = (uint32_t*)malloc(sizeof(uint32_t) * index_c);
  for(int x = 0; x < len; x++){

    vertex2 top_left = {
      .pos = { (float)(x * pos_stride_x) - 1, -1.0f},
      .uv = { (float)*c * uv_stride, 0}
    };

    vertex2 bottom_left = top_left;
    bottom_left.pos[1] += pos_stride_y; // Y
    bottom_left.uv[1] += 1; // Y
    
    vertex2 top_right = top_left;
    top_right.pos[0] += pos_stride_x;
    top_right.uv[0] += uv_stride;

    vertex2 bottom_right = top_right;
    bottom_right.pos[1] += pos_stride_y; // Y
    bottom_right.uv[1] += 1.0f;  // Y

    int TL = x * 4;
    int TR = x * 4 +1;
    int BL = x * 4 +2;
    int BR = x * 4 +3;
    
    vertices[TL] = top_left;
    vertices[TR] = top_right;
    vertices[BL] = bottom_left;
    vertices[BR] = bottom_right;

    indices[x * 6] =  TL;
    indices[x * 6 +1] = BL;
    indices[x * 6 +2] = TR;
      
    indices[x * 6 +3] = TR;
    indices[x * 6 +4] = BL;
    indices[x * 6 +5] = BR;
    
    c++;
  }

  GfxStagingText staging = {
    .vertices = vertices,
    .vertex_c = vertex_c,
    .indices = indices,
    .index_c = index_c,
  };
  text_load(context, staging, dest, model);
  
  free(vertices);
  free(indices);

  return 0;
}


void text_draw(GfxContext context, GfxShader shader,
	       text_entity* entities, int count){
  VmaAllocationInfo indirect;
  vmaGetAllocationInfo(context.allocator,
		       shader.indirect_b.allocation,
		       &indirect);
  
  VmaAllocationInfo indirect_args;
  vmaGetAllocationInfo(context.allocator,
		       shader.uniform_b[context.current_frame_index].allocation,
		       &indirect_args);
  
  for(int i = 0; i < count; i++){
    text_entity* entity = &entities[i];
    // INDIRECT ARGS BUFFER
    text_args *args_ptr = (text_args*)indirect_args.pMappedData + i;
    args_ptr->texture_i = entity->model.textureIndex;

    glm_mat4_identity(args_ptr->view_m);
    
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

int font_free(GfxFont* font){
  free(font->pixels);
  return 0;
}
