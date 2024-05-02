#version 450
#extension GL_EXT_nonuniform_qualifier : enable

layout(set = 0, binding = 0 ) uniform sampler2D texSampler[];

layout(location = 0) in vec2 inUV;
layout(location = 1) flat in uint in_texture_i;

layout(location = 0) out vec4 outColor;

void main() {

    vec4 color = texture(texSampler[in_texture_i], inUV);
    outColor = vec4(color.r, color.r, color.r, color.r);
}
