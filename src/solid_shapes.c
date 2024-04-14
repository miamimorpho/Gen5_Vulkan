#include "solid.h"

#define FNL_IMPL
#include "../extern/FastNoiseLite.h"

// float  stb_perlin_noise3_seed( float x,
//                                float y,
//                                float z,
//                                int   x_wrap=0,
//                                int   y_wrap=0,
//                                int   z_wrap=0,
//                                int   seed)
//

int noise_distort(){
}

int normals_calc(){
  return 1;
}

void print_vec3(vec3 v){
  printf("%f, %f, %f\n", v[0], v[1], v[2]);
}

int make_plane(int width, int height, GfxBuffer* dest, Entity* indirect){

  /* units passed are quads
   *
   */

  
  // generate vertices
  int vertex_c = (width +1)  * (height +1); // count vertices, not quads
  vertex vertices[vertex_c];
  int index_c = width * height * 6;
  uint32_t indices[index_c];

  /*
  for(size_t i = 0; i < pos_accessor->count; i++){
    vertex v = {
      .pos = {pos_data[i * 3 ], pos_data[i * 3 +1 ], pos_data[i * 3 +2 ] },
      .normal =
      {normal_data[i * 3], normal_data[i * 3 + 1], normal_data[i * 3 + 2] },
      .tex = {tex_data[i * 2 ], tex_data[i * 2 +1 ] },
    };
    vertices[i] = v;
  */

  fnl_state noise = fnlCreateState();
  noise.noise_type = FNL_NOISE_OPENSIMPLEX2;
  
  for(int y = 0; y <= height; y++){
    for(int x = 0; x <= width; x++){
      vertex v = {
	.pos = {(float)x, (float)y, fnlGetNoise2D(&noise, x, y) * 2},
	.normal = {0.0f, 0.0f, 0.0f},
	.tex ={0.0f,0.0f},
	};
    vertices[y * (width +1) + x] = v;
    }
  }

  // generate indices
  for(int i = 0; i < index_c; i += 6){

    // find indices for a quad
    int topLeft = i / 6 + ( i / (6 * width)); // skip last vertex every row
    int topRight = topLeft + 1;
    int bottomLeft = topLeft + width + 1; // counting vertices, not quads
    int bottomRight = bottomLeft + 1;

    // triangle 1
    indices[i] = topLeft;
    indices[i +1] = bottomLeft;
    indices[i +2] = topRight;

    // triangle 2
    indices[i +3] = topRight;
    indices[i +4] = bottomLeft;
    indices[i +5] = bottomRight;
  }

  GfxBuffer *dest_indices = (GfxBuffer*)dest->p_next;
  indirect->firstIndex = dest_indices->used_size / sizeof(uint32_t);
  indirect->indexCount = index_c;
  indirect->vertexOffset = dest->used_size / sizeof(vertex);
  index_buffer_append(indices, dest_indices, index_c );

  // calculate normals
  for(int i = 0; i < index_c; i += 3){
    vec3 v1, v2, v3;
    glm_vec3_copy(vertices[indices[i]].pos, v1);
    glm_vec3_copy(vertices[indices[i+1]].pos, v2);
    glm_vec3_copy(vertices[indices[i+2]].pos, v3);
  
    vec3 edge1, edge2;
    glm_vec3_sub(v2, v1, edge1);
    glm_vec3_sub(v3, v1, edge2);
    
    vec3 fragNormal;
    glm_vec3_cross(edge1, edge2, fragNormal);
    //printf("normal =");
    //print_vec3(fragNormal);
    
    glm_vec3_add(vertices[indices[i]].normal,   fragNormal, vertices[indices[i]].normal);
    glm_vec3_add(vertices[indices[i+1]].normal, fragNormal, vertices[indices[i+1]].normal);
    glm_vec3_add(vertices[indices[i+2]].normal, fragNormal, vertices[indices[i+2]].normal);
  }
  
  for(int v = 0; v < vertex_c; v++){
    glm_vec3_normalize(vertices[v].normal);
  }

  vertex_buffer_append(vertices, dest, vertex_c );
  
  indirect->texture_index = 0;
  
  return 0;
}
