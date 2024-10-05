#version 460 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in vec3 aTangent;

uniform mat4 projection;
uniform mat4 view;
uniform int playerIndex;
uniform int renderItemIndex;
uniform int goldBaseColorTextureIndex;
uniform int goldRMATextureIndex;

//out vec3 Normal;
out vec2 TexCoords;
out flat int PlayerIndex;
out vec3 attrNormal;
out vec3 attrTangent;
out vec3 attrBiTangent;

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

	TexCoords = aTexCoord;
	PlayerIndex = playerIndex;

	mat4 model = RenderItems[renderItemIndex].modelMatrix;
	mat4 invereseModel = RenderItems[renderItemIndex].inverseModelMatrix;

	Material material = Materials[RenderItems[renderItemIndex].materialIndex];
	BaseColorTextureIndex =  material.baseColorTextureIndex;
	NormalTextureIndex =  material.normalTextureIndex;
	RMATextureIndex =  material.rmaTextureIndex;

	mat4 normalMatrix = transpose(invereseModel);
	attrNormal = normalize((normalMatrix * vec4(aNormal, 0)).xyz);
	attrTangent = normalize((normalMatrix * vec4(aTangent, 0)).xyz);;
	//attrTangent = normalize(attrTangent - dot(attrTangent, attrNormal) * attrNormal);
	attrBiTangent = normalize(cross(attrNormal,attrTangent));

	gl_Position = projection * view * model * vec4(aPos, 1.0);

	// Gold?
	if (RenderItems[renderItemIndex].isGold == 1) {
		BaseColorTextureIndex = goldBaseColorTextureIndex;
		RMATextureIndex = goldRMATextureIndex;
	}
}