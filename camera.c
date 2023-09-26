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
bird_mvp_calc(Camera cam, Entity entity, mat4 *dest)
{
  /* if you are doing m*vector then the order is (p*(v*(m*vector)))
   * so the pre multiplied matrix is p * (v*m)
   * btw `   glm_mat4_mul(vm, projection, push.mvp);`
   * vm * p is not the same as `p * vm`
   */

  mat4 model;
  glm_mat4_identity(model);
  glm_translate(model, entity.pos);
  
  //  float pitch_r = glm_rad(cam.pitch);
  float yaw_r = glm_rad(cam.yaw);

  vec3 ground;
  ground[0] = cam.pos[0];
  ground[1] = cam.pos[1] - 10;
  ground[2] = -1;

  vec3 eye;
  float translatedX = cam.pos[0] - ground[0];
  float translatedY = cam.pos[1] - ground[1]; 

  eye[0] = translatedX * cos(yaw_r) - translatedY * sin(yaw_r);
  eye[1] = translatedX * sin(yaw_r) + translatedY * cos(yaw_r);

  eye[0] += ground[0];
  eye[1] += ground[1];
  eye[2] = cam.pos[2];

  mat4 view;
  glm_lookat(eye, ground, (vec3){0.0f, 0.0f, -1.0f},
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
  
  glm_vec3_copy( (vec3){0.0f, 0.0f, -10.0f}, camera.pos );
	 
  camera.pitch = -90.0f;
  camera.yaw = 0.0f;

  camera.fov = glm_rad(90.0f);
  camera.aspect_ratio = context.extent.width / context.extent.height;
  
  camera.near = 0.1f;
  camera.far = 500;
  
  return camera;
}

