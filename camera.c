#include "solid.h"

const vec3 CAMERA_UP = {0.0f, 0.0f, -1.0f};

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
  
  camera.fov = glm_rad(90.0f);
  camera.aspect_ratio = context.extent.width / context.extent.height;
  
  camera.near = 0.1f;
  camera.far = 500;
  
  return camera;
}

