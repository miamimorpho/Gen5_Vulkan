#include "solid.h"
#include <SDL2/SDL.h>

Entity
entity_duplicate(Entity mother, float x, float y, float z){

  Entity child;
  glm_vec3_copy( (vec3){x, y, z}, child.pos);
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

  /* Load gltf data to renderer */
  int model_count = 1;
  GeometryData geo = geometry_buffer_create(context);
  ImageData textures[model_count];
  Entity car;
  gltf_load(context, "models/Car2.glb",
	    &geo, textures, &car);

  
  int entity_c = 100;
  Entity entities[entity_c];
  for(int x = 0; x < 10; x++){
    for(int y = 0; y < 10; y++){
      entities[(x * 10)+y] = entity_duplicate(car, x * 10, y * 10, 0);
    }
  }
  slow_descriptors_update(context, model_count, textures, &pipeline);
  
  DeviceBuffer draw_args[context.frame_c];
  draw_args_create(context, pipeline, entity_c, draw_args);

  DeviceBuffer indirect_b;
  indirect_buffer_create(context, entity_c, &indirect_b);

  /* Mouse Data */
  double front_vel = 0, strafe_vel = 0, speed = 0.2;
  vec3 strafe;
  float pitch = 0, yaw = 0, sensitivity = 0.001f;
  float pitch_vel = 0, yaw_vel = 0;
  SDL_SetRelativeMouseMode(SDL_TRUE);

  vec3 camera_front_delta;
  
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

    /* Mouse/Camera Settings */

    pitch += pitch_vel;
    yaw += yaw_vel;
    camera.front[0] = cos(pitch) * cos(yaw);
    camera.front[1] = sin(pitch);
    camera.front[2] = sin(yaw) * cos(pitch);
    glm_vec3_normalize(camera.front);   
    pitch_vel = 0;
    yaw_vel = 0;

    while(pitch >= GLM_PI / 2 )
      pitch = ((GLM_PI / 2) - 0.1);
    while(pitch <= -(GLM_PI / 2))
      pitch = -((GLM_PI / 2)  - 0.1);
    while(yaw >= GLM_PI * 2)
      yaw = 0;
    while(yaw <= -GLM_PI * 2)
      yaw = 0;
      
    /* Clamping of pitch and yaw */
    //printf("pitch, yaw :%f %f\n", pitch, yaw);
    printf("front :%f %f %f\n", camera.front[0], camera.front[1], camera.front[2]);
    glm_vec3_cross(camera.front, (vec3){0.0f, 0.0f, -1.0f}, strafe);
    glm_vec3_cross(strafe, camera.front, camera.up);
    camera.pos[0] += camera.front[0] * front_vel;
    camera.pos[1] += camera.front[1] * front_vel;
    camera.pos[0] += strafe[0] * strafe_vel;
    camera.pos[1] += strafe[1] * strafe_vel;
   
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
