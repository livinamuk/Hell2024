#version 460 core
#extension GL_KHR_vulkan_glsl : enable

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;

layout (location = 0) out vec2 TexCoord;
layout (location = 1) out vec3 Normal;
layout (location = 2) out flat int BaseColorTextureIndex;
layout (location = 3) out flat int NormalTextureIndex;
layout (location = 4) out flat int RMATextureIndex;
layout (location = 5) out flat int playerIndex;

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

layout(set = 0, binding = 8) readonly buffer A {RenderItem3D data[];} renderItems;


layout( push_constant ) uniform constants {
	int playerIndex;
	int instanceOffset;
	int emptpy;
	int emptp2;
} PushConstants;

void main() {	

	mat4 projection = cameraData.data[PushConstants.playerIndex].projection;
	mat4 view = cameraData.data[PushConstants.playerIndex].view;

	int index = gl_InstanceIndex + (PushConstants.instanceOffset);
			
	mat4 model = renderItems.data[index].modelMatrix;
	mat4 inverseModel = renderItems.data[index].inverseModelMatrix;
	BaseColorTextureIndex =  renderItems.data[index].baseColorTextureIndex;
	NormalTextureIndex =  renderItems.data[index].normalMapTextureIndex;
	RMATextureIndex =  renderItems.data[index].rmaTextureIndex;

	mat4 normalMatrix = transpose(inverseModel);
	Normal = normalize((normalMatrix * vec4(aNormal, 0)).xyz);
	
	TexCoord = aTexCoord;
	playerIndex = PushConstants.playerIndex;

	gl_Position = projection * view * model *vec4(aPos, 1.0);
}

