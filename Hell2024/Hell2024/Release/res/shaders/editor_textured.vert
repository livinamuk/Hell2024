#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;
out vec2 TexCoord;

out vec3 Color;

void main() {

    vec4 worldPos = vec4(vec4(aPos, 1.0));
	gl_Position = projection * view * model * worldPos;

	TexCoord = aTexCoord;
}