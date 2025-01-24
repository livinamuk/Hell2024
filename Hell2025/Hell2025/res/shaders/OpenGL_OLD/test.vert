#version 330 core

layout (location = 0) in vec4 aPos;
//layout (location = 1) in vec3 aColor;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;
uniform vec3 color;
uniform bool uniformColor;

out vec3 Color;

void main() {
	Color = (model * vec4(aPos.xyz, 1.0)).rgb;
	gl_Position = projection * view * model *vec4(aPos.xyz, 1.0);
}