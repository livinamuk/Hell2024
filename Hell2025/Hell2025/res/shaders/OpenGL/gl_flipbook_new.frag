#version 460 core
layout (location = 0) out vec4 FragOut;
layout (binding = 0) uniform sampler2DArray TextureArray;

in vec2 TexCoord;
uniform float mixFactor;
uniform int index;
uniform int indexNext;

void main() {        
    vec4 colorA = texture(TextureArray, vec3(TexCoord, index));
    vec4 colorB = texture(TextureArray, vec3(TexCoord, indexNext));    
    FragOut = mix(colorA, colorB, mixFactor);
}
