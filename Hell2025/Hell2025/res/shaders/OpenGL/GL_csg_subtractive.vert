#version 460 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;

uniform mat4 projection;
uniform mat4 model;
uniform mat4 view;
out vec3 Normal;

void main() {

	mat4 normalMatrix = transpose(inverse(model));
	Normal = normalize((normalMatrix * vec4(aNormal, 0)).xyz);
	gl_Position = projection * view * model * vec4(aPos, 1.0);
}