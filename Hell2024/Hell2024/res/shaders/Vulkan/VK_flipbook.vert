#version 460
#extension GL_KHR_vulkan_glsl : enable

layout (location = 0) in vec3 vPosition;
layout (location = 2) in vec2 vTexCoord;

layout (location = 1) out vec2 texCoord;
layout (location = 2) out vec3 FragPos;

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

struct MuzzleFlashData {
    mat4 modelMatrix;
    int frameIndex;
    int RowCount;
    int ColumnCount;
    float timeLerp;
};

layout(set = 0, binding = 0) readonly buffer CAMERA_DATA_BUFFER {
    CameraData[4] data;
} cameraData;

layout(set = 0, binding = 11) readonly buffer MUZZLE_FLASH_DATA_BUFFER {
    MuzzleFlashData[4] data;
} muzzleFlashData;

layout( push_constant ) uniform constants {
	int playerIndex;
	int instanceOffset;
	int emptpy;
	int emptp2;
} PushConstants;

void main() {
	mat4 projection = cameraData.data[PushConstants.playerIndex].projection;
	mat4 view = cameraData.data[PushConstants.playerIndex].view;
	texCoord = vec2(vTexCoord.x, vTexCoord.y);
	mat4 model = muzzleFlashData.data[PushConstants.playerIndex].modelMatrix;
	FragPos = vec4(model * vec4(vPosition.xyz, 1.0)).xyz;
	gl_Position = projection * view * vec4(FragPos, 1);
}
