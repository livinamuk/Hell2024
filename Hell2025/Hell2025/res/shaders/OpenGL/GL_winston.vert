#version 330 core

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inNormal;

out vec3 normal;
out vec3 viewDir;

uniform mat4 projection;
uniform mat4 model;
uniform mat4 view;

void main() {
	mat3 normalMat = mat3(transpose(inverse(view * model)));
	normal = normalize(normalMat * inNormal);

	vec4 viewSpace = view * model * vec4(inPos, 1);
	viewDir = vec3(viewSpace);
	gl_Position = projection * viewSpace;
}