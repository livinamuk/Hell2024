#version 420 core

out vec4 FragColor;
in vec2 TexCoords;

layout (binding = 0) uniform sampler2D tex;

uniform vec3 color;
uniform vec3 overrideColor;

void main() {
    vec4 textureColor = texture(tex, TexCoords); 
    FragColor = vec4(textureColor);
    FragColor.rgb *= color;
    FragColor.rgb *= overrideColor;
}