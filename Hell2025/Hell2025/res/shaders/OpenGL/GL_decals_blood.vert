#version 460 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;

out vec2 TexCoord;
out flat int BaseColorTextureIndex;
out flat int NormalTextureIndex;
out flat int RMATextureIndex;

uniform int playerIndex;

struct RenderItem3D {
    mat4 modelMatrix;
    mat4 inverseModelMatrix;
    int meshIndex;
    int baseColorTextureIndex;
    int normalMapTextureIndex;
    int rmaTextureIndex;
    int vertexOffset;
    int indexOffset;
    int castShadow;
    int useEmissiveMask;
    float emissiveColorR;
    float emissiveColorG;
    float emissiveColorB;
    float aabbMinX;
    float aabbMinY;
    float aabbMinZ;
    float aabbMaxX;
    float aabbMaxY;
    float aabbMaxZ;
    float padding0;
    float padding1;
    float padding2;
};

struct Material {
	int baseColorTextureIndex;
	int normalTextureIndex;
	int rmaTextureIndex;
	int emissiveTextureIndex;
};

layout(std430, binding = 14) readonly buffer renderItems {
    RenderItem3D RenderItems[];
};

layout(std430, binding = 3) readonly buffer materials {
    Material Materials[];
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

void main() {

	mat4 projection = cameraDataArray[playerIndex].projection;
	mat4 view = cameraDataArray[playerIndex].view;

	BaseColorTextureIndex =  RenderItems[gl_InstanceID + gl_BaseInstance].baseColorTextureIndex;

	TexCoord = aTexCoord;
	mat4 model = RenderItems[gl_InstanceID + gl_BaseInstance].modelMatrix;
	gl_Position = projection * view * model *vec4(aPos, 1.0);
}

