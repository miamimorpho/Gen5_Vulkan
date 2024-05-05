#include "vulkan_util.h"
#include "gui.h"
#include "textures.h"
#include "destroyer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int FONT_LOADED = 0;

static VkPipelineLayout g_pipeline_layout;
static VkPipeline g_pipeline;

int bdf_load(GfxFont* font, uint8_t* pixels, size_t* size, char* filename){

  const int L_LEN = 80;
  const int W_LEN = 8;
  
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
      if(strcmp(encoding, "\"437\"") != 0 )printf("unsupported font format");

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

  if(*size <= 0){
    *size = font->glyph_width * font->glyph_c * font->height;
    return 0;
  }
  
  memset(pixels, 255, *size);
  
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
	pixels[pixel_xy] = pixel;
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

  size_t size = 0;
  err = bdf_load(font, NULL, &size, filename);
  uint8_t pixels[size];
  err = bdf_load(font, pixels, &size, filename);
  
  int width = font->glyph_width * font->glyph_c;
  
  err = texture_load(context, pixels, width, font->height, 1,
		     &font->texture_index);

  FONT_LOADED = 1;
  
  return err;
}


int gui_pipeline_create(GfxContext context,
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
  attribute_descriptions[0].format = VK_FORMAT_R32G32_SFLOAT; // 2 double
  attribute_descriptions[0].offset = offsetof(vertex2, pos);

  attribute_descriptions[1].binding = 0;
  attribute_descriptions[1].location = 1;
  attribute_descriptions[1].format = VK_FORMAT_R32G32_SFLOAT; // 2 float
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

int gui_shader_create(GfxContext context)
{
  VkPipelineLayoutCreateInfo pipeline_layout_info = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
    .setLayoutCount = 1,
    .pSetLayouts = &context.texture_descriptors_layout,
  };

  if(vkCreatePipelineLayout(context.l_dev, &pipeline_layout_info,
			    NULL, &g_pipeline_layout)
       != VK_SUCCESS)
    {
      printf("!failed to create pipeline layout!\n");
      return 1;
    }
  deferd_add( deferd_VkPipelineLayout, &g_pipeline_layout);
  
  gui_pipeline_create(context, g_pipeline_layout,
		       &g_pipeline);
  deferd_add( deferd_VkPipeline, &g_pipeline);

  return 0;
}

int text_render(GfxContext context, GfxFont font, char* string,
			    GfxBuffer* dest)
{
  if(FONT_LOADED == 0)return 1;

  double aspect = (double)font.glyph_width / (double)font.height;
  double pos_stride_x = (double)font.glyph_width / (double)context.extent.width *4;
  double pos_stride_y = pos_stride_x / aspect;
  
  float uv_stride = (float)font.glyph_width /
    (float)(font.glyph_c * font.glyph_width);
  
  int len = strlen(string);
  size_t vertex_c = len * 4;
  vertex2 vertices[vertex_c];
  size_t index_c = len * 6;
  uint32_t indices[index_c];
  
  float cursor_x = -1;
  float cursor_y = -1;

  char* c = string; 
 
 
  /* START OF COMPOSITING LOOP */
  for(int x = 0; x < len; x++){

    vertex2 top_left = {
      .pos = { (float)cursor_x, cursor_y},
      .uv = { (float)*c * uv_stride, 0}
    };
    cursor_x += pos_stride_x;
    
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
  }/* END OF COMPOSITING LOOP */

  buffer_append(context, dest, vertices, vertex_c * sizeof(vertex2));
  
  GfxBuffer* dest_indices = buffer_next(context, *dest);
  buffer_append(context, dest_indices, indices, index_c * sizeof(uint32_t));

  return index_c;
}

int gui_shader_bind(GfxContext context){
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
	            g_pipeline);

  return 0;
}
