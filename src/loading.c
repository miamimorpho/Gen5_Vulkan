#include "loading.h"

#define STB_IMAGE_IMPLEMENTATION
#include "../extern/stb_image.h"

#define CGLTF_IMPLEMENTATION
#include "../extern/cgltf.h"


int
vertex_buffer_append(const vertex *vertices, GfxBuffer *dest, uint32_t count ){
  return buffer_append(vertices, dest, count * sizeof(vertex) );
}

int
index_buffer_append(const uint32_t *indices, GfxBuffer *dest, uint32_t count){
  return buffer_append(indices, dest, count * sizeof(uint32_t) );
}


int texture_load(GfxContext context, unsigned char* pixels, ImageData* texture,
		 int width, int height, int channels){

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
  
  VkDeviceSize image_size = width * height * channels;
  GfxBuffer image_b;
  buffer_create(context, image_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		&image_b);

  /* copy pixel data to buffer */
  memcpy(image_b.first_ptr, pixels, image_size);
 
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

  buffers_destroy(context, &image_b, 1);

 
  
  /* Create image view */
  image_view_create(context.l_dev, texture->handle,
		    &texture->view, format,
		    VK_IMAGE_ASPECT_COLOR_BIT);
  sampler_create(context, &texture->sampler);

  return 0;
}


int textures_slot(GfxResources resources){
  for(uint32_t i = 0; i < MAX_SAMPLERS; i ++){
    if(resources.textures[i].handle == VK_NULL_HANDLE){
      printf("texture slot: [%d]\n", i);
      return i;
    }
  }
  return MAX_SAMPLERS;
}

int png_load(GfxContext context, const char* filename, GfxResources *resources){

  int ti = textures_slot(*resources);
  
  ImageData* dest = &resources->textures[ti];
  
  int x, y, n;
  stbi_info(filename, &x, &y, &n);
  printf("channels: %d\n", n);
  unsigned char *pixels = stbi_load(filename, &x, &y, &n, n);
  texture_load(context, pixels, dest, x, y, n);

  return ti;
}

/* error 1 file format */
int gltf_skin_load(GfxContext context, cgltf_data* file, ImageData* texture){
 /* Load image skin textures */
  cgltf_buffer_view* view = NULL;
  printf("loading textures [ ");
  for(cgltf_size i = 0 ; i < file->images_count; i++){
    printf("%s, ", file->images[i].mime_type);
    if(strcmp(file->images[i].mime_type, "image/png") == 0){
      view = file->images[i].buffer_view;
      break;
    }
  }
  if(view == NULL){
    printf("MISSING PNG");
    return 1;
  }
  printf(" ]\n");
  
  /* create view accessor */
  int size = view->size;
  int offset = view->offset;
  int stride = view->stride ? view->stride : 1;
  unsigned char *data = ((unsigned char *)view->buffer->data);

  /* load raw data */
  uint8_t* raw_pixels = malloc(size);
  for(int i = 0; i < size; i++){
    raw_pixels[i] = data[offset];
    offset += stride;
  }

  /* Choose image format */
  int width, height, channels;
  stbi_info_from_memory(raw_pixels, size,
			&width, &height, &channels);
  /* interpret view file */
  uint8_t* pixels = stbi_load_from_memory
    ((uint8_t*)raw_pixels, size,
     &width, &height, &channels, channels);
  if(pixels == NULL) {
    printf("failed to interpret raw pixel data\n");
    return 2;
  }

  texture_load(context, pixels, texture, width, height, channels);
  
  free(raw_pixels);
  
  return 0;
}

/* GfxBuffer *dest is a vertice buffer that is linked
 * with the indice buffer through a linked list
 */

int
gltf_mesh_load(cgltf_data *data, GfxBuffer *dest,
	       VkDrawIndexedIndirectCommand *indirect)
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
  printf("loading model [ ");
  for(size_t attrib_i = 0; attrib_i < primitive->attributes_count; attrib_i++){
    cgltf_attribute_type attribute = primitive->attributes[attrib_i].type;

    if(attribute == cgltf_attribute_type_position){
      pos_index = attrib_i;
      printf("vertices, ");
    }
      
    if(attribute == cgltf_attribute_type_normal){
      normal_index = attrib_i;
      printf("normals, ");
    }

    if(attribute == cgltf_attribute_type_color){
      printf("colours, ");
    }
    
    if(attribute == cgltf_attribute_type_texcoord){
      tex_index = attrib_i;
      printf("texture coords, ");
    }
 
  }
  printf(" ]\n");
  
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
  indirect->vertexOffset = dest->used_size / sizeof(vertex);
  vertex_buffer_append(vertices, dest, pos_accessor->count );
  /* Load indice data into int array */
  /* Then load that into a vulkan buffer */
  cgltf_accessor* indices_accessor =
    data->meshes[mesh_index].primitives[primitive_index].indices;

  cgltf_uint indices[indices_accessor->count];
  cgltf_accessor_unpack_indices
    (indices_accessor, indices, indices_accessor->count);

  GfxBuffer *dest_indices = (GfxBuffer*)dest->p_next;
  
  indirect->firstIndex = dest_indices->used_size / sizeof(uint32_t);
  indirect->indexCount = indices_accessor->count;

  index_buffer_append(indices, dest_indices, indices_accessor->count);

  
  return 0;
}

/* 
 * Public function for model loading, hides the cgltf_data format
 * through the functions 'gltf_mesh_load' and 'gltf_skin_load'
 */
int entity_gltf_load(GfxContext context, const char* filename,
		     GfxModelOffsets *model, GfxResources *resources){ 
  cgltf_data* data;
  cgltf_options options = {0};

  if(cgltf_parse_file(&options, filename, &data)
     != cgltf_result_success){
    printf("file missing!\n");
    return 1;
  }
  if(cgltf_load_buffers(&options, data, filename)
     != cgltf_result_success) return 1;
  if(cgltf_validate(data)
     != cgltf_result_success) return 1;
  
  VkDrawIndexedIndirectCommand indirect;
  if(gltf_mesh_load(data, &resources->geometry, &indirect)) return 2;

  model->indexCount = indirect.indexCount;
  model->firstIndex = indirect.firstIndex;
  model->vertexOffset = indirect.vertexOffset;

  int ti = textures_slot(*resources);
  gltf_skin_load(context, data, &resources->textures[ti]);
  model->textureIndex = ti;

  cgltf_free(data);
  
  return 0;
}
