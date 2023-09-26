#version 450

struct argument_t {
    uint texture_index;
    mat4 transform;
};

layout ( set = 1, binding = 0) buffer SSBO {
    argument_t args[];
}ssboData;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;

layout(location = 0) out vec2 fragTexCoord;
layout(location = 1) flat out uint texture_index;

void main() {
    argument_t currentArg = ssboData.args[gl_InstanceIndex];
    texture_index = currentArg.texture_index;
    gl_Position = currentArg.transform * vec4(inPosition, 1.0);
 
    fragTexCoord = inTexCoord;
}
