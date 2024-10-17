#version 460 core


layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in vec3 aTangent;

//uniform mat4 projection;
//uniform mat4 view;

out vec3 WorldPos;
out vec2 TexCoords;
out vec3 Normal;
out vec3 viewPos;

uniform int playerIndex;

out vec3 attrNormal;
out vec3 attrTangent;
out vec3 attrBiTangent;

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

layout(std430, binding = 11) readonly buffer renderItems {
    RenderItem3D RenderItems[];
};

layout(std430, binding = 3) readonly buffer materials {
    Material Materials[];
};

out flat int BaseColorTextureIndex;
out flat int NormalTextureIndex;
out flat int RMATextureIndex;

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

	viewPos = cameraDataArray[playerIndex].viewInverse[3].xyz;

	TexCoords = aTexCoord;
	mat4 model = RenderItems[gl_InstanceID + gl_BaseInstance].modelMatrix;
	mat4 invereseModel = RenderItems[gl_InstanceID + gl_BaseInstance].inverseModelMatrix;

	BaseColorTextureIndex =  RenderItems[gl_InstanceID + gl_BaseInstance].baseColorTextureIndex;
	NormalTextureIndex =  RenderItems[gl_InstanceID + gl_BaseInstance].normalMapTextureIndex;
	RMATextureIndex =  RenderItems[gl_InstanceID + gl_BaseInstance].rmaTextureIndex;

	mat4 normalMatrix = transpose(invereseModel);
	attrNormal = normalize((normalMatrix * vec4(aNormal, 0)).xyz);
	attrTangent = (model * vec4(aTangent, 0.0)).xyz;
	attrBiTangent = normalize(cross(attrNormal,attrTangent));
	TexCoords = aTexCoord;
	WorldPos = vec3(model * vec4(aPos, 1.0));
	gl_Position = projection * view * vec4(WorldPos, 1.0);

}