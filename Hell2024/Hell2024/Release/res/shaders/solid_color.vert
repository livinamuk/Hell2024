#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;
uniform vec3 color;
uniform bool uniformColor;

out vec3 Color;

void main() {

	if (uniformColor)
		Color = color;
	else
		Color = aColor;

	gl_Position = projection * view * model *vec4(aPos, 1.0);
}