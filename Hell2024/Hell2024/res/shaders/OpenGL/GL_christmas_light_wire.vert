#version 460 core

layout (location = 0) in vec3 vPos;
layout (location = 1) in vec3 vNormal;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

out vec3 Normal;

void main() {

	Normal = vNormal;
	gl_Position = projection * view * vec4(vPos, 1.0);

}