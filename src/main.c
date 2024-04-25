#include "main.h"

int render_text(GfxFont font, char* string,
		GfxBuffer* dest, GfxModelOffsets* model){

  float ratio = (float)font.glyph_width / (float)font.height;
  printf("%f\n", ratio);
  
  int len = strlen(string);
  char* c = string;

  size_t vertex_c = len * 4;
  size_t index_c = len * 6;
  vertex* vertices = (vertex*)malloc(sizeof(vertex) * vertex_c);
  uint32_t* indices = (uint32_t*)malloc(sizeof(uint32_t) * index_c);
  for(int x = 0; x < len; x++){
    
    int atlas_width = font.glyph_c * font.glyph_width;
    vertex top_left = {
      .pos = { (float)x * ratio, 0.0f, -1.0f },
      .normal = { 1.0f, 1.0f, 1.0f},
      .uv = { (float)(*c * font.glyph_width) / (float)atlas_width, 0}
    };

    vertex bottom_left = top_left;
    bottom_left.pos[2] = 0; // Z
    bottom_left.uv[1] = 1; // Y
    
    vertex top_right = top_left;
    top_right.pos[0] += ratio; // X
    top_right.uv[0] += (float)font.glyph_width / (float)atlas_width; //X

    vertex bottom_right = top_right;
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

  geometry_load(vertices, vertex_c, indices, index_c, dest, model);
  
  free(vertices);
  free(indices);

  return 0;
}

int
main(void)
{
  GfxContext context;
  context_create(&context);

  GfxResources resources;
  resources_init(context, &resources);
  
  GfxPipeline pipeline;
  pipeline_create(context, resources, &pipeline);

  Camera camera = camera_create(WIDTH, HEIGHT);
  
  GfxFont font;
  bdf_load(&font, "textures/spleen-8x16-437-custom.bdf");

  int ti = textures_slot(resources);
  int font_atlas_i = texture_load(context, font.pixels, &resources.textures[ti],
         font.glyph_c * font.glyph_width, font.height, 1);

  GfxModelOffsets car_m;
  entity_gltf_load(context, "models/CarBroken.glb", &car_m, &resources);

  GfxModelOffsets world_m;
  make_plane(128, 128, &resources.geometry, &world_m);
  world_m.textureIndex = png_load(context, "textures/dune.png", &resources);

  GfxModelOffsets text_m;
  render_text(font, "I love my girlfriend!", &resources.geometry, &text_m);
  
  int entity_c = 3;
  Entity entities[entity_c];
  entities[0] = entity_add1(car_m, 12, 12, 0);
  entities[1] = entity_add1(world_m, 5, 5,1);
  entities[2] = entity_add1(text_m, 0, 0, 0);
  
  texture_descriptors_update(context, resources, entity_c);
  draw_descriptors_update(context, resources, entity_c);

  /* START OF DRAW LOOP */
  while(main_input(context, &camera)){
    
    draw_start(&context, pipeline);
   
    resources_bind(context, pipeline, resources);

    geometry_bind(pipeline, resources.geometry);
    for(int i = 0; i < entity_c; i++)
      render_entity(context, camera, entities[i]);

   
    draw_end(context, pipeline, entity_c);

  } /* END OF DRAW LOOP */
  
  VkFence fences[] = { pipeline.in_flight};
  vkWaitForFences(context.l_dev, 1, fences, VK_TRUE, UINT64_MAX);

  draw_descriptors_free(context);

  font_free(&font);
  resources_destroy(context, resources);
  
  pipeline_destroy(context, pipeline);
  context_destroy(context);
    
  return 0;
  }
