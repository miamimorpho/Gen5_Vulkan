#include "main.h"

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
  
  GfxModelOffsets car_m, world_m;
    make_cube(&resources.geometry, &world_m);
  world_m.textureIndex = png_load(context, "textures/dune.png", &resources);
  entity_gltf_load(context, "models/CarBroken.glb", &car_m, &resources);
  car_m.textureIndex = world_m.textureIndex;
  
  int entity_c = 2;
  Entity entities[entity_c];
  printf("car ti: %d\n", car_m.textureIndex);
  entities[0] = entity_add1(car_m, 0, 0, 0);
  entities[1] = entity_add1(world_m, 5, 5,1);
  
  texture_descriptors_update(context, resources, entity_c);
  draw_descriptors_update(context, resources, entity_c);

  while(main_input(context, &camera)){
    
    draw_start(&context, pipeline);

    resources_bind(context, pipeline, resources);
    
    for(int i = 0; i < entity_c; i++)
      render_entity(context, camera, entities[i]);
  
    draw_end(context, pipeline, entity_c);

  }
  
  VkFence fences[] = { pipeline.in_flight};
  vkWaitForFences(context.l_dev, 1, fences, VK_TRUE, UINT64_MAX);

  printf("free textures\n");
  unused_textures_free(context, entities, entity_c, resources.textures);

  draw_descriptors_free(context);

  printf("free indirect\n");
  printf("free resources\n");
  resources_destroy(context, resources);

  
  pipeline_destroy(context, pipeline);
  context_destroy(context);
    
  return 0;
  }
