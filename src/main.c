#include "main.h"

int
main(void)
{
  /*-- Create Window */
  GfxContext context;
  if(context_create(&context) != 0)printf("serious error");

  /*-- Textures */
  textures_init(&context);

  GfxShader model_shader;
  model_shader_create(context, &model_shader);
  
  GfxShader ui;
  text_shader_create(context, &ui);

  /*-- Vertex + Index Buffers */
  GfxBuffer geometry;
  geometry_init(context, &geometry, 100000);

  /*-- Camera */
  Camera camera = camera_create(WIDTH, HEIGHT);
  GfxFont font;
  gfx_font_load(context, &font, "textures/spleen-8x16-437-custom.bdf");

  int total_entity_c = 3;
  
  int text_count = 1;
  GfxModelOffsets text_m;
  text_render(context, font, "hello world", &geometry, &text_m);
  text_entity texts[text_count];
  glm_vec2_copy((vec2){0,0}, texts[0].pos);
  texts[0].model = text_m;

  int model_c = 2;
  Entity entities[model_c];

  GfxModelOffsets world_m;
  make_plane(context, 128, 128, &geometry, &world_m);
  texture_file_load(context, "textures/dune.png", &world_m.textureIndex);
  entities[0] = entity_add1(world_m, 0, 5, 0); 

  GfxModelOffsets car_m;
  gltf_load(context, "models/Car2.glb", &car_m, &geometry);
  entities[1] = entity_add1(car_m, 0, 5, 0);
    
  /*-- prepare descriptors for scene */
  texture_descriptors_update(context, total_entity_c);

  indirect_b_create(context, &model_shader, model_c);
  text_indirect_b_create(context, &ui, text_count);
  
  /* START OF DRAW LOOP */
  int running = 1;
  while(running && main_input(context, &camera)){
    
    draw_start(&context);

    pipeline_bind(context, model_shader);

    vkCmdBindDescriptorSets(context.command_buffer,
			    VK_PIPELINE_BIND_POINT_GRAPHICS,
			    model_shader.pipeline_layout, 1, 1,
			    &model_shader.descriptors[context.current_frame_index],
			    0, NULL);
    geometry_bind(context, geometry);

    textures_bind(context, model_shader);

    draw_entities(context, model_shader, camera, entities, model_c);
      
    pipeline_bind(context, ui);

    vkCmdBindDescriptorSets(context.command_buffer,
			    VK_PIPELINE_BIND_POINT_GRAPHICS,
			    ui.pipeline_layout, 1, 1,
			    &ui.descriptors[context.current_frame_index],
			    0, NULL);

    geometry_bind(context, geometry);
    
    textures_bind(context, ui);
    
    text_draw(context, ui, texts, text_count);

    draw_end(context);

  } /* END OF DRAW LOOP */
  
  VkFence fences[] = { context.in_flight};
  vkWaitForFences(context.l_dev, 1, fences, VK_TRUE, UINT64_MAX);
  vkResetCommandBuffer(context.command_buffer, 0);

  textures_free(context);
  //font_free(&font);
  
  pipeline_destroy(context, ui);

  buffer_destroy(context, &geometry, 1);
  
  context_destroy(context);
    
  return 0;
  }
