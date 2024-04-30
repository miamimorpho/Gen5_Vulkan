#include "main.h"

int
main(void)
{
  /*-- Create Window */
  GfxContext context;
  if(context_create(&context) != 0)printf("serious error");

  /*-- Textures */
  textures_init(&context);

  /*-- 3D Pipeline */
  GfxShader ps1_shader;
  ps1_pipeline_create(context, &ps1_shader);

  /*-- Vertex + Index Buffers */
  GfxBuffer geometry;
  geometry_init(context, &geometry, 100000);

  /*-- Camera */
  Camera camera = camera_create(WIDTH, HEIGHT);
  //GfxFont font;
  //gfx_font_create(context, "textures/spleen-8x16-437-custom.bdf", &font);

  /*-- Model Loading */
  GfxModelOffsets car_m;
  entity_gltf_load(context, "models/CarBroken.glb", &car_m, &geometry);

  GfxModelOffsets world_m;
  make_plane(context, 128, 128, &geometry, &world_m);
  texture_file_load(context, "textures/dune.png", &world_m.textureIndex);
  
  int entity_c = 2;
  Entity entities[entity_c];
  entities[0] = entity_add1(car_m, 12, 12, 0);
  entities[1] = entity_add1(world_m, 5, 5,1);

  /*-- prepare descriptors for scene */
  texture_descriptors_update(context, entity_c);
  draw_indirect_init(context, ps1_shader.descriptors, entity_c);
  
  /* START OF DRAW LOOP */
  int running = 1;
  while(running && main_input(context, &camera)){
    
    draw_start(&context, ps1_shader);

    pipeline_bind(context, ps1_shader);

    vkCmdBindDescriptorSets(context.command_buffer,
			    VK_PIPELINE_BIND_POINT_GRAPHICS,
			    ps1_shader.pipeline_layout, 1, 1,
			    &ps1_shader.descriptors[context.current_frame_index],
			    0, NULL);

    geometry_bind(context, geometry);
    
    textures_bind(context, ps1_shader);
    
    draw_entities(context, camera, entities, entity_c);

    draw_end(context, entity_c, ps1_shader);

  } /* END OF DRAW LOOP */
  
  VkFence fences[] = { ps1_shader.in_flight};
  vkWaitForFences(context.l_dev, 1, fences, VK_TRUE, UINT64_MAX);
  vkResetCommandBuffer(context.command_buffer, 0);

  textures_free(context);
  
  draw_indirect_free(context, &ps1_shader);

  //font_free(&font);
  
  pipeline_destroy(context, ps1_shader);

  buffer_destroy(context, &geometry, 1);
  
  context_destroy(context);
    
  return 0;
  }
