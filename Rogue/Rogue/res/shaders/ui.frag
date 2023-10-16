#version 420 core

out vec4 FragColor;
in vec2 TexCoords;

layout (binding = 0) uniform sampler2D tex;
layout (binding = 1) uniform sampler3D tex3D;

uniform bool use3DTexture;

#define MAP_DEPTH   36

void main() {

    vec4 textureColor = texture(tex, TexCoords); 

    float increment = 1.0 / MAP_DEPTH;

    if (use3DTexture) {
        textureColor = texture(tex3D, vec3(TexCoords.x, -TexCoords.y, increment * 18.0));   
    }

    FragColor = textureColor;
}