#version 450
#extension GL_EXT_nonuniform_qualifier : enable

layout(set = 0, binding = 0 ) uniform sampler2D texSampler[];

layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) flat in uint texture_index;

layout(location = 0) out vec4 outColor;

void main() {
    // Sample texture color
    vec4 textureColor = texture(texSampler[texture_index], fragTexCoord);

    // Basic diffuse lighting (Lambertian reflection model)
    vec3 lightDirection = normalize(vec3(1.0, 0.0, -1.0)); // Example: Directional light direction
    float diffuseFactor = max(dot(fragNormal, lightDirection), 0.0);
    diffuseFactor = 0.2 + 0.5 * diffuseFactor;
    vec3 diffuseColor = vec3(1.0, 1.0, 1.0); // Example: Diffuse light color

    vec3 finalColor = diffuseColor * diffuseFactor * textureColor.rgb;
    // Output final fragment color
    outColor = vec4(finalColor, textureColor.a);
}
