#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 normal;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;
uniform vec3 color;
uniform bool uniformColor;

out vec3 Color;

void main() {
	Color = normal;
	//Color.r += 0.5;
	vec3 pos = aPos.xyz;
	pos.x *= 1.15;
	pos.z = -2.2;
	pos.y += 0.05;
	gl_Position = projection * vec4(pos.xyz, 1.0);
}