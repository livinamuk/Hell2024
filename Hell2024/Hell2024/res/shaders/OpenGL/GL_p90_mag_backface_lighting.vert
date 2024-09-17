#version 460 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in vec3 aTangent;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

out vec3 Normal;
out vec2 TexCoords;
out vec3 WorldPos;

out vec3 attrNormal;
out vec3 attrTangent;
out vec3 attrBiTangent;

out flat int BaseColorTextureIndex;
out flat int NormalTextureIndex;
out flat int RMATextureIndex;

uniform int materialIndex;

struct Material {
	int baseColorTextureIndex;
	int normalTextureIndex;
	int rmaTextureIndex;
	int emissiveTextureIndex;
};


layout(std430, binding = 3) readonly buffer materials {
    Material Materials[];
};


void main() {
	TexCoords = aTexCoord;
	Normal = aNormal;

	gl_Position = projection * view * model * vec4(aPos, 1.0);

	mat4 inverseModel = inverse(model);
	mat4 normalMatrix = transpose(inverseModel);

	attrNormal = normalize((normalMatrix * vec4(aNormal, 0)).xyz);
	attrTangent = (model * vec4(aTangent, 0.0)).xyz;
	attrBiTangent = normalize(cross(attrNormal,attrTangent));

	WorldPos = vec3(model * vec4(aPos, 1.0)).xyz;


	Material material = Materials[materialIndex];
	BaseColorTextureIndex =  material.baseColorTextureIndex;
	NormalTextureIndex =  material.normalTextureIndex;
	RMATextureIndex =  material.rmaTextureIndex;

}