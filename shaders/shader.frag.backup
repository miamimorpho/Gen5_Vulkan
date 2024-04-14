#version 450
#extension GL_EXT_nonuniform_qualifier : enable

layout(set = 0, binding = 0 ) uniform sampler2D texSampler[];

layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) flat in vec3 lightDirection; 
layout(location = 3) flat in uint texture_index;


layout(location = 0) out vec4 outColor;

void main() {

    // base texture colour
    vec4 textureColor = texture(texSampler[texture_index], fragTexCoord);

    // Basic diffuse lighting (Lambertian reflection model)
    float diffuseFactor =  max( dot( normalize(fragNormal), normalize(lightDirection) ), 0.0);
    diffuseFactor = (diffuseFactor * 0.9) + 0.1;
    vec3 diffuseColor = vec3(1.0, 1.0, 1.0); // Example: Diffuse light color
    vec3 finalColor = diffuseColor * diffuseFactor * textureColor.rgb;
    
    outColor = vec4(finalColor, 1.0);
    //vec4(1.0,1.0,1.0,1.0);
}
