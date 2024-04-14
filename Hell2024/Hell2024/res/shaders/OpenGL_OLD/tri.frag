#version 420 core

out vec4 FragColor;
in vec3 Color;

void main() {
vec3 final = Color * 3.0;
    FragColor = vec4(final, 1.0f);
	FragColor.a = 0.45;
}