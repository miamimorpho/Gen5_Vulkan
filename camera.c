#include "solid.h"

const vec3 CAMERA_UP = {0.0f, 0.0f, -1.0f};

int
camera_rotate(Camera *cam, float x_vel, float y_vel){
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

int
fps_mvp_calc(Camera cam, Entity entity, mat4 *dest)
{
  /* if you are doing m*vector then the order is (p*(v*(m*vector)))
   * so the pre multiplied matrix is p * (v*m)
   * btw `   glm_mat4_mul(vm, projection, push.mvp);`
   * vm * p is not the same as `p * vm`
   */

  mat4 model;
  glm_mat4_identity(model);
  glm_translate(model, entity.pos);

  mat4 view;
  glm_look(cam.pos, cam.front, cam.up,
	     view);

  mat4 vm; // view*model
  glm_mat4_mul(view, model, vm);
  
  mat4 projection;
  glm_perspective(cam.fov, cam.aspect_ratio,
		  cam.near, cam.far, projection);
  projection[1][1] *= -1;

  glm_mat4_mul(projection, vm, *dest);
  return 0;
}

Camera
camera_create(GfxContext context)
{
  Camera camera;

  glm_vec3_copy( (vec3){0.0f, 0.0f, -2.0f}, camera.pos );
  glm_vec3_copy( (vec3){1.0f, 0.0f, 0.0f}, camera.front );
  glm_vec3_copy( (vec3){0.0f, 0.0f, -1.0f}, camera.up );
  glm_vec3_cross(camera.front, camera.up, camera.right);
  
  
  camera.fov = glm_rad(90.0f);
  camera.aspect_ratio = context.extent.width / context.extent.height;
  
  camera.near = 0.1f;
  camera.far = 500;
  
  return camera;
}

