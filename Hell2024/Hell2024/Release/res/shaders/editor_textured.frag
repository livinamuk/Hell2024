#version 420 core

out vec4 FragColor;
layout (binding = 0) uniform sampler2D basecolorTexture;
layout (binding = 1) uniform sampler2D normalTexture;
layout (binding = 2) uniform sampler2D rmaTexture;
in vec2 TexCoord;

void main() {
    vec4 baseColor = texture(basecolorTexture, TexCoord);
    FragColor = vec4(baseColor.rgb, 1.0f);
}