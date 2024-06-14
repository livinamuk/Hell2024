#version 460
#extension GL_KHR_vulkan_glsl : enable

layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec3 vNormal;
layout (location = 2) in vec2 vTexCoord;
layout (location = 3) in vec3 vTangent;

layout (location = 1) out vec2 texCoord;
layout (location = 2) out flat int BaseColorTextureIndex;
layout (location = 3) out flat int NormalTextureIndex;
layout (location = 4) out flat int RMATextureIndex;
layout (location = 5) out vec3 attrNormal;
layout (location = 6) out vec3 attrTangent;
layout (location = 7) out vec3 attrBiTangent;
layout (location = 8) out vec3 WorldPos;
layout (location = 9) out flat int playerIndex;

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

layout(set = 0, binding = 0) readonly buffer C {
    CameraData[4] data;
} cameraData;

struct SkinnedRenderItem {
    mat4 modelMatrix;
    mat4 inverseModelMatrix;

    int originalMeshIndex;
    int vertexBufferIndex;
    int baseColorTextureIndex;
    int normalTextureIndex;

    int rmaTextureIndex;
    int castShadow;
    int useEmissiveMask;
    int padding0;

    vec3 emissiveColor;
    int padding1;
};


layout( push_constant ) uniform PushConstants {
    int playerIndex;
    int renderItemIndex;
    int emptpy;
    int emptp2;
} pushConstants;

layout(std140,set = 0, binding = 6) readonly buffer A {SkinnedRenderItem data[];} renderItems;
layout(std140,set = 0, binding = 7) readonly buffer B {mat4 data[];} animatedTransforms;

void main() {

	mat4 projection = cameraData.data[pushConstants.playerIndex].projection;
	mat4 view = cameraData.data[pushConstants.playerIndex].view;

	playerIndex = pushConstants.playerIndex;
	int renderItemIndex = pushConstants.renderItemIndex;

	texCoord = vTexCoord;

	mat4 model = renderItems.data[renderItemIndex].modelMatrix;
	mat4 invereseModel = renderItems.data[renderItemIndex].inverseModelMatrix;
	BaseColorTextureIndex =  renderItems.data[renderItemIndex].baseColorTextureIndex;
	NormalTextureIndex =  renderItems.data[renderItemIndex].normalTextureIndex;
	RMATextureIndex =  renderItems.data[renderItemIndex].rmaTextureIndex;

	mat4 normalMatrix = transpose(invereseModel);
	attrNormal = normalize((normalMatrix * vec4(vNormal, 0)).xyz);
	attrTangent = normalize((normalMatrix * vec4(vTangent, 0)).xyz);;
	attrBiTangent = normalize(cross(attrNormal,attrTangent));

	gl_Position = projection * view * model * vec4(vPosition, 1.0);
}
