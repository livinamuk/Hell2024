#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in vec3 aTangent;
layout (location = 4) in vec3 aBitangent;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

out vec2 TexCoord;
out vec3 WorldPos;

out vec3 attrNormal;
out vec3 attrTangent;
out vec3 attrBiTangent;

uniform int tex_flag;

void main() {

	TexCoord = aTexCoord;
	WorldPos = (model * vec4(aPos.x, aPos.y, aPos.z, 1.0)).xyz;

	gl_Position = projection * view * vec4(WorldPos, 1.0);
	
	attrNormal = (model * vec4(aNormal, 0.0)).xyz;
	attrTangent = (model * vec4(aTangent, 0.0)).xyz;
	attrBiTangent = (model * vec4(aBitangent, 0.0)).xyz;
}