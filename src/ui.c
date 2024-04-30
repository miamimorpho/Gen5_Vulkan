#include "vulkan_util.h"
#include "ui.h"
#include "model.h"
#include "textures.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define L_LEN 80
#define W_LEN 8

static int FONT_LOADED = 0;

int bdf_load(GfxFont* font, char* filename){

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
  if(bdf_load(font, filename) > 0 )return 1;
  texture_memory_load(context, font->pixels,
		      font->glyph_c * font->glyph_width * font->height,
		      &font->texture_index);
  FONT_LOADED = 1;
  
  return 0;
}

int
ui_pipeline_create(GfxContext context, GfxShader *shader)
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
    .depthTestEnable = VK_FALSE,
    .depthWriteEnable = VK_FALSE,
    .depthCompareOp = VK_COMPARE_OP_ALWAYS,
  };
  
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
    .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA,
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
    .layout = shader->pipeline_layout,
    .renderPass = context.render_pass,
    .subpass = 0,
    .basePipelineHandle = VK_NULL_HANDLE,
    .basePipelineIndex = -1,
  };

  if(vkCreateGraphicsPipelines
     (context.l_dev, VK_NULL_HANDLE, 1, &pipeline_info, NULL, &shader->pipeline) != VK_SUCCESS) {
    printf("!failed to create graphics pipeline!\n");
  }
    
  vkDestroyShaderModule(context.l_dev, frag_shader, NULL);
  vkDestroyShaderModule(context.l_dev, vert_shader, NULL);
    
  return 0;
}


int render_3D_text(GfxContext context, GfxFont font, char* string,
		GfxBuffer* dest, GfxModelOffsets* model){
  if(FONT_LOADED == 0)return 1;

  float ratio = (float)font.glyph_width / (float)font.height;
  printf("%f\n", ratio);
  
  int len = strlen(string);
  char* c = string;

  size_t vertex_c = len * 4;
  size_t index_c = len * 6;
  vertex3* vertices = (vertex3*)malloc(sizeof(vertex3) * vertex_c);
  uint32_t* indices = (uint32_t*)malloc(sizeof(uint32_t) * index_c);
  for(int x = 0; x < len; x++){
    
    int atlas_width = font.glyph_c * font.glyph_width;
    vertex3 top_left = {
      .pos = { (float)x * ratio, 0.0f, -1.0f },
      .normal = { 1.0f, 1.0f, 1.0f},
      .uv = { (float)(*c * font.glyph_width) / (float)atlas_width, 0}
    };

    vertex3 bottom_left = top_left;
    bottom_left.pos[2] = 0; // Z
    bottom_left.uv[1] = 1; // Y
    
    vertex3 top_right = top_left;
    top_right.pos[0] += ratio; // X
    top_right.uv[0] += (float)font.glyph_width / (float)atlas_width; //X

    vertex3 bottom_right = top_right;
    bottom_right.pos[2] = 0; // Y
    bottom_right.uv[1] = 1;  // Y

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

  GfxStagingMesh staging = {
    .vertices = vertices,
    .vertex_c = vertex_c,
    .indices = indices,
    .index_c = index_c,
  };
  geometry_load(context, staging, dest, model);
  
  free(vertices);
  free(indices);

  return 0;
}

int font_free(GfxFont* font){
  free(font->pixels);
  return 0;
}
