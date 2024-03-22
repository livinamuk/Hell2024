#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;

uniform mat4 projection;
uniform mat4 model;
uniform mat4 view;

out vec4 WorldPos;
out vec3 Normal;
out vec2 TexCoords;

void main()
{
	WorldPos = model * vec4(aPos, 1.0);
	WorldPos.y += 0.025;
	Normal = normalize((model * vec4(aNormal, 0.0)).xyz);
	TexCoords = aTexCoord;	
	gl_Position = projection * view * WorldPos;
}