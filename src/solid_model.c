#include "vulkan_public.h"
#include "solid.h"

#define CGLTF_IMPLEMENTATION
#include "../extern/cgltf.h"

static uint32_t TEXTURES_USED = 0;

int
models_destroy(GfxContext context, uint32_t count,
	       GfxBuffer geometry, ImageData *textures)
{
  /* once we have done textures and world mesh, we should come
   * up with a generic way to create/destroy models with no
   * memory leaks */
  buffers_destroy(context, (GfxBuffer*)geometry.p_next, 1);
  buffers_destroy(context, &geometry, 1);
  
  for(uint32_t i = 0; i < count; i++){
    image_destroy(context, &textures[i]);
  }
  
  return 1;
}

int
geometry_buffer_bind(VkCommandBuffer commands, GfxBuffer geometry){
  VkBuffer vertex_buffers[] = {geometry.handle};
  VkDeviceSize offsets[] = {0};

  GfxBuffer *indices = (GfxBuffer*)geometry.p_next;
  vkCmdBindVertexBuffers(commands, 0, 1, vertex_buffers, offsets);
  vkCmdBindIndexBuffer(commands, indices->handle,
		       0, VK_INDEX_TYPE_UINT32);

  return 0;
}

int
geometry_buffer_create(GfxContext context, int count, GfxBuffer *geometry)
{
  GfxBuffer *vertices = (GfxBuffer*)malloc(sizeof(GfxBuffer));
  GfxBuffer *indices = (GfxBuffer*)malloc(sizeof(GfxBuffer));

  
  //GfxBuffer *vertices;
  buffer_create(context,
		count * sizeof(vertex),
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		vertices);

  //GfxBuffer *indices;
  buffer_create(context,
		count * sizeof(uint32_t),
		VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		indices);

  *geometry = *vertices;
  geometry->p_next = (void*)indices;
  
  return 0;
}

int
vertex_buffer_append(const vertex *vertices, GfxBuffer *dest, uint32_t count ){
  return buffer_append(vertices, dest, count * sizeof(vertex) );
}

int
index_buffer_append(const uint32_t *indices, GfxBuffer *dest, uint32_t count){
  return buffer_append(indices, dest, count * sizeof(uint32_t) );
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
  unsigned char *data = ((unsigned char *)view->buffer->data);
  
  image_file_load(context, data, offset, view->size, stride, texture);

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
	      GfxBuffer *geometry, ImageData *textures,
	      Entity* components)
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
  
  VkDrawIndexedIndirectCommand indirect;
  if(gltf_mesh_load(data, geometry, &indirect)) return 2;

  components->indices_len = indirect.indexCount;
  components->first_index = indirect.firstIndex;
  components->vertex_offset = indirect.vertexOffset;
  
  if(gltf_skin_load(context, data, &textures[TEXTURES_USED])) return 3;
  components->texture_index = TEXTURES_USED;
  TEXTURES_USED += 1;

  cgltf_free(data);
  
  return 0;
}

Entity
entity_add1(Entity mother, float x, float y, float z){

  Entity child;
  glm_vec3_copy( (vec3){x, y, z}, child.pos);
  glm_quat(child.rotate, 0.0f, 0.0f, 0.0f, -1.0f);
  
  child.indices_len = mother.indices_len;
  child.first_index = mother.first_index;
  child.vertex_offset = mother.vertex_offset;
  child.texture_index = mother.texture_index;
  return child;
}
