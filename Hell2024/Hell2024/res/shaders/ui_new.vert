#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;

uniform mat4 model;
out vec2 TexCoords;

void main() {
	TexCoords = aTexCoord;
	gl_Position = model * vec4(aPos.xy, 0, 1.0);
}