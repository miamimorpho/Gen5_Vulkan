#version 450
#extension GL_EXT_nonuniform_qualifier : enable

layout(set = 0, binding = 0 ) uniform sampler2D texSampler[];

layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) flat in uint texture_index;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = texture(texSampler[texture_index], fragTexCoord);
}
