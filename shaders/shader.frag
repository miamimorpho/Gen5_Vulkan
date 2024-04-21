#version 450
#extension GL_EXT_nonuniform_qualifier : enable

layout(set = 0, binding = 0 ) uniform sampler2D texSampler[];

layout(location = 0) in vec4 fragNormal;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) flat in uint texture_index;

layout(location = 0) out vec4 outColor;

void main() {

    // base texture colour
    vec4 textureColor = texture(texSampler[texture_index], fragTexCoord);

    // Basic diffuse lighting (Lambertian reflection model)
    vec4 lightDirection = normalize(vec4(1, 0, -1, 0));
    
    // Primary Light
    float diffuseFactor =  max( dot(lightDirection, fragNormal ), 0.0);
    diffuseFactor = (diffuseFactor * 0.9) + 0.1;
    vec3 diffuseColor = diffuseFactor * vec3(1.0, 1.0, 1.0);

    vec3 finalColor = diffuseColor * textureColor.rgb;
    
    outColor = vec4(finalColor, 1.0);
}
