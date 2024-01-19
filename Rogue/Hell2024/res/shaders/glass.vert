#version 450 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

out vec3 WorldPos;
out vec2 TexCoords;
out vec3 Normal;

void main() {

	mat4 normalMatrix = transpose(inverse(model));
	Normal = normalize((normalMatrix * vec4(aNormal, 0)).xyz);
	TexCoords = aTexCoord;
	WorldPos = vec3(model * vec4(aPos, 1.0));
	gl_Position = projection * view * vec4(WorldPos, 1.0);
}