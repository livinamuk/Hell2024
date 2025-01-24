#version 460 core

layout (location = 0) in vec3 vPos;
layout (location = 1) in vec3 vNormal;
layout (location = 2) in vec2 vTexCoords;
layout (location = 3) in vec3 vTangent;

uniform mat4 normalMatrix;
uniform mat4 mvp;

uniform float heightMapWidth;
uniform float heightMapDepth;
out vec2 MegaTextureCoords;

out vec2 TexCoords;
out vec3 Normal;
out vec3 BiTangent;
out vec3 Tangent;

void main() {

	MegaTextureCoords = vec2(vPos.x / heightMapWidth, vPos.z / heightMapDepth);
	
	TexCoords = vTexCoords * vec2(3);
	Normal = normalize((normalMatrix * vec4(vNormal, 0)).xyz);
	Tangent = normalize((normalMatrix * vec4(vTangent, 0)).xyz);
	BiTangent = normalize(cross(Normal, Tangent));

	vec3 position = vPos;
	//position.y += 10;

	gl_Position = mvp * vec4(position, 1.0);
}