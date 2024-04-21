#include "main.h"

int
main(void)
{
  /* GLFW Init */
  GfxContext context;
  context_create(&context);

  /* Resources Descriptor Sets Creation */
  GfxResources resources;
  resources_init(context, &resources);
  
  /* Fixed Pipeline Creation */
  GfxPipeline pipeline;
  pipeline_create(context, resources, &pipeline);

  /* Camera Controls Init */
  Camera camera = camera_create(WIDTH, HEIGHT);
  SDL_SetRelativeMouseMode(SDL_TRUE);
  
  /* Load gltf data to memory */
  GfxModelOffsets car_m, world_m;
  
  make_cube(&resources.geometry, &world_m);
  world_m.textureIndex = png_load(context, "textures/dune.png", &resources);
  entity_gltf_load(context, "models/CarBroken.glb", &car_m, &resources);
 
  /* Init Entity Components */
  int entity_c = 2;
  Entity entities[entity_c];
  printf("car ti: %d\n", car_m.textureIndex);
  entities[0] = entity_add1(car_m, 0, 0, 0);
  entities[1] = entity_add1(world_m, 5, 5,1);
  
  /* Start Rendering */
  texture_descriptors_update(context, entity_c, resources);
  
  GfxBuffer draw_descriptors[context.frame_c];
  draw_descriptors_create(context, resources, entity_c, draw_descriptors);

  GfxBuffer indirect;
  draw_indirect_create(context, &indirect, entity_c);
  
  /* Main rendering loop */
  int running = 1;
  while(running){

    running = main_input(context, &camera);
    
    /* Reset command buffer, set initial values */
    draw_start(&context, pipeline);

    /* Update Draw Arguments */
    draw_descriptors_update(camera, entities,
		     draw_descriptors[context.frame_index] );

    draw_indirect_update(indirect, entities);

    /* Bind geometry */
    geometry_buffer_bind(pipeline.command_buffer, resources.geometry);

    /* Bind sampler array */
    vkCmdBindDescriptorSets(pipeline.command_buffer, 
			    VK_PIPELINE_BIND_POINT_GRAPHICS,
			    pipeline.layout, 0, 1,
			    &resources.slow_set, 0, NULL);

    /* Bind draw arguments array */
    vkCmdBindDescriptorSets(pipeline.command_buffer,
			    VK_PIPELINE_BIND_POINT_GRAPHICS,
			    pipeline.layout, 1, 1,
			    &resources.rapid_sets[context.frame_index],0, NULL);

    /* Final draw call */
    vkCmdDrawIndexedIndirect(pipeline.command_buffer, indirect.handle, 0,
			     entity_c, sizeof(VkDrawIndexedIndirectCommand));
    
    draw_end(context, pipeline);

    //    running = 0;
    }
  /* Main rendering loop end */
  
  /* Cleanup */
  VkFence fences[] = { pipeline.in_flight};
  vkWaitForFences(context.l_dev, 1, fences, VK_TRUE, UINT64_MAX);

  printf("free textures\n");
  unused_textures_free(context, entities, entity_c, resources.textures);

  printf("free indirect\n");
  buffers_destroy(context, &indirect, 1);
  printf("free resources\n");
  resources_destroy(context, resources);

  buffers_destroy(context, draw_descriptors, context.frame_c);
  
  pipeline_destroy(context, pipeline);
  context_destroy(context);
    
  return 0;
  }
