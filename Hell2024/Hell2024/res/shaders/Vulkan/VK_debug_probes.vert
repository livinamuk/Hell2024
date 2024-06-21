#version 460 core
#extension GL_KHR_vulkan_glsl : enable

layout (location = 0) in vec3 aPos;

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

layout (location = 0) out vec3 Color;

void main() {

	const float cubeScale = 0.03;

	int probeSpaceWidth = 18;
	int probeSpaceHeight = 7;
	int probeSpaceDepth = 29;
	float probeSpacing = 0.375;

	const int indexZ = gl_InstanceIndex % probeSpaceDepth;
	const int indexY = (gl_InstanceIndex / probeSpaceDepth) % probeSpaceHeight;
	const int indexX = gl_InstanceIndex / (probeSpaceHeight * probeSpaceDepth); 
	
	float x = indexX * probeSpacing;
	float y = indexY * probeSpacing;
	float z = indexZ * probeSpacing;
	
	const mat4 correction = mat4(1.0, 0.0, 0.0, 0.0, 0.0, -1.0, 0.0, 0.0, 0.0, 0.0, 0.5, 0.0, 0.0, 0.0, 0.5, 1.0);
	mat4 projection = cameraData.projection;
	mat4 view = cameraData.view;

	const mat4 model = mat4(
		1.0,  0.0, 0.0, 0.0, 
		0.0,  1.0, 0.0, 0.0, 
		0.0,  0.0, 1.0, 0.0, 
		  x,    y,   z, 1.0
	);	

	Color = vec3(1,0,0);

	gl_Position = projection * view * model * vec4(aPos * cubeScale, 1.0);
}