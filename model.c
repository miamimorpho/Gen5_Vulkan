#include "solid.h"

#ifndef CGLTF_IMPLEMENTATION
#define CGLTF_IMPLEMENTATION
#endif

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#endif

#include "cgltf.h"
#include "stb_image.h"

static uint32_t TEXTURES_USED = 0;

int
models_destroy(GfxContext context, uint32_t count,
	       GeometryData *geo, ImageData *textures)
{
  /* once we have done textures and world mesh, we should come
   * up with a generic way to create/destroy models with no
   * memory leaks */
  buffers_destroy(context, 1, &geo->vertices);
  buffers_destroy(context, 1, &geo->indices);
  
  for(uint32_t i = 0; i < count; i++){
    image_destroy(context, &textures[i]);
  }
  
  return 1;
}

GeometryData
geometry_buffer_create(GfxContext context)
{
  static const int MAX_VERTEX_COUNT = 30000; /* Representing 10'000 triangles */
  GeometryData geo;
  
  buffer_create(context,
		MAX_VERTEX_COUNT * sizeof(vertex),
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		&geo.vertices);
  geo.vertices_used = 0;
  
  
  buffer_create(context,
		MAX_VERTEX_COUNT * sizeof(uint32_t),
		VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		&geo.indices);
  geo.indices_used = 0;
  
  return geo;
}

int
vertices_append(const vertex *vertices, uint32_t count, GeometryData *dest){

  size_t size = count * sizeof(vertex);
  vertex *write = ((vertex*)dest->vertices.first_index) + dest->vertices_used;
  memcpy(write, vertices, size);
  dest->vertices_used += count;

  return 0;
}

int
indices_append(const uint32_t *indices, uint32_t count, GeometryData *dest){
  
  size_t size = count * sizeof(uint32_t);
  uint32_t *write = ((uint32_t*)dest->indices.first_index) + dest->indices_used;
  memcpy(write, indices, size);
  dest->indices_used += count;

  return 0;
}

int
sampler_create(GfxContext context, VkSampler *sampler)
{

  VkPhysicalDeviceProperties properties = { 0 };
  vkGetPhysicalDeviceProperties(context.p_dev, &properties);
  
  VkSamplerCreateInfo sampler_info = {
    .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
    .magFilter = VK_FILTER_NEAREST,
    .minFilter = VK_FILTER_NEAREST,
    .addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
    .addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
    .addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
    .anisotropyEnable = VK_TRUE,
    .maxAnisotropy = properties.limits.maxSamplerAnisotropy,
    .borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
    .unnormalizedCoordinates = VK_FALSE,
    .compareEnable = VK_FALSE,
    .compareOp = VK_COMPARE_OP_ALWAYS,
    .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
    .mipLodBias = 0.0f,
    .minLod = 0.0f,
    .maxLod = 0.0f,
  };

  if(vkCreateSampler(context.l_dev, &sampler_info, NULL, sampler)
     != VK_SUCCESS){
    printf("!failed to create buffer\n!");
    return 1;
  }
  
  return 0;
  
}

int
copy_buffer_to_image(GfxContext context,
		     VkBuffer buffer, VkImage image,
		     uint32_t width, uint32_t height)
{
  VkCommandBuffer command = command_single_begin(context);
  
  VkBufferImageCopy region = {
    .bufferOffset = 0,
    .bufferRowLength = 0,
    .bufferImageHeight = 0,

    .imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
    .imageSubresource.mipLevel = 0,
    .imageSubresource.baseArrayLayer = 0,
    .imageSubresource.layerCount = 1,

    .imageOffset = {0, 0, 0},
    .imageExtent  = {width, height, 1},
  };

  vkCmdCopyBufferToImage(command, buffer, image,
			 VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			 1, &region);
  
  return command_single_end(context, &command);
}

