#version 460 core

layout (location = 0) in vec3 a_Position;
layout (location = 1) in vec3 a_Normal;
layout (location = 2) in vec2 a_Texcoord;

uniform int playerIndex;

out vec3 Normal;
out vec2 Texcoord;
out vec3 FragPos;

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

layout(std430, binding = 16) readonly buffer CameraDataArray {
    CameraData cameraDataArray[];
};

layout(std430, binding = 18) readonly buffer MuzzleFlashDataArray {
    MuzzleFlashData muzzleFlashDataArray[];
};

void main() {
	Texcoord = a_Texcoord;
	mat4 projection = cameraDataArray[playerIndex].projection;
	mat4 view = cameraDataArray[playerIndex].view;
	mat4 model = muzzleFlashDataArray[playerIndex].modelMatrix;
	FragPos = vec4(model * vec4(a_Position.xyz, 1.0)).xyz;
	gl_Position = projection * view * vec4(FragPos, 1);
}