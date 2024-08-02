#version 460 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in vec3 aTangent;
layout (location = 4) in ivec4 aBoneID;
layout (location = 5) in vec4 aBoneWeight;

uniform mat4 projection;
uniform mat4 view;
uniform int playerIndex;
uniform int goldBaseColorTextureIndex;
uniform int goldRMATextureIndex;

out vec2 TexCoord;
out flat int BaseColorTextureIndex;
out flat int NormalTextureIndex;
out flat int RMATextureIndex;
out flat int useEmissiveMask;
out flat int PlayerIndex;
out vec3 attrNormal;
out vec3 attrTangent;
out vec3 attrBiTangent;
out vec3 emissiveColor;

struct RenderItem3D {
    mat4 modelMatrix;
    mat4 inverseModelMatrix;
    int meshIndex;
    int materialIndex;
    int vertexOffset;
    int indexOffset;
    int castShadow;
    int useEmissiveMask;
    int isGold;
    float emissiveColorR;
    float emissiveColorG;
    float emissiveColorB;
    float aabbMinX;
    float aabbMinY;
    float aabbMinZ;
    float aabbMaxX;
    float aabbMaxY;
    float aabbMaxZ;
};

struct Material {
	int baseColorTextureIndex;
	int normalTextureIndex;
	int rmaTextureIndex;
	int emissiveTextureIndex;
};

layout(std430, binding = 1) readonly buffer renderItems {
    RenderItem3D RenderItems[];
};

layout(std430, binding = 3) readonly buffer materials {
    Material Materials[];
};


void main() {

	TexCoord = aTexCoord;
	mat4 model = RenderItems[gl_InstanceID + gl_BaseInstance].modelMatrix;
	mat4 invereseModel = RenderItems[gl_InstanceID + gl_BaseInstance].inverseModelMatrix;

	Material material = Materials[RenderItems[gl_InstanceID + gl_BaseInstance].materialIndex];
	//Material material = Materials[5];
	BaseColorTextureIndex =  material.baseColorTextureIndex;
	NormalTextureIndex =  material.normalTextureIndex;
	RMATextureIndex =  material.rmaTextureIndex;

	useEmissiveMask = RenderItems[gl_InstanceID+ gl_BaseInstance].useEmissiveMask;
	emissiveColor.r = RenderItems[gl_InstanceID+ gl_BaseInstance].emissiveColorR;
	emissiveColor.g = RenderItems[gl_InstanceID+ gl_BaseInstance].emissiveColorG;
	emissiveColor.b = RenderItems[gl_InstanceID+ gl_BaseInstance].emissiveColorB;

	mat4 normalMatrix = transpose(invereseModel);
	attrNormal = normalize((normalMatrix * vec4(aNormal, 0)).xyz);
	attrTangent = (model * vec4(aTangent, 0.0)).xyz;
	attrBiTangent = normalize(cross(attrNormal,attrTangent));

	gl_Position = projection * view * model * vec4(aPos, 1.0);
	PlayerIndex = playerIndex;

	// Gold?
	if (RenderItems[gl_InstanceID + gl_BaseInstance].isGold == 1) {
		BaseColorTextureIndex = goldBaseColorTextureIndex;
		RMATextureIndex = goldRMATextureIndex;
	}

}