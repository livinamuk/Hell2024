#version 420

layout (location = 0) out vec4 FragColor;
layout (binding = 0) uniform sampler2D tex;
in vec2 TexCoords;

void main() {
    FragColor.rgb = texture(tex, TexCoords).rgb;
    FragColor.a = 1;   
}