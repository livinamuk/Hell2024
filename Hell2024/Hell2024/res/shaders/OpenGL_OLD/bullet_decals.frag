#version 420 core

layout (location = 0) out vec4 BaseColorOut;
layout (location = 1) out vec4 NormalsOut;
layout (location = 2) out vec4 RMAOut;

layout (binding = 0) uniform sampler2D Texture;

in vec3 Normal;
in vec2 TexCoord;

void main() {
    vec4 baseColor = texture(Texture, TexCoord);

    if (baseColor.a < 0.9) {
        discard;
    }
    
    float roughness = 0.95;
    float metallic = 0.1;

    BaseColorOut = vec4(baseColor.rgb, 1.0f);
    NormalsOut = vec4(Normal, 1.0);
    RMAOut =  vec4(roughness, metallic, 1, 1.0);

}