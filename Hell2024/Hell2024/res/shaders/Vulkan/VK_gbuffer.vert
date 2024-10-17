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
layout (location = 10) out flat int useEmissiveMask;
layout (location = 11) out flat vec3 emissiveColor;

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

layout(set = 0, binding = 0) readonly buffer CAMERA_DATA_BUFFER {
    CameraData[4] data;
} cameraData;

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

layout(set = 0, binding = 2) readonly buffer A {RenderItem3D data[];} renderItems;

layout( push_constant ) uniform constants {
	int playerIndex;
	int instanceOffset;
	int emptpy;
	int emptp2;
} PushConstants;

void main() {

	mat4 proj = cameraData.data[PushConstants.playerIndex].projection;
	mat4 view = cameraData.data[PushConstants.playerIndex].view;

	int index = gl_InstanceIndex + (PushConstants.instanceOffset);

	mat4 model = renderItems.data[index].modelMatrix;
	BaseColorTextureIndex =  renderItems.data[index].baseColorTextureIndex;
	NormalTextureIndex =  renderItems.data[index].normalMapTextureIndex;
	RMATextureIndex =  renderItems.data[index].rmaTextureIndex;

	useEmissiveMask = renderItems.data[index].useEmissiveMask;
	emissiveColor.r = renderItems.data[index].emissiveColorR;
	emissiveColor.g = renderItems.data[index].emissiveColorG;
	emissiveColor.b = renderItems.data[index].emissiveColorB;

	//model = instanceData.data[gl_InstanceIndex].modelMatrix;
	//BaseColorTextureIndex =  instanceData.data[gl_InstanceIndex].baseColorTextureIndex;
	//NormalTextureIndex =  instanceData.data[gl_InstanceIndex].normalTextureIndex;
	//RMATextureIndex =  instanceData.data[gl_InstanceIndex].rmaTextureIndex;




	mat4 normalMatrix = transpose(inverse(model));						// FIX THIS IMMEDIATELY AKA LATER
	attrNormal = normalize((normalMatrix * vec4(vNormal, 0)).xyz);
	attrTangent = (model * vec4(vTangent, 0.0)).xyz;
	attrBiTangent = normalize(cross(attrNormal,attrTangent));

	WorldPos = (model * vec4(vPosition, 1.0)).xyz;

	texCoord = vTexCoord;
	playerIndex = PushConstants.playerIndex;

	gl_Position = proj * view * vec4(WorldPos, 1.0);
}
