#version 420 core

out vec4 FragColor;
layout (binding = 0) uniform sampler2D Texture;
in vec2 TexCoord;

void main() {
    vec4 baseColor = texture(Texture, TexCoord);

    if (baseColor.a < 0.9) {
        discard;
    }

    FragColor = vec4(baseColor.rgb, 1.0f);
}