int
transition_image_layout(GfxContext context, VkImage image,
			VkImageLayout old_layout, VkImageLayout new_layout)
{
  VkCommandBuffer commands = command_single_begin(context);

  VkImageMemoryBarrier barrier = {
    .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
    .oldLayout = old_layout,
    .newLayout = new_layout,
    .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
    .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
    .image = image,
    .subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
    .subresourceRange.baseMipLevel = 0,
    .subresourceRange.levelCount = 1,
    .subresourceRange.baseArrayLayer = 0,
    .subresourceRange.layerCount = 1,
    .srcAccessMask = 0,
    .dstAccessMask = 0,
  };

  VkPipelineStageFlags source_stage, destination_stage;
  
  if(old_layout == VK_IMAGE_LAYOUT_UNDEFINED
     && new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL){
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    destination_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;

  }else if(old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
	    && new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL){
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    
    source_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    destination_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    
  } else {
    printf("unsupported layout transition\n");
    return 1;
  }
  
  vkCmdPipelineBarrier(commands,
		       source_stage, destination_stage,
		       0,
		       0, NULL,
		       0, NULL,
		       1, &barrier);
  
  return command_single_end(context, &commands);
}

/* error 1 file format */
int
gltf_skin_load(GfxContext context, cgltf_data* file, ImageData *texture)
{
 /* Load image skin textures */
  cgltf_buffer_view* view = NULL;
  for(cgltf_size i = 0 ; i < file->images_count; i++){
    printf("texture MIME type: %s\n", file->images[i].mime_type);
    if(strcmp(file->images[i].mime_type, "image/png") == 0){
      view = file->images[i].buffer_view;
      break;
    }
  }
  if(view == NULL){
    printf("no view found!\n");
    return 1;
  }
  
  /* create view accessor */
  int offset = view->offset;
  int stride = view->stride ? view->stride : 1;

  /* load raw data */
  stbi_uc* raw_pixels = malloc(view->size);
  for(unsigned int i = 0; i < view->size; i++){
    raw_pixels[i] = ((unsigned char *)view->buffer->data)[offset];
    offset += stride;
  }

  /* Choose image format */
  int width, height, channels;
  stbi_info_from_memory(raw_pixels, view->size,
			&width, &height, &channels);
  VkFormat format;
  switch(channels){
  case 4:
    format = VK_FORMAT_R8G8B8A8_SRGB;
    break;
  case 3:
    format = VK_FORMAT_R8G8B8_SRGB;
    break;
  default:
    return 2;
  }
  
  /* interpret view file */
  stbi_uc* pixels = stbi_load_from_memory
    ((stbi_uc*)raw_pixels, view->size,
     &width, &height, &channels, channels);
  if(pixels == NULL) {
    printf("failed to interpret raw pixel data\n");
    return 2;
  }
  free(raw_pixels);

  VkDeviceSize image_size = width * height * channels;
  DeviceBuffer image_b;
  buffer_create(context, image_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		&image_b);

  /* copy pixel data to buffer */
  memcpy(image_b.first_index, pixels, image_size);

  stbi_image_free(pixels);
  //  free(pixels);

  image_create(context, &texture->handle, &texture->memory,
	       format,
	       VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
	       width, height);

  /* Copy the image buffer to a VkImage proper */
  transition_image_layout(context, texture->handle,
			  VK_IMAGE_LAYOUT_UNDEFINED,
			  VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

  copy_buffer_to_image(context, image_b.handle, texture->handle,
		       (uint32_t)width, (uint32_t)height);

  transition_image_layout(context, texture->handle,
			  VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			  VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

  buffers_destroy(context, 1, &image_b);
  
  /* Create image view */
  image_view_create(context.l_dev, texture->handle,
		    &texture->view, format,
		    VK_IMAGE_ASPECT_COLOR_BIT);
  sampler_create(context, &texture->sampler);
  
  
  return 0;
  /* images->buffer_view->buffers */
  /* loaded by cgltf_load_buffers */
}

int
gltf_mesh_load(cgltf_data* data, GeometryData *geo, Entity *component)
{
  /* currently only loads 1 mesh per model */
  uint32_t mesh_index = 0;
  uint32_t primitive_index = 0;

  cgltf_mesh* mesh = &data->meshes[mesh_index];
  cgltf_primitive* primitive = &mesh->primitives[primitive_index];
  
  /* Search for attributes accessors */
  int pos_index = 0;
  int tex_index = 0;
  int normal_index = 0;
  int color_index = 0;
  for(size_t attrib_i = 0; attrib_i < primitive->attributes_count; attrib_i++){
    cgltf_attribute_type attribute = primitive->attributes[attrib_i].type;

    if(attribute == cgltf_attribute_type_position){
      pos_index = attrib_i;
      printf("found vertices\n");
    }
      
    if(attribute == cgltf_attribute_type_normal){
      normal_index = attrib_i;
      printf("found normals\n");
    }

    if(attribute == cgltf_attribute_type_color){
      color_index = attrib_i;
      printf("found colours\n");
    }
    
    if(attribute == cgltf_attribute_type_texcoord){
      tex_index = attrib_i;
      printf("found texture coords\n");
    }
 
  }

  /* load attributes into vertex buffer
   * starting with position data */
  cgltf_accessor* pos_accessor = data->meshes[mesh_index]
    .primitives[0]
    .attributes[pos_index].data;
  int float_count = pos_accessor->count * 3;
  float pos_data[float_count];
  cgltf_accessor_unpack_floats(pos_accessor, pos_data, float_count);
  
  /* textures data */
  cgltf_accessor* tex_accessor = data->meshes[mesh_index]
    .primitives[primitive_index].attributes[tex_index].data;
  float tex_data[float_count];
  cgltf_accessor_unpack_floats(tex_accessor, tex_data, float_count);

  /* normals */
  cgltf_accessor* normal_accessor = data->meshes[mesh_index]
    .primitives[primitive_index].attributes[normal_index].data;
  float normal_data[float_count];
  cgltf_accessor_unpack_floats(normal_accessor, normal_data, float_count);

  /* position and normal are 3D 32bit numbers
   * textures are 2D 32bit numbers
   */
  
  vertex vertices[pos_accessor->count];
  for(size_t i = 0; i < pos_accessor->count; i++){
    vertex v = {
      .pos = {pos_data[i * 3 ], pos_data[i * 3 +1 ], pos_data[i * 3 +2 ] },
      .normal =
      {normal_data[i * 3], normal_data[i * 3 + 1], normal_data[i * 3 + 2] },
      .tex = {tex_data[i * 2 ], tex_data[i * 2 +1 ] },
    };
    vertices[i] = v;
  }
  component->vertex_offset = geo->vertices_used;
  vertices_append(vertices, pos_accessor->count, geo);

  /* Load indice data into int array */
  /* Then load that into a vulkan buffer */
  cgltf_accessor* indices_accessor =
    data->meshes[mesh_index].primitives[primitive_index].indices;

  cgltf_uint indices[indices_accessor->count];
  //cgltf_uint* indices = (cgltf_uint*)malloc(indices_accessor->count * sizeof(cgltf_uint));
  cgltf_accessor_unpack_indices
    (indices_accessor, indices, indices_accessor->count);
  
  component->first_index = geo->indices_used;
  component->indices_len = indices_accessor->count;
  indices_append(indices, indices_accessor->count, geo);
  
  return 0;
}

int gltf_load(GfxContext context, const char* filename,
	      GeometryData *geo, ImageData *textures, Entity* components)
{
  cgltf_data* data;
  cgltf_options options = {0};

  // add static here for TEXTURES USED
  
  /* Check we can use the gltf data
   * must be triangualated and contain only 1 mesh and 1 primitive
   */

  if(cgltf_parse_file(&options, filename, &data)
     != cgltf_result_success) return 1;
  if(cgltf_load_buffers(&options, data, filename)
     != cgltf_result_success) return 1;
  if(cgltf_validate(data)
     != cgltf_result_success) return 1;

  if(gltf_mesh_load(data, geo, components)) return 2;

  if(gltf_skin_load(context, data, &textures[TEXTURES_USED])) return 3;
  components->texture_index = TEXTURES_USED;
  TEXTURES_USED += 1;

  cgltf_free(data);
  
  return 0;
}
