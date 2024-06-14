#version 460
#extension GL_KHR_vulkan_glsl : enable

layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec3 vColor;

layout (location = 0) out vec3 color;

layout(set = 0, binding = 0) readonly buffer CameraData {
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
} cameraData;

void main() {

	const mat4 correction = mat4(1.0,  0.0, 0.0, 0.0, 0.0, -1.0, 0.0, 0.0, 0.0,  0.0, 0.5, 0.0, 0.0,  0.0, 0.5, 1.0);
	mat4 proj = cameraData.projection;
	mat4 view = cameraData.view;
	color = vColor;
	gl_Position = proj * view * vec4(vPosition, 1.0);
    gl_PointSize = 4.0;
}
