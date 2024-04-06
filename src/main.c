#include "main.h"

int
main(void)
{
  /* SDL+Vulkan Init */
  GfxContext context;
  stage1_create(&context);

  /* Fixed Pipeline Creation */
  GfxPipeline pipeline;
  stage2_create(context, &pipeline);

  /* Stage 3: Descriptor Sets Creation */
  slow_descriptors_alloc(context.l_dev, &pipeline);
  rapid_descriptors_alloc(context, &pipeline);

  /* Camera Controls Init */
  Camera camera = camera_create(WIDTH, HEIGHT);
  double front_vel = 0, strafe_vel = 0, speed = 0.2;
  float pitch_vel = 0, yaw_vel = 0, sensitivity = 0.005;
  SDL_SetRelativeMouseMode(SDL_TRUE);
  
  /* Memory Allocation */
  int model_count = 2;
  GfxBuffer geo_data;
  geometry_buffer_create(context, 30000, &geo_data);
  ImageData textures[model_count];

  /* Load gltf data to memory */
  Entity car, car2;
  entity_gltf_load(context, "models/CarBroken.glb",
	    &geo_data, textures, &car);
  entity_gltf_load(context, "models/Car2.glb",
	    &geo_data, textures, &car2);
  
  /*
  int entity_c = 100;
  Entity entities[entity_c];
  for(int x = 0; x < 10; x++){
    for(int y = 0; y < 10; y++){
      entities[(x * 10)+y] = entity_duplicate(car, x * 20, y * 20, 0);
    }
  }
  */

  int entity_c = 2;
  Entity entities[entity_c];
  entities[0] = entity_duplicate(car, 5, 5, 0);
  entities[1] = entity_duplicate(car2, 10, 5, 0);

  /* Start Rendering */
  slow_descriptors_update(context, model_count, textures, &pipeline);
  
  GfxBuffer indirect_args[context.frame_c];
  draw_args_create(context, pipeline, entity_c, indirect_args);

  GfxBuffer indirect_draw;
  draw_indirect_create(context, &indirect_draw, entity_c);

  /* Main rendering loop */
  int running = 1;
  SDL_Event event;
  while(running > 0){
    SDL_PollEvent( &event );
    
    switch( event.type ) {
      
    case SDL_KEYDOWN:
      switch( event.key.keysym.sym) {
      case SDLK_LEFT:
	strafe_vel = -speed;
	break;
      case SDLK_RIGHT:
	strafe_vel = speed;
	break;
      case SDLK_UP:
	front_vel = speed;
	break;
      case SDLK_DOWN:
	front_vel = -speed;
	break;
      default:
	break;
      } break;
   
    case SDL_KEYUP:      
      switch( event.key.keysym.sym){
      case SDLK_LEFT:
	if( strafe_vel < 0 ) strafe_vel = 0;
	break;
      case SDLK_RIGHT:
       	if( strafe_vel > 0 ) strafe_vel = 0;
	break;
      case SDLK_UP:
	if( front_vel > 0 ) front_vel = 0;
	break;
      case SDLK_DOWN:
	if( front_vel < 0 ) front_vel = 0;
	break;
      default:
	break;
      } break;

    case SDL_MOUSEMOTION:
      yaw_vel = event.motion.xrel * sensitivity;
      pitch_vel = event.motion.yrel * sensitivity;
      break;
      
    case SDL_QUIT:
      running = 0;
      break;

    default:
      break;
  
    }

    /* Camera Movement */
    camera_rotate(&camera, yaw_vel, pitch_vel);
    pitch_vel = 0;
    yaw_vel = 0;

    /* Player Movement */
    camera.pos[0] += camera.front[0] * front_vel;
    camera.pos[1] += camera.front[1] * front_vel;
    camera.pos[0] += camera.right[0] * strafe_vel;
    camera.pos[1] += camera.right[1] * strafe_vel;

    /* Reset command buffer, set initial values */
    draw_start(&context, pipeline);
    
    /* Bind Slow Data    
     * Textures, Vertices
     */
    geometry_buffer_bind(pipeline.command_buffer, geo_data);
    
    vkCmdBindDescriptorSets(pipeline.command_buffer,
			    VK_PIPELINE_BIND_POINT_GRAPHICS,
			    pipeline.layout, 0, 1,
			    &pipeline.slow_set, 0, NULL);

    /* Rapid / Per-frame data
     * Transformation matrices, texture index
     */
    draw_args_update(camera, entities,
		     indirect_args[context.frame_index] );
    vkCmdBindDescriptorSets(pipeline.command_buffer,
			    VK_PIPELINE_BIND_POINT_GRAPHICS,
			    pipeline.layout, 1, 1,
			    &pipeline.rapid_sets[context.frame_index],0, NULL);

    /* ultimate draw call */
    draw_indirect_update(indirect_draw, entities);
    vkCmdDrawIndexedIndirect(pipeline.command_buffer, indirect_draw.handle, 0,
			     entity_c, sizeof(VkDrawIndexedIndirectCommand));
    
    draw_end(context, pipeline);

    //    running = 0;
    }
  /* Main rendering loop end */
  
  /* Cleanup */
  VkFence fences[] = { pipeline.in_flight};
  vkWaitForFences(context.l_dev, 1, fences, VK_TRUE, UINT64_MAX);
  models_destroy(context, model_count, geo_data, textures);
  buffers_destroy(context, &indirect_draw, 1);
  buffers_destroy(context, indirect_args, context.frame_c);
  stage2_destroy(context, pipeline);
  stage1_destroy(context);
    
  return 0;
  }
