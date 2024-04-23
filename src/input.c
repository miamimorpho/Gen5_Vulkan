#include "main.h"

int main_input(GfxContext context, Camera* camera) {
  double front_vel = 0, strafe_vel = 0;
  double x_raw = 0, y_raw = 0;
  
  static VkBool32 CAMERA_ON = 1;
  double speed = 0.5;
  float sensitivity = 0.005;
  
  glfwSetCursorPos(context.window, 0,0);
  glfwPollEvents();
  
  if (glfwGetKey(context.window, GLFW_KEY_A) == GLFW_PRESS) {
    strafe_vel = -speed;
  } else if (glfwGetKey(context.window, GLFW_KEY_D) == GLFW_PRESS) {
    strafe_vel = speed;
  } else {
    strafe_vel = 0.0;
  }
  
  if (glfwGetKey(context.window, GLFW_KEY_W) == GLFW_PRESS) {
    front_vel = speed;
  } else if (glfwGetKey(context.window, GLFW_KEY_S) == GLFW_PRESS) {
    front_vel = -speed;
  } else {
    front_vel = 0.0;
  }
  
  if(glfwGetKey(context.window, GLFW_KEY_F1) == GLFW_PRESS) {
    if(CAMERA_ON){
      glfwSetInputMode(context.window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }else{
      glfwSetInputMode(context.window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    }
    
    CAMERA_ON = !CAMERA_ON;
  } 
    
  // Mouse motion handling
  if(CAMERA_ON && glfwGetWindowAttrib(context.window, GLFW_FOCUSED)){

    glfwGetCursorPos(context.window, &x_raw, &y_raw);

    // Process Movement System
    camera_rotate(camera, x_raw * sensitivity, y_raw * sensitivity);
    camera->pos[0] += camera->front[0] * front_vel;
    camera->pos[1] += camera->front[1] * front_vel;
    camera->pos[0] += camera->right[0] * strafe_vel;
    camera->pos[1] += camera->right[1] * strafe_vel;
  }
  
  // Check if window should close
  if (glfwWindowShouldClose(context.window)) {
    return 0;
  }

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
  if (cam->front[2] > 1) cam->front[2] = 0.9f;
  if (cam->front[2] < -1) cam->front[2] = -0.9f;

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

  //clear
  for(int i = 0; i < 100; i++){
    glfwPollEvents();
  }
  
  return cam;
}
