#version 460 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;

out vec2 TexCoord;
out vec3 Normal;

out flat int BaseColorTextureIndex;
out flat int NormalTextureIndex;
out flat int RMATextureIndex;

uniform int instanceDataOffset;
uniform int playerIndex;

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

layout(std430, binding = 13) readonly buffer renderItems {
    RenderItem3D RenderItems[];
};



struct CameraData {
    mat4 projection;
    mat4 projectionInverse;
    mat4 view;
    mat4 viewInverse;
	float viewportWidth;
	float viewportHeight;
    float viewportOffsetX;
    float viewportOffsetY;
	float clipSpaceXMin;
    float clipSpaceXMax;
    float clipSpaceYMin;
    float clipSpaceYMax;
	float finalImageColorContrast;
    float finalImageColorR;
    float finalImageColorG;
    float finalImageColorB;
};

layout(std430, binding = 16) readonly buffer CameraDataArray {
    CameraData cameraDataArray[];
};

layout(std430, binding = 3) readonly buffer materials {
    Material Materials[];
};

void main() {

	TexCoord = aTexCoord;

	mat4 projection = cameraDataArray[playerIndex].projection;
	mat4 view = cameraDataArray[playerIndex].view;
	int index = gl_InstanceID + gl_BaseInstance + instanceDataOffset;

	mat4 model = RenderItems[index].modelMatrix;
	mat4 invereseModel = RenderItems[index].inverseModelMatrix;

	Material material = Materials[RenderItems[index].materialIndex];
	BaseColorTextureIndex =  material.baseColorTextureIndex;
	NormalTextureIndex =  material.normalTextureIndex;
	RMATextureIndex =  material.rmaTextureIndex;

	mat4 normalMatrix = transpose(invereseModel);
	Normal = normalize((normalMatrix * vec4(aNormal, 0)).xyz);

	gl_Position = projection * view * model *vec4(aPos, 1.0);
}

