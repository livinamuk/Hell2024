#version 460 core

layout (location = 0) in vec3 vPos;
layout (location = 1) in vec3 vNormal;
layout (location = 2) in vec2 vTexCoord;
layout (location = 3) in vec3 vTangent;

uniform mat4 projection;
uniform mat4 view;
uniform int playerIndex;
uniform int renderItemIndex;
uniform int goldBaseColorTextureIndex;
uniform int goldRMATextureIndex;

//out vec3 Normal;
out vec2 TexCoords;
out flat int PlayerIndex;
out vec3 Normal;
out vec3 Tangent;
out vec3 BiTangent;

out flat int BaseColorTextureIndex;
out flat int NormalTextureIndex;
out flat int RMATextureIndex;

struct SkinnedRenderItem {
    mat4 modelMatrix;
    mat4 inverseModelMatrix;
    int meshIndex;
    int materialIndex;
    int baseVertex;
	int isGold;
};

struct Material {
	int baseColorTextureIndex;
	int normalTextureIndex;
	int rmaTextureIndex;
	int emissiveTextureIndex;
};


layout(std430, binding = 17) readonly buffer renderItems {
    SkinnedRenderItem RenderItems[];
};

layout(std430, binding = 3) readonly buffer materials {
    Material Materials[];
};

void main() {

	TexCoords = vTexCoord;
	PlayerIndex = playerIndex;

	mat4 model = RenderItems[renderItemIndex].modelMatrix;
	mat4 invereseModel = RenderItems[renderItemIndex].inverseModelMatrix;

	Material material = Materials[RenderItems[renderItemIndex].materialIndex];
	BaseColorTextureIndex =  material.baseColorTextureIndex;
	NormalTextureIndex =  material.normalTextureIndex;
	RMATextureIndex =  material.rmaTextureIndex;

	//mat4 normalMatrix = transpose(invereseModel);
	//attrNormal = normalize((normalMatrix * vec4(aNormal, 0)).xyz);
	//attrTangent = normalize((normalMatrix * vec4(aTangent, 0)).xyz);;
	////attrTangent = normalize(attrTangent - dot(attrTangent, attrNormal) * attrNormal);
	//attrBiTangent = normalize(cross(attrNormal,attrTangent));

    mat4 normalMatrix = transpose(invereseModel);
	Normal = normalize(normalMatrix * vec4(vNormal, 0)).xyz;
	Tangent = normalize(normalMatrix * vec4(vTangent, 0)).xyz;
	BiTangent = normalize(cross(Normal, Tangent));

	gl_Position = projection * view * model * vec4(vPos, 1.0);

	// Gold?
	if (RenderItems[renderItemIndex].isGold == 1) {
		BaseColorTextureIndex = goldBaseColorTextureIndex;
		RMATextureIndex = goldRMATextureIndex;
	}
}