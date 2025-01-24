#version 460 core

layout (location = 0) in vec3 vPos;
layout (location = 1) in vec3 vNormal;
layout (location = 2) in vec2 vTexCoord;
layout (location = 3) in vec3 vTangent;
layout (location = 4) in int vMaterialIndex;

uniform mat4 projection;
uniform mat4 view;
uniform int playerIndex;
uniform mat4 model;
uniform mat4 inverseModel;
uniform int materialIndex;

out vec2 TexCoord;
out flat int PlayerIndex;
out vec3 attrNormal;
out vec3 attrTangent;
out vec3 attrBiTangent;

out flat int BaseColorTextureIndex;
out flat int NormalTextureIndex;
out flat int RMATextureIndex;

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

	TexCoord = vTexCoord;

	Material material = Materials[materialIndex];
	BaseColorTextureIndex =  material.baseColorTextureIndex;
	NormalTextureIndex =  material.normalTextureIndex;
	RMATextureIndex =  material.rmaTextureIndex;

	//mat4 normalMatrix = transpose(inverseModel);
	//attrNormal = normalize((normalMatrix * vec4(vNormal, 0)).xyz);
	//attrTangent = (model * vec4(vTangent, 0.0)).xyz;

	attrNormal = vNormal;
	attrTangent = vTangent;
	attrBiTangent = normalize(cross(attrNormal,attrTangent));

	/*
	// you can probably remove all that matrix shit above. we're all identity here baby.
	if (vMaterialIndex > 0 && vPos.z < -2.4) {
		material = Materials[vMaterialIndex];
		BaseColorTextureIndex =  material.baseColorTextureIndex;
		NormalTextureIndex =  material.normalTextureIndex;
		RMATextureIndex =  material.rmaTextureIndex;
		//BaseColorTextureIndex = /100;
	}*/

    vec4 worldPosition = model * vec4(vPos, 1.0);

	gl_Position = projection * view * worldPosition;
	PlayerIndex = playerIndex;

}