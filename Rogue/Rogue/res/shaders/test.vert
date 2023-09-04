#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

out vec3 Normal;
out vec2 TexCoord;
out vec3 WorldPos;

uniform int tex_flag;

void main() {

	Normal = aNormal;
	TexCoord = aTexCoord;
	WorldPos = (model * vec4(aPos.x, aPos.y, aPos.z, 1.0)).xyz;

	gl_Position = projection * view * vec4(WorldPos, 1.0);
	
	if (tex_flag == 1)
		TexCoord = vec2(WorldPos.z, WorldPos.x)  * 0.5;
		
	if (tex_flag == 2)
		TexCoord = vec2(WorldPos.z / 2.4, WorldPos.y / 2.4) ;
	if (tex_flag == 3)
		TexCoord = vec2(WorldPos.x / 2.4, WorldPos.y / 2.4) ;
}