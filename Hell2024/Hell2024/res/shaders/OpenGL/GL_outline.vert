#version 450 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;
uniform float viewportWidth;
uniform float viewportHeight;

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

ivec2 ofssets[5] = ivec2[](
    ivec2(0, 0),
    ivec2(-1, -1),
    ivec2(-1, 1),
    ivec2(1, 1),
    ivec2(1, -1)
);

void main() {
	int playerIndex = 0;
	mat4 projection = cameraDataArray[playerIndex].projection;
	mat4 view = cameraDataArray[playerIndex].view;
	gl_Position = projection * view * model *vec4(aPos, 1.0);
	float pixelWidth = 2.0 / cameraDataArray[playerIndex].viewportWidth * 2;	// multiplying by 2 because cameraData contains the gbuffer resolution, not present buffer resolution
	float pixelHeight = 2.0 / cameraDataArray[playerIndex].viewportHeight * 2;	// multiplying by 2 because cameraData contains the gbuffer resolution, not present buffer resolution
	int offsetX = ofssets[gl_InstanceID].x;
	int offsetY = ofssets[gl_InstanceID].y;
	gl_Position.x += pixelWidth * gl_Position.z * offsetX;
	gl_Position.y += pixelHeight * gl_Position.z * offsetY;
}