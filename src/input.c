#include "main.h"

int main_input(GfxContext context, Camera* camera) {
  static double front_vel, strafe_vel;
  float pitch_vel = 0, yaw_vel = 0;
  static VkBool32 CAMERA_ON = 1;
  double speed = 0.5;
  float sensitivity = 0.005;

  SDL_Event event;
  SDL_PollEvent( &event );
  switch( event.type ) {
    
  case SDL_KEYDOWN:
    switch( event.key.keysym.scancode) {
    case SDL_SCANCODE_D:
      strafe_vel = speed;
      break;
    case SDL_SCANCODE_A:
      strafe_vel = -speed;
      break;
    case SDL_SCANCODE_W:
      front_vel = speed;
      break;
    case SDL_SCANCODE_S:
      front_vel = -speed;
      break;
    default:
      break;
    } break;
    
  case SDL_KEYUP:      
    switch( event.key.keysym.scancode){
    case SDL_SCANCODE_D:
      if( strafe_vel > 0 ) strafe_vel = 0;
      break;
    case SDL_SCANCODE_A:
      if( strafe_vel < 0 ) strafe_vel = 0;
      break;
    case SDL_SCANCODE_W:
      if( front_vel > 0 ) front_vel = 0;
      break;
    case SDL_SCANCODE_S:
      if( front_vel < 0 ) front_vel = 0;
      break;
    default:
      break;
    } break;
    
  case SDL_MOUSEMOTION:
    pitch_vel = event.motion.yrel * sensitivity;
    yaw_vel = event.motion.xrel * sensitivity;
    break;
    
  case SDL_QUIT:
    return 0;
    break;
    
  default:
    break;
    
  }
  
  // Process Movement System
  camera_rotate(camera, yaw_vel, pitch_vel);
  camera->pos[0] += camera->front[0] * front_vel;
  camera->pos[1] += camera->front[1] * front_vel;
  camera->pos[0] += camera->right[0] * strafe_vel;
  camera->pos[1] += camera->right[1] * strafe_vel;
  
  return 1;
}


int camera_rotate(Camera *cam, float x_vel, float y_vel){
  /*
   * rotates camera front, up and right from mouse input
   * cam [OUT]
   * x_vel, yvel [IN]
   */
  
  versor q, qPitch;
  float yaw = x_vel * -1;
  float pitch = y_vel * -1;

  /* up/down clamp */
  if (pitch > GLM_PI_2) pitch = GLM_PI_2;
  if (pitch < -GLM_PI_2) pitch = -GLM_PI_2;

  /* rotate camera by quaternion */
  glm_quatv(q, yaw, cam->up);
  glm_quatv(qPitch, pitch, cam->right);
  glm_quat_mul(q, qPitch, q);
  glm_quat_rotatev(q, cam->front, cam->front);

  /* maintain axis-angle alignment */
  glm_vec3_cross(cam->front, (vec3){0.0f, 0.0f, -1.0f}, cam->right);
  glm_vec3_cross(cam->right, cam->front, cam->up);
  
  return 1;
}

Camera
camera_create(int width, int height)
{
  Camera cam;

  glm_vec3_copy( (vec3){0.0f, 0.0f, -2.0f}, cam.pos );
  glm_vec3_copy( (vec3){1.0f, 0.0f, 0.0f}, cam.front );
  glm_vec3_copy( (vec3){0.0f, 0.0f, -1.0f}, cam.up );
  glm_vec3_cross(cam.front, cam.up, cam.right);
  
  float fov = glm_rad(90.0f);
  float aspectRatio = width / height;
  
  float near = 0.5f;
  float far= 500;

  glm_perspective(fov, aspectRatio, near, far, cam.projection);
  cam.projection[1][1] *= -1;
  
  return cam;
}
