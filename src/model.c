#include "model.h"
#include "textures.h"

#define CGLTF_IMPLEMENTATION
#include "../extern/cgltf.h"

int
geometry_init(GfxContext context, GfxBuffer* geometry,
		       size_t estimated_size)
{
  int err;

  GfxBuffer* geometry_indices = (GfxBuffer*)malloc(sizeof(GfxBuffer));
  err = buffer_create(context, geometry_indices,
		      VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		      VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
		      estimated_size * sizeof(uint32_t) * 2 );
  vmaSetAllocationName(context.allocator, geometry_indices->allocation,
		       "geometry index buffer\n");

  
  err = buffer_create(context, geometry,
		      VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		      VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
		      estimated_size * sizeof(vertex3));
  vmaSetAllocationName(context.allocator, geometry->allocation,
		       "geometry vertex buffer\n");
  vmaSetAllocationUserData(context.allocator, geometry->allocation,
			   geometry_indices);

  return err;
}

int
geometry_load(GfxContext context,
	      GfxStagingMesh staging,
	      GfxBuffer* g, GfxModelOffsets* model){

  VmaAllocationInfo g_vertices_info;
  vmaGetAllocationInfo(context.allocator, g->allocation, &g_vertices_info);
  GfxBuffer* g_indices = (GfxBuffer*)g_vertices_info.pUserData;
  VmaAllocationInfo g_indices_info;
  vmaGetAllocationInfo(context.allocator, g_indices->allocation, &g_indices_info);
  
  size_t vb_size = staging.vertex_c * sizeof(vertex3);
  if(g_vertices_info.size < g->used_size + vb_size )return 1;
  
  size_t ib_size = staging.index_c * sizeof(uint32_t);
  if(g_indices_info.size < g_indices->used_size + ib_size )return 1;
  
  // we need to give offsets before we append to the geometry buffer
  model->firstIndex = g_indices->used_size / sizeof(uint32_t);
  model->indexCount = staging.index_c;
  model->vertexOffset = g->used_size / sizeof(vertex3);
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

int geometry_bind(GfxContext context, GfxBuffer g){

  VmaAllocationInfo g_vertices_info;
  vmaGetAllocationInfo(context.allocator, g.allocation, &g_vertices_info);
  GfxBuffer* g_indices = (GfxBuffer*)g_vertices_info.pUserData;

  VkBuffer vertex_buffers[] = {g.handle};
  VkDeviceSize offsets[] = {0};
  vkCmdBindVertexBuffers(context.command_buffer, 0, 1, vertex_buffers, offsets);
  vkCmdBindIndexBuffer(context.command_buffer, g_indices->handle,
		       0, VK_INDEX_TYPE_UINT32);

  return 0;

}

/* error 1 file format */
int gltf_skin_load(GfxContext context, cgltf_data* file, uint32_t* index){
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
  texture_memory_load(context, raw_pixels, size, index);
 
  free(raw_pixels);
  
  return 0;
}

/* GfxBuffer *dest is a vertice buffer that is linked
 * with the indice buffer through a linked list
 */

int
gltf_mesh_load(cgltf_data* data, GfxStagingMesh* m)
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
  
  m->vertex_c = pos_accessor->count;
  m->vertices = (vertex3*)malloc( m->vertex_c * sizeof(vertex3));
  for(size_t i = 0; i < pos_accessor->count; i++){
    vertex3 v = {
      .pos = {pos_data[i * 3 ], pos_data[i * 3 +1 ], pos_data[i * 3 +2 ] },
      .normal =
      {normal_data[i * 3], normal_data[i * 3 + 1], normal_data[i * 3 + 2] },
      .uv = {tex_data[i * 2 ], tex_data[i * 2 +1 ] },
    };
    m->vertices[i] = v;
  }
 
  cgltf_accessor* indices_accessor =
    data->meshes[mesh_index].primitives[primitive_index].indices;

  m->index_c = indices_accessor->count;
  m->indices = (uint32_t*)malloc( m->index_c * sizeof(uint32_t));
  cgltf_accessor_unpack_indices
    (indices_accessor, m->indices, m->index_c);

  return 0;
}

/* 
 * Public function for model loading, hides the cgltf_data format
 * through the functions 'gltf_mesh_load' and 'gltf_skin_load'
 */
int gltf_load(GfxContext context, const char* filename,
		     GfxModelOffsets *model, GfxBuffer *geometry){ 
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

  GfxStagingMesh mesh;
  if(gltf_mesh_load(data, &mesh)) return 2;

  geometry_load(context, mesh, geometry, model);
  
  gltf_skin_load(context, data, &model->textureIndex);

  cgltf_free(data);
  
  return 0;
}
