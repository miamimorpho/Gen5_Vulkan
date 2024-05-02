#version 450

struct argument_t {
  mat4 translate_m;
  uint texture_i;
};

layout ( set = 1, binding = 0, std430) buffer SSBO {
    argument_t args[];
}ssboData;

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec2 inUV;

layout(location = 0) out vec2 outUV;
layout(location = 1) flat out uint out_texture_i;

void main() {

  // get texture index to pass to fragment shader
  argument_t currentArg = ssboData.args[gl_InstanceIndex];
  
  out_texture_i = currentArg.texture_i;
  outUV = inUV;

  // rotate position vectors to camera space
  gl_Position = vec4(inPosition, 0.0, 1.0);
  
}
