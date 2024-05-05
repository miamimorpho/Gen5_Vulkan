#include "main.h"

int
main(void)
{
  /*-- Create Window */
  GfxContext context;
  if(context_create(&context) != 0)printf("serious error");

  /*-- Textures */
  textures_init(&context);

  /* Text Rendering + UI */
  gui_shader_create(context);
  GfxBuffer ui_mesh_b;
  mesh_b_create(context, &ui_mesh_b, 10000);
  
  GfxFont font;
  gfx_font_load(context, &font, "textures/spleen-8x16-437-custom.bdf");

  /* 3D model shader */
  model_shader_t model_shader;
  model_shader_create(context, &model_shader);
  GfxBuffer model_mesh_b;
  mesh_b_create(context, &model_mesh_b, 100000);
  
  /*-- Camera */
  Camera camera = camera_create(WIDTH, HEIGHT);

  /* Load 3D Resources */
  int model_c = 2;
  Entity entities[model_c];

  GfxModelOffsets world_m;
  make_plane(context, 128, 128, &model_mesh_b, &world_m);
  texture_file_load(context, "textures/dune.png", &world_m.textureIndex);
  entities[0] = entity_add1(world_m, 0, 5, 0); 

  GfxModelOffsets car_m;
  gltf_load(context, "models/Car2.glb", &car_m, &model_mesh_b);
  entities[1] = entity_add1(car_m, 0, 5, 0);
    
  /*-- prepare descriptors for scene */
  texture_descriptors_update(context, 3);
  //int text_index_c = text_render(context, font, "hello world", &ui_mesh_b);
  model_descriptors_update(context, &model_shader, model_c);
  
  /* START OF DRAW LOOP */
  int running = 1;
  int text_index_c = 5;
  while(running && main_input(context, &camera)){

    char char_buffer[200];
    snprintf(char_buffer, 200, "X%.0f Y%.0f Z%.0f", camera.pos[0], camera.pos[1], camera.pos[2] * -1);
    text_index_c = text_render(context, font, char_buffer, &ui_mesh_b);
    
    draw_start(&context);

    vkCmdBindDescriptorSets(context.command_buffer, 
			  VK_PIPELINE_BIND_POINT_GRAPHICS,
			  model_shader.pipeline_layout, 0, 1,
			  &context.texture_descriptors, 0, NULL);

    mesh_b_bind(context, model_mesh_b);    
    model_shader_bind(context, model_shader);
    model_shader_draw(context, model_shader, camera, entities, model_c);
      
    gui_shader_bind(context);
    mesh_b_bind(context, ui_mesh_b);
    
    vkCmdDrawIndexed(context.command_buffer, text_index_c, 1, 0, 0, 0);
    ui_mesh_b.used_size = 0;
    GfxBuffer* ui_indices_b = buffer_next(context, ui_mesh_b);
    ui_indices_b->used_size = 0;
      
    draw_end(context);

  } /* END OF DRAW LOOP */
  
  VkFence fences[] = { context.in_flight};
  vkWaitForFences(context.l_dev, 1, fences, VK_TRUE, UINT64_MAX);
  vkResetCommandBuffer(context.command_buffer, 0);

  deferd_run(context);

  model_shader_destroy(context, &model_shader);
 
  context_destroy(context);
    
  return 0;
  }
