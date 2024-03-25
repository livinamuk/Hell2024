#version 460
#extension GL_KHR_vulkan_glsl : enable

layout (location = 0) in vec3 vPosition;
//layout (location = 1) in vec3 vNormal;
layout (location = 2) in vec2 vTexCoord;

layout (location = 1) out vec2 texCoord;
//layout (location = 2) out flat int textureIndex;
//layout (location = 3) out vec3 color;

layout(set = 0, binding = 0) uniform GlobalShaderData {
    mat4 projection;
    mat4 projectionInverse;
    mat4 view;
    mat4 viewInverse;
} globalShaderData;

void main() {	
	mat4 proj = globalShaderData.projection;
	mat4 view = globalShaderData.view;
	
	//proj[1][1] = -proj[1][1]; // Invert the vertical field of view.
	//proj[1][3] = -proj[1][3]; // Invert the vertical field of view.

	//color = vec3(vTexCoord, 0);
	texCoord = vTexCoord;
	gl_Position = proj * view * vec4(vPosition, 1.0);
}
