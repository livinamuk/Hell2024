#version 460 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec3 aDirectLighting;

uniform mat4 projection;
uniform mat4 view;

out vec3 Color;

void main() {

	Color = aDirectLighting;
	gl_Position = projection * view * vec4(aPos, 1.0);

}