#include "solid.h"
#include <SDL2/SDL.h>

Entity
entity_duplicate(Entity mother, float x, float y){

  Entity child;
  glm_vec3_copy( (vec3){x, y, -1.0f}, child.pos);
  child.indices_len = mother.indices_len;
  child.first_index = mother.first_index;
  child.vertex_offset = mother.vertex_offset;
  child.texture_index = mother.texture_index;
  return child;
}

int
main(void)
{
  /* SDL+Vulkan Init */
  GfxContext context;
  stage1_create(&context);

  /* Fixed Pipeline Creation */
  GfxPipeline pipeline;
  stage2_create(context, &pipeline);

  /* Stage 3 Descriptor Sets Creation */
  descriptor_pool_create(context, &pipeline.descriptor_pool);
  slow_descriptors_alloc(context.l_dev, &pipeline);
  rapid_descriptors_alloc(context, &pipeline);
  
  Camera camera = camera_create(context);
  
  GeometryData geo = geometry_buffer_create(context);
  ImageData textures[2];
  Entity car;
  gltf_load(context, "models/CarBroken.glb",
	    &geo, textures, &car);
  Entity car2;
  gltf_load(context, "models/Car2.glb",
	    &geo, textures, &car2);

  int entity_c = 2;
  Entity entities[entity_c];
  entities[0] = entity_duplicate(car, 0, 0);
  entities[1] = entity_duplicate(car2, 10, 0);
  
  /*
  int entity_c = 100;
  Entity entities[entity_c];
  for(int x = 0; x < 10; x++){
    for(int y = 0; y < 10; y++){
      entities[(x * 10)+y] = entity_copy(car, x * 10, y * 10);
    }
  }
  */
 
  slow_descriptors_update(context, 2, textures, &pipeline);
  
  DeviceBuffer draw_args[context.frame_c];
  draw_args_create(context, pipeline, entity_c, draw_args);

  DeviceBuffer indirect_b;
  indirect_buffer_create(context, entity_c, &indirect_b);

  /* Mouse Data */
  double xvel = 0, yvel = 0;
  double speed = 0.2;
  float sensitivity = 0.05f;
  SDL_SetRelativeMouseMode(SDL_TRUE);

  /* Main rendering loop */
  int running = 1;
  SDL_Event event;
  while(running > 0){
    SDL_PollEvent( &event );
    
    switch( event.type ) {
      
    case SDL_KEYDOWN:
      switch( event.key.keysym.sym) {
      case SDLK_LEFT:
	xvel = speed;
	break;
      case SDLK_RIGHT:
	xvel = -speed;
	break;
      case SDLK_UP:
	yvel = speed;
	break;
      case SDLK_DOWN:
	yvel = -speed;
	break;
      default:
	break;
      } break;
   
    case SDL_KEYUP:      
      switch( event.key.keysym.sym){
      case SDLK_LEFT:
	if( xvel > 0 ) xvel = 0;
	break;
      case SDLK_RIGHT:
       	if( xvel < 0 ) xvel = 0;
	break;
      case SDLK_UP:
	if( yvel > 0 ) yvel = 0;
	break;
      case SDLK_DOWN:
	if( yvel < 0 ) yvel = 0;
	break;
      default:
	break;
      } break;

    case SDL_MOUSEMOTION:
      camera.pitch += event.motion.yrel * sensitivity;
      camera.yaw -= event.motion.xrel * sensitivity;
      break;
      
    case SDL_QUIT:
      running = 0;
      break;

    default:
      break;
  
    }
    
    camera.pos[0] += xvel;
    camera.pos[1] += yvel;
    
    draw_start(&context, pipeline, geo);
    
    /* Slow / Global Data */
    vkCmdBindDescriptorSets(pipeline.command_buffer,
			    VK_PIPELINE_BIND_POINT_GRAPHICS,
			    pipeline.layout, 0, 1,
			    &pipeline.slow_set, 0, NULL);

    /* Rapid / Per-frame data */
    draw_args_update(camera, entities,
		     draw_args[context.frame_index] );
    vkCmdBindDescriptorSets(pipeline.command_buffer,
			    VK_PIPELINE_BIND_POINT_GRAPHICS,
			    pipeline.layout, 1, 1,
			    &pipeline.rapid_sets[context.frame_index],0, NULL);

    /* ultimate draw call */
    indirect_buffer_update(indirect_b, entities);
    vkCmdDrawIndexedIndirect(pipeline.command_buffer, indirect_b.handle, 0,
			     entity_c, sizeof(VkDrawIndexedIndirectCommand));
    
    draw_end(context, pipeline);

    //    running = 0;
    }
  /* Main rendering loop end */
  
  /* Cleanup */
  VkFence fences[] = { pipeline.in_flight};
  vkWaitForFences(context.l_dev, 1, fences, VK_TRUE, UINT64_MAX);
  models_destroy(context, entity_c, &geo, textures);
  buffers_destroy(context, 1, &indirect_b);
  buffers_destroy(context, context.frame_c, draw_args);
  stage2_destroy(context, pipeline);
  stage1_destroy(context);
    
  return 0;
  }
