#version 450

struct argument_t {
  mat4 rotate_m;
  mat4 mvp;
  uint texIndex;
};

layout ( set = 1, binding = 0, std430) buffer SSBO {
    argument_t args[];
}ssboData;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec4 fragNormal;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) flat out uint texture_index;

void main() {

  // get texture index to pass to fragment shader
  argument_t currentArg = ssboData.args[gl_InstanceIndex];
  texture_index = currentArg.texIndex;

  // rotate position vectors to camera space
  gl_Position = currentArg.mvp * vec4(inPosition, 1.0); // MVP matrix
  
  // rotate normals to correct lighting
  fragNormal = currentArg.rotate_m * vec4(inNormal, 0 );
 
fragTexCoord = inTexCoord;
}
