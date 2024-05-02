#include "shapes.h"
#include "model.h"

#define FNL_IMPL

// we can build a quad by defining the polarity of its axis
int make_cube(GfxContext context, GfxBuffer* dest, GfxModelOffsets* model)
{
  
vertex3 vertices[24] = {
  { { -0.500000, -0.500000, 0.500000 }, { 0.000000, 0.000000, 1.000000 }, { 6.000000, 0.000000},},
  { { 0.500000, -0.500000, 0.500000 }, { 0.000000, 0.000000, 1.000000 }, { 5.000000, 0.000000},},
  { { -0.500000, 0.500000, 0.500000 }, { 0.000000, 0.000000, 1.000000 }, { 6.000000, 1.000000},},
  { { 0.500000, 0.500000, 0.500000 }, { 0.000000, 0.000000, 1.000000 }, { 5.000000, 1.000000},},
  { { 0.500000, 0.500000, 0.500000 }, { 1.000000, 0.000000, 0.000000 }, { 4.000000, 0.000000},},
  { { 0.500000, -0.500000, 0.500000 }, { 1.000000, 0.000000, 0.000000 }, { 5.000000, 0.000000},},
  { { 0.500000, 0.500000, -0.500000 }, { 1.000000, 0.000000, 0.000000 }, { 4.000000, 1.000000},},
  { { 0.500000, -0.500000, -0.500000 }, { 1.000000, 0.000000, 0.000000 }, { 5.000000, 1.000000},},
  { { -0.500000, 0.500000, 0.500000 }, { 0.000000, 1.000000, 0.000000 }, { 2.000000, 0.000000},},
  { { 0.500000, 0.500000, 0.500000 }, { 0.000000, 1.000000, 0.000000 }, { 1.000000, 0.000000},},
  { { -0.500000, 0.500000, -0.500000 }, { 0.000000, 1.000000, 0.000000 }, { 2.000000, 1.000000},},
  { { 0.500000, 0.500000, -0.500000 }, { 0.000000, 1.000000, 0.000000 }, { 1.000000, 1.000000},},
  { { 0.500000, -0.500000, 0.500000 }, { 0.000000, -1.000000, 0.000000 }, { 3.000000, 0.000000},},
  { { -0.500000, -0.500000, 0.500000 }, { 0.000000, -1.000000, 0.000000 }, { 4.000000, 0.000000},},
  { { 0.500000, -0.500000, -0.500000 }, { 0.000000, -1.000000, 0.000000 }, { 3.000000, 1.000000},},
  { { -0.500000, -0.500000, -0.500000 }, { 0.000000, -1.000000, 0.000000 }, { 4.000000, 1.000000},},
  { { -0.500000, -0.500000, 0.500000 }, { -1.000000, 0.000000, 0.000000 }, { 3.000000, 0.000000},},
  { { -0.500000, 0.500000, 0.500000 }, { -1.000000, 0.000000, 0.000000 }, { 2.000000, 0.000000},},
  { { -0.500000, -0.500000, -0.500000 }, { -1.000000, 0.000000, 0.000000 }, { 3.000000, 1.000000},},
  { { -0.500000, 0.500000, -0.500000 }, { -1.000000, 0.000000, 0.000000 }, { 2.000000, 1.000000},},
  { { -0.500000, -0.500000, -0.500000 }, { 0.000000, 0.000000, -1.000000 }, { 0.000000, 0.000000},},
  { { -0.500000, 0.500000, -0.500000 }, { 0.000000, 0.000000, -1.000000 }, { 0.000000, 1.000000},},
  { { 0.500000, -0.500000, -0.500000 }, { 0.000000, 0.000000, -1.000000 }, { 1.000000, 0.000000},},
  { { 0.500000, 0.500000, -0.500000 }, { 0.000000, 0.000000, -1.000000 }, { 1.000000, 1.000000},},
};
 uint32_t indices[36] = {
   0,1,2,3,2,1,4,5,6,7,6,5,8,9,10,11,10,9,12,13,14,15,14,13,16,17,18,19,18,17,20,21,22,23,22,21,};

 GfxStagingMesh staging = {
   .vertices = vertices,
   .vertex_c = sizeof(vertices)/sizeof(vertex3),
   .indices = indices,
   .index_c = sizeof(indices)/sizeof(uint32_t),
 };
   
 geometry_load(context, staging, dest, model);

 return 0;
}

int generate_normals(vertex3* vertices, uint32_t* indices, int vertex_c, int index_c){

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
  
  return 1;
}

int make_plane(GfxContext context, int width, int height, GfxBuffer* geometry,
	       GfxModelOffsets* model){

  int vertex_c = (width +1)  * (height +1); // count vertices, not quads
  vertex3* vertices;
  vertices = (vertex3*)malloc(vertex_c * sizeof(vertex3));

  for(int y = 0; y <= height; y++){
    for(int x = 0; x <= width; x++){
      vertex3 v = {
	.pos = {(float)x -(width/2), (float)y -(height/2), 0},
	.normal = {0.0f, 0.0f, 0.0f},
	.uv ={(float)x / (float)width, (float)y / (float)height},
	};
    vertices[y * (width +1) + x] = v;
    }
  }

  // generate indices
  int index_c = width * height * 6;
  uint32_t* indices;
  indices = (uint32_t*)malloc(index_c * sizeof(uint32_t));

  for(int i = 0; i < index_c; i += 6){

    // find indices for a quad
    int topLeft = i / 6 + ( i / (6 * width)); // skip last vertex every row
    int topRight = topLeft + 1;
    int bottomLeft = topLeft + width + 1; // counting vertices, not quads
    int bottomRight = bottomLeft + 1;

    indices[i] = topLeft; // triangle 1
    indices[i +1] = bottomLeft;
    indices[i +2] = topRight;
 
    indices[i +3] = topRight; // triangle 2
    indices[i +4] = bottomLeft;
    indices[i +5] = bottomRight;
  }

  generate_normals(vertices, indices, vertex_c, index_c);

  GfxStagingMesh staging = {
    .vertices = vertices,
    .vertex_c = vertex_c,
    .indices = indices,
    .index_c = index_c,
  };
  
  geometry_load(context, staging, geometry, model);

  free(vertices);
  free(indices);
  
  return 0;
}
