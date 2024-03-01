#version 460 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec3 aDirectLightingColor;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;
out vec3 Color;

void main() {
	Color = aDirectLightingColor.xyz;
	//Color = aNormal.xyz;
	gl_Position = projection * view * vec4(aPos.xyz, 1.0);
}