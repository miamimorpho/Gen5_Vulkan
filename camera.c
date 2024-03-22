#include "solid.h"

int
static_mvp_calc(Camera cam, Entity entity, mat4 *dest){

  mat4 model;
  glm_mat4_identity(model);
  glm_translate(model, entity.pos);
  
  /* if you are doing m*vector then the order is (p*(v*(m*vector)))
   * so the pre multiplied matrix is p * (v*m)
   * btw `   glm_mat4_mul(vm, projection, push.mvp);`
   * vm * p is not the same as `p * vm`
   */
  
  mat4 view;
  glm_lookat((vec3){-5.0f, -10.0f, -5.0f},
	       (vec3){0.0f, 0.0f, 0.0f},
	       (vec3){0.0f, 0.0f, -1.0f}, view);

  mat4 vm; // view*model
  glm_mat4_mul(view, model, vm);
  
  mat4 projection;
  glm_perspective(cam.fov, cam.aspect_ratio,
		  cam.near, cam.far, projection);
  projection[1][1] *= -1;

  glm_mat4_mul(projection, vm, *dest);
  return 0;
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

  /*
  float pitch_r = glm_rad(cam.pitch);
  float yaw_r = glm_rad(cam.yaw);
  */
  
  vec3 front;
  front[0] = cos(cam.yaw) * cos(cam.pitch);
  front[1] = sin(cam.pitch);
  front[2] = sin(cam.yaw) * cos(cam.pitch);
  glm_vec3_normalize(front);
  
  vec3 eye;
  eye[0] = cam.pos[0];// front[0];
    eye[1] = cam.pos[1];// front[1];
    eye[2] = cam.pos[2];// front[2];
  
  mat4 view;
  glm_look(eye, front, (vec3){0.0f, 1.0f, 0},
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
  
  glm_vec3_copy( (vec3){0.0f, 60.0f, 0.0f}, camera.pos );
	 
  camera.pitch = 0.0f;
  camera.yaw = 0.0f;

  camera.fov = glm_rad(90.0f);
  camera.aspect_ratio = context.extent.width / context.extent.height;
  
  camera.near = 0.1f;
  camera.far = 500;
  
  return camera;
}

