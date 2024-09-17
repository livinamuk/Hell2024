#version 460 core
out vec4 FragColor;

in vec3 WorldPos;

void main() {
	FragColor = vec4(WorldPos, 1.0);
